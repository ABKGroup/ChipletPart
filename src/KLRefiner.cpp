///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "KLRefiner.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "OpenMPSupport.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace chiplet {

// Utility function to calculate block balance (similar to FMRefiner)
Matrix<float> GetBlockBalance(const HGraphPtr& hgraph, const Partition& solution) {
  // Find maximum partition ID
  int max_part_id = -1;
  for (int part_id : solution) {
    max_part_id = std::max(max_part_id, part_id);
  }
  
  int num_parts = max_part_id + 1;
  
  // Initialize block balance matrix
  Matrix<float> block_balance(num_parts, std::vector<float>(hgraph->GetVertexDimensions(), 0.0f));
  
  // Calculate balance for each partition
  for (int v = 0; v < hgraph->GetNumVertices(); ++v) {
    int part_id = solution[v];
    if (part_id >= 0 && part_id < num_parts) {
      const auto& weights = hgraph->GetVertexWeights(v);
      for (size_t i = 0; i < weights.size(); ++i) {
        block_balance[part_id][i] += weights[i];
      }
    }
  }
  
  return block_balance;
}

// Utility function to calculate net degrees (similar to FMRefiner)
Matrix<int> GetNetDegrees(const HGraphPtr& hgraph, const Partition& solution) {
  // Find maximum partition ID
  int max_part_id = -1;
  for (int part_id : solution) {
    max_part_id = std::max(max_part_id, part_id);
  }
  
  int num_parts = max_part_id + 1;
  
  // Initialize net degrees matrix
  Matrix<int> net_degs(hgraph->GetNumHyperedges(), std::vector<int>(num_parts, 0));
  
  // Calculate degrees for each hyperedge
  for (int e = 0; e < hgraph->GetNumHyperedges(); ++e) {
    for (const int vertex_id : hgraph->Vertices(e)) {
      int part_id = solution[vertex_id];
      if (part_id >= 0 && part_id < num_parts) {
        net_degs[e][part_id]++;
      }
    }
  }
  
  return net_degs;
}

// Constructor
KLRefiner::KLRefiner(int num_parts, int refiner_iters, int max_swaps, bool floorplanner)
    : num_parts_(num_parts), 
      refiner_iters_(refiner_iters), 
      max_swaps_(max_swaps),
      floorplanner_(floorplanner) {
  
  // Get available threads for parallel processing
  int available_threads = omp_utils::get_max_threads();
  num_fp_workers_ = std::max(2, std::min(available_threads / 2, 4));
}

// Main refinement method
void KLRefiner::Refine(const HGraphPtr& hgraph,
                       const Matrix<float>& upper_block_balance,
                       const Matrix<float>& lower_block_balance,
                       Partition& solution) {
  
  auto start_time = std::chrono::high_resolution_clock::now();
  
  // Calculate initial block balance and net degrees
  Matrix<float> cur_block_balance = GetBlockBalance(hgraph, solution);
  Matrix<int> net_degs = GetNetDegrees(hgraph, solution);
  
  float total_improvement = 0.0;
  
  // Determine iteration count based on graph size
  int actual_iterations = refiner_iters_;
  const int num_vertices = hgraph->GetNumVertices();
  const int num_hyperedges = hgraph->GetNumHyperedges();
  
  // Reduce iterations for very large graphs to improve runtime
  if (num_vertices > 1000) {
    actual_iterations = std::min(refiner_iters_, 2);
  }
  
  // Determine max swaps based on graph size
  int actual_max_swaps = max_swaps_;
  if (num_vertices > 500) {
    // Limit max swaps to approximately sqrt(n) for large graphs
    actual_max_swaps = std::min(max_swaps_, static_cast<int>(std::sqrt(num_vertices) * 3));
  }
  
  // Perform multiple KL refinement passes
  for (int i = 0; i < actual_iterations; ++i) {
    
    // Use floorplanner if enabled
    if (floorplanner_) {
      
      // Run floorplanner (local=true for finer granularity)
      auto fp_result = RunFloorplanner(solution, hgraph, max_fp_steps_, max_fp_perturbations_, 0.95);
      
      bool success = std::get<3>(fp_result);
    }
    
    // Save the current max_swaps_ value
    int saved_max_swaps = max_swaps_;
    
    // Set the max_swaps_ to the adaptive value for this iteration
    max_swaps_ = actual_max_swaps;
    
    // Run a KL pass to improve the partition
    float improvement = KLPass(hgraph, upper_block_balance, lower_block_balance, 
                              cur_block_balance, net_degs, solution);
    
    // Restore the original max_swaps_ value
    max_swaps_ = saved_max_swaps;
    
    total_improvement += improvement;
    
    // Stop if no improvement
    if (improvement <= 0) {
      break;
    }
  }
  
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  total_refine_time_ += duration.count() / 1000.0f;
}

// Core KL pass implementation
float KLRefiner::KLPass(const HGraphPtr& hgraph,
                        const Matrix<float>& upper_block_balance,
                        const Matrix<float>& lower_block_balance, 
                        Matrix<float>& block_balance,
                        Matrix<int>& net_degs,
                        Partition& solution) {
  
  // Clear the gain cache at the beginning of each pass
  gain_cache_.clear();
  
  const int num_vertices = hgraph->GetNumVertices();
  
  // Track locked vertices (those that have been swapped in this pass)
  std::vector<bool> locked_vertices(num_vertices, false);
  
  // Track all swaps in this pass
  std::vector<GainPair> swaps;
  swaps.reserve(max_swaps_);
  
  // Track cumulative gain after each swap
  std::vector<float> cumulative_gains;
  cumulative_gains.reserve(max_swaps_);
  
  float total_gain = 0.0f;
  float best_total_gain = 0.0f;
  int best_swap_index = -1;
  
  // Counter for consecutive non-positive gain swaps
  int consecutive_non_positive_swaps = 0;
  const int max_consecutive_non_positive = 3; // Stop after this many consecutive non-positive swaps
  
  // Perform up to max_swaps_ vertex pair swaps
  for (int swap_count = 0; swap_count < max_swaps_; ++swap_count) {
    // Find the best pair of vertices to swap
    GainPair best_pair = FindBestSwapPair(hgraph, block_balance, 
                                         upper_block_balance, lower_block_balance,
                                         solution, net_degs, locked_vertices);
    
    // Check if we found a valid pair
    if (best_pair.first < 0 || best_pair.second < 0) {
      break;
    }
    
    int vertex_a = best_pair.first;
    int vertex_b = best_pair.second;
    
    // Calculate gain for this swap
    int block_a = solution[vertex_a];
    int block_b = solution[vertex_b];
    float swap_gain = CalculateSwapGain(hgraph, vertex_a, block_a, vertex_b, block_b, solution, net_degs);
    
    // Early termination: if gain is too small, it's not worth continuing
    if (swap_gain <= 0) {
      consecutive_non_positive_swaps++;
      if (consecutive_non_positive_swaps >= max_consecutive_non_positive) {
        break;
      }
    } else {
      consecutive_non_positive_swaps = 0; // Reset the counter on positive gain
    }
    
    // Execute the swap
    ExecuteSwap(hgraph, vertex_a, vertex_b, block_balance, net_degs, solution);
    
    // Lock these vertices for the remainder of this pass
    locked_vertices[vertex_a] = true;
    locked_vertices[vertex_b] = true;
    
    // Record the swap
    swaps.push_back(best_pair);
    
    // Update total gain
    total_gain += swap_gain;
    cumulative_gains.push_back(total_gain);
    
    // Track the point of maximum cumulative gain
    if (total_gain > best_total_gain) {
      best_total_gain = total_gain;
      best_swap_index = swap_count;
    }
    
    // Early termination: if we haven't improved in a while, break
    if (swap_count - best_swap_index > 5) {
      break;
    }
  }
  
  // If best gain is positive, keep swaps up to the best point
  // If negative, roll back all swaps
  if (best_total_gain > 0 && best_swap_index >= 0) {
    // Rollback swaps after the best index (in reverse order)
    for (int i = swaps.size() - 1; i > best_swap_index; --i) {
      int vertex_a = swaps[i].first;
      int vertex_b = swaps[i].second;
      ExecuteSwap(hgraph, vertex_a, vertex_b, block_balance, net_degs, solution);
    }
    
    return best_total_gain;
  } else {
    // Rollback all swaps (in reverse order)
    for (int i = swaps.size() - 1; i >= 0; --i) {
      int vertex_a = swaps[i].first;
      int vertex_b = swaps[i].second;
      ExecuteSwap(hgraph, vertex_a, vertex_b, block_balance, net_degs, solution);
    }

    return 0.0f;
  }
}

// Find the best pair of vertices to swap
GainPair KLRefiner::FindBestSwapPair(const HGraphPtr& hgraph,
                                    const Matrix<float>& block_balance,
                                    const Matrix<float>& upper_block_balance,
                                    const Matrix<float>& lower_block_balance,
                                    const Partition& solution,
                                    const Matrix<int>& net_degs,
                                    const std::vector<bool>& locked_vertices) {
  
  const int num_vertices = hgraph->GetNumVertices();
  float best_gain = -std::numeric_limits<float>::max();
  GainPair best_pair(-1, -1);
  
  // Organize vertices by partition
  std::vector<std::vector<int>> part_vertices(num_parts_);
  for (int v = 0; v < num_vertices; ++v) {
    if (!locked_vertices[v]) {
      part_vertices[solution[v]].push_back(v);
    }
  }
  
  // Identify boundary vertices (connected to another partition)
  std::vector<bool> is_boundary(num_vertices, false);
  for (int v = 0; v < num_vertices; ++v) {
    if (locked_vertices[v]) continue;
    
    int part_id = solution[v];
    for (int he : hgraph->Edges(v)) {
      for (int u : hgraph->Vertices(he)) {
        if (u != v && solution[u] != part_id) {
          is_boundary[v] = true;
          break;
        }
      }
      if (is_boundary[v]) break;
    }
  }
  
  // For each pair of different partitions
  for (int part_a = 0; part_a < num_parts_; ++part_a) {
    // Skip empty partitions
    if (part_vertices[part_a].empty()) continue;
    
    for (int part_b = part_a + 1; part_b < num_parts_; ++part_b) {
      // Skip empty partitions
      if (part_vertices[part_b].empty()) continue;
      
      const auto& vertices_a = part_vertices[part_a];
      const auto& vertices_b = part_vertices[part_b];
      
      // Focus on boundary vertices which are more likely to provide good gains
      std::vector<int> boundary_a, boundary_b;
      for (int v : vertices_a) {
        if (is_boundary[v]) boundary_a.push_back(v);
      }
      
      for (int v : vertices_b) {
        if (is_boundary[v]) boundary_b.push_back(v);
      }
      
      // If we have boundary vertices, use them; otherwise use all vertices
      const auto& search_a = boundary_a.empty() ? vertices_a : boundary_a;
      const auto& search_b = boundary_b.empty() ? vertices_b : boundary_b;
      
      // Limit search space for very large partitions
      const int max_pairs_to_check = 500;
      const bool limit_search = search_a.size() * search_b.size() > max_pairs_to_check;
      
      // Use OpenMP for parallelism if there are many vertex pairs
      bool found_better_pair = false;
      
      #if HAVE_OPENMP
      if (search_a.size() * search_b.size() > 100) {
        float thread_best_gain = -std::numeric_limits<float>::max();
        GainPair thread_best_pair(-1, -1);
        
        // Determine sampling if we need to limit the search space
        std::vector<int> indicesa, indicesb;
        if (limit_search) {
          // Create random samples from each partition
          indicesa.reserve(std::min(static_cast<int>(search_a.size()), max_pairs_to_check/10));
          indicesb.reserve(std::min(static_cast<int>(search_b.size()), max_pairs_to_check/10));
          
          std::random_device rd;
          std::mt19937 gen(rd());
          
          // Sample vertices from partition A
          std::sample(search_a.begin(), search_a.end(),
                     std::back_inserter(indicesa),
                     max_pairs_to_check/10,
                     gen);
                     
          // Sample vertices from partition B
          std::sample(search_b.begin(), search_b.end(),
                     std::back_inserter(indicesb),
                     max_pairs_to_check/10,
                     gen);
        }
        
        #pragma omp parallel
        {
          float local_best_gain = -std::numeric_limits<float>::max();
          GainPair local_best_pair(-1, -1);
          
          if (limit_search) {
            // Check only sampled pairs
            #pragma omp for schedule(dynamic, 16) nowait
            for (int i = 0; i < indicesa.size(); ++i) {
              for (int j = 0; j < indicesb.size(); ++j) {
                int v_a = indicesa[i];
                int v_b = indicesb[j];
                
                // Check if swap is legal
                if (!IsSwapLegal(v_a, part_a, v_b, part_b, 
                                hgraph, block_balance, 
                                upper_block_balance, lower_block_balance)) {
                  continue;
                }
                
                // Calculate gain
                float gain = CalculateSwapGain(hgraph, v_a, part_a, v_b, part_b, solution, net_degs);
                
                if (gain > local_best_gain) {
                  local_best_gain = gain;
                  local_best_pair = std::make_pair(v_a, v_b);
                }
              }
            }
          } else {
            // Check all pairs
            #pragma omp for schedule(dynamic, 16) nowait
            for (int i = 0; i < search_a.size(); ++i) {
              for (int j = 0; j < search_b.size(); ++j) {
                int v_a = search_a[i];
                int v_b = search_b[j];
                
                // Check if swap is legal
                if (!IsSwapLegal(v_a, part_a, v_b, part_b, 
                                hgraph, block_balance, 
                                upper_block_balance, lower_block_balance)) {
                  continue;
                }
                
                // Calculate gain
                float gain = CalculateSwapGain(hgraph, v_a, part_a, v_b, part_b, solution, net_degs);
                
                if (gain > local_best_gain) {
                  local_best_gain = gain;
                  local_best_pair = std::make_pair(v_a, v_b);
                }
              }
            }
          }
          
          // Update thread best with thread-local best
          #pragma omp critical
          {
            if (local_best_gain > thread_best_gain) {
              thread_best_gain = local_best_gain;
              thread_best_pair = local_best_pair;
              found_better_pair = true;
            }
          }
        }
        
        // Update global best with thread best
        if (found_better_pair && thread_best_gain > best_gain) {
          best_gain = thread_best_gain;
          best_pair = thread_best_pair;
        }
      } else 
      #endif
      {
        // Sequential processing for small number of vertex pairs
        std::vector<int> indicesa, indicesb;
        if (limit_search) {
          // Same sampling logic for sequential case
          std::random_device rd;
          std::mt19937 gen(rd());
          
          indicesa.reserve(std::min(static_cast<int>(search_a.size()), max_pairs_to_check/10));
          indicesb.reserve(std::min(static_cast<int>(search_b.size()), max_pairs_to_check/10));
          
          std::sample(search_a.begin(), search_a.end(),
                     std::back_inserter(indicesa),
                     max_pairs_to_check/10,
                     gen);
                     
          std::sample(search_b.begin(), search_b.end(),
                     std::back_inserter(indicesb),
                     max_pairs_to_check/10,
                     gen);
                     
          for (int v_a : indicesa) {
            for (int v_b : indicesb) {
              // Check if swap is legal
              if (!IsSwapLegal(v_a, part_a, v_b, part_b, 
                              hgraph, block_balance, 
                              upper_block_balance, lower_block_balance)) {
                continue;
              }
              
              // Calculate gain
              float gain = CalculateSwapGain(hgraph, v_a, part_a, v_b, part_b, solution, net_degs);
              
              if (gain > best_gain) {
                best_gain = gain;
                best_pair = std::make_pair(v_a, v_b);
                found_better_pair = true;
              }
            }
          }
        } else {
          for (int v_a : search_a) {
            for (int v_b : search_b) {
              // Check if swap is legal
              if (!IsSwapLegal(v_a, part_a, v_b, part_b, 
                              hgraph, block_balance, 
                              upper_block_balance, lower_block_balance)) {
                continue;
              }
              
              // Calculate gain
              float gain = CalculateSwapGain(hgraph, v_a, part_a, v_b, part_b, solution, net_degs);
              
              if (gain > best_gain) {
                best_gain = gain;
                best_pair = std::make_pair(v_a, v_b);
                found_better_pair = true;
              }
            }
          }
        }
      }
    }
  }
  
  return best_pair;
}

// Execute a vertex swap
void KLRefiner::ExecuteSwap(const HGraphPtr& hgraph,
                           int vertex_a, int vertex_b,
                           Matrix<float>& block_balance,
                           Matrix<int>& net_degs,
                           Partition& solution) {
  
  // Get current partitions
  int block_a = solution[vertex_a];
  int block_b = solution[vertex_b];
  
  // Swap vertices between partitions
  solution[vertex_a] = block_b;
  solution[vertex_b] = block_a;
  
  // Update block balance
  const std::vector<float>& weight_a = hgraph->GetVertexWeights(vertex_a);
  const std::vector<float>& weight_b = hgraph->GetVertexWeights(vertex_b);
  
  for (size_t i = 0; i < weight_a.size(); ++i) {
    block_balance[block_a][i] -= weight_a[i];
    block_balance[block_a][i] += weight_b[i];
    block_balance[block_b][i] += weight_a[i];
    block_balance[block_b][i] -= weight_b[i];
  }
  
  // Update net degrees
  // For each hyperedge connected to vertex_a
  for (const int edge_a : hgraph->Edges(vertex_a)) {
    --net_degs[edge_a][block_a]; // Remove vertex_a from block_a
    ++net_degs[edge_a][block_b]; // Add vertex_a to block_b
  }
  
  // For each hyperedge connected to vertex_b
  for (const int edge_b : hgraph->Edges(vertex_b)) {
    --net_degs[edge_b][block_b]; // Remove vertex_b from block_b
    ++net_degs[edge_b][block_a]; // Add vertex_b to block_a
  }
}

// Check if a swap is legal (respects balance constraints)
bool KLRefiner::IsSwapLegal(int vertex_a, int block_a, 
                           int vertex_b, int block_b,
                           const HGraphPtr& hgraph,
                           const Matrix<float>& block_balance,
                           const Matrix<float>& upper_block_balance,
                           const Matrix<float>& lower_block_balance) {
  
  // Get vertex weights
  const std::vector<float>& weight_a = hgraph->GetVertexWeights(vertex_a);
  const std::vector<float>& weight_b = hgraph->GetVertexWeights(vertex_b);
  
  // Calculate new block balances after the swap
  std::vector<float> new_balance_a = block_balance[block_a];
  std::vector<float> new_balance_b = block_balance[block_b];
  
  for (size_t i = 0; i < weight_a.size(); ++i) {
    new_balance_a[i] -= weight_a[i];
    new_balance_a[i] += weight_b[i];
    new_balance_b[i] += weight_a[i];
    new_balance_b[i] -= weight_b[i];
  }
  
  // Check if new balances are within constraints
  for (size_t i = 0; i < new_balance_a.size(); ++i) {
    if (new_balance_a[i] < lower_block_balance[block_a][i] || 
        new_balance_a[i] > upper_block_balance[block_a][i]) {
      return false;
    }
  }
  
  for (size_t i = 0; i < new_balance_b.size(); ++i) {
    if (new_balance_b[i] < lower_block_balance[block_b][i] || 
        new_balance_b[i] > upper_block_balance[block_b][i]) {
      return false;
    }
  }
  
  return true;
}

// Calculate gain for swapping a pair of vertices
KLGain KLRefiner::CalculateSwapGain(const HGraphPtr& hgraph,
                                    int vertex_a, int block_a,
                                    int vertex_b, int block_b,
                                    const Partition& solution,
                                    const Matrix<int>& net_degs) {
  // Use fast cut-size based gain calculation for initial estimates
  // This avoids expensive cost model calls for obvious bad swaps
  /*float fast_gain = 0.0f;
  
  // Process nets connected to vertex_a
  for (const int edge_a : hgraph->Edges(vertex_a)) {
    // Current edge distribution
    int a_part_count = net_degs[edge_a][block_a];
    int b_part_count = net_degs[edge_a][block_b];
    
    // After the swap, a_part_count will decrease by 1 and b_part_count will increase by 1
    
    // Calculate cutset change 
    if (a_part_count == 1) {
      // Edge will no longer connect to block_a after the swap (no longer in cutset)
      fast_gain += hgraph->GetHyperedgeWeights(edge_a)[0];
    } else if (a_part_count == 2 && hgraph->Vertices(edge_a).size() == 2) {
      // Special case: this is a 2-pin net, currently bridging blocks, but will be internal
      fast_gain += hgraph->GetHyperedgeWeights(edge_a)[0];
    }
    
    if (b_part_count == 0) {
      // Edge will connect to block_b after the swap (adding to cutset)
      fast_gain -= hgraph->GetHyperedgeWeights(edge_a)[0];
    }
  }
  
  // Process nets connected to vertex_b (similar logic)
  for (const int edge_b : hgraph->Edges(vertex_b)) {
    // Skip edges that we've already processed for vertex_a
    bool already_processed = false;
    for (const int vertex : hgraph->Vertices(edge_b)) {
      if (vertex == vertex_a) {
        already_processed = true;
        break;
      }
    }
    if (already_processed) continue;
    
    // Current edge distribution
    int a_part_count = net_degs[edge_b][block_a];
    int b_part_count = net_degs[edge_b][block_b];
    
    if (b_part_count == 1) {
      // Edge will no longer connect to block_b after the swap
      fast_gain += hgraph->GetHyperedgeWeights(edge_b)[0];
    } else if (b_part_count == 2 && hgraph->Vertices(edge_b).size() == 2) {
      // Special case: 2-pin net that will be internal after swap
      fast_gain += hgraph->GetHyperedgeWeights(edge_b)[0];
    }
    
    if (a_part_count == 0) {
      // Edge will connect to block_a after the swap
      fast_gain -= hgraph->GetHyperedgeWeights(edge_b)[0];
    }
  }
  
  // Early exit with fast gain calculation if the gain is clearly negative
  // This avoids expensive cost model calls for unpromising swaps
  if (fast_gain < 0 && fast_gain * weight_scale_factor_ < -10.0) {
    return fast_gain * weight_scale_factor_;
  }
  
  // For promising swaps, use the cost model if available
  if (cost_evaluator_) {
    // Create a copy of the solution to test the swap
    Partition test_solution = solution;
    
    // Store original solution
    int original_block_a = solution[vertex_a];
    int original_block_b = solution[vertex_b];
    
    // Perform the swap
    test_solution[vertex_a] = block_b;
    test_solution[vertex_b] = block_a;
    
    // Get costs - check if cached
    float cost_before, cost_after;
    
    // Reference the original solution for caching
    std::string solution_hash = std::to_string(vertex_a) + "_" + std::to_string(vertex_b);
    
    // Use cached cost_before if available from previous calculations
    if (gain_cache_.find(solution_hash) != gain_cache_.end()) {
      float cached_gain = gain_cache_[solution_hash];
      return cached_gain;
    }*/
    
    // No cached value, calculate gain using cost model
    Partition test_solution = solution;
    test_solution[vertex_a] = block_b;
    test_solution[vertex_b] = block_a;
    float cost_before = cost_evaluator_->GetCostFromScratch(solution);
    float cost_after = cost_evaluator_->GetCostFromScratch(test_solution);
    std::string solution_hash = std::to_string(vertex_a) + "_" + std::to_string(vertex_b);
    // Calculate gain
    float gain = (cost_before - cost_after) * weight_scale_factor_;
    
    // Cache the result for future use
    gain_cache_[solution_hash] = gain;
    
    return gain;
  //}
  
  // If no cost model available, return the fast gain calculation
  //return fast_gain * weight_scale_factor_;
}

// Floorplanner integration
std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, bool>
KLRefiner::RunFloorplanner(std::vector<int>& partition, 
                          HGraphPtr hgraph, 
                          int max_steps,
                          int perturbations, 
                          float cooling_acceleration_factor,
                          bool local) {
  
  auto start_time = std::chrono::high_resolution_clock::now();
  
  try {
    
    // Generate netlist from partition
    chiplet_graph_ = GenerateNetlist(hgraph, partition);
    
    if (!chiplet_graph_) {
      std::cerr << "[KL-FLOORPLAN] Failed to generate chiplet netlist" << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Build chiplets
    BuildChiplets(chiplet_graph_);
    
    if (chiplets_.empty()) {
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Initialize sequence pairs
    if (local) {
      if (local_pos_seq_.size() != chiplets_.size() || local_neg_seq_.size() != chiplets_.size()) {
        local_pos_seq_.resize(chiplets_.size());
        local_neg_seq_.resize(chiplets_.size());
        std::iota(local_pos_seq_.begin(), local_pos_seq_.end(), 0);
        std::iota(local_neg_seq_.begin(), local_neg_seq_.end(), 0);
      }
    } else {
      if (global_pos_seq_.size() != chiplets_.size() || global_neg_seq_.size() != chiplets_.size()) {
        global_pos_seq_.resize(chiplets_.size());
        global_neg_seq_.resize(chiplets_.size());
        std::iota(global_pos_seq_.begin(), global_pos_seq_.end(), 0);
        std::iota(global_neg_seq_.begin(), global_neg_seq_.end(), 0);
      }
    }
    
    // Select the appropriate sequences
    std::vector<int> pos_seq = local ? local_pos_seq_ : global_pos_seq_;
    std::vector<int> neg_seq = local ? local_neg_seq_ : global_neg_seq_;
    
    // Create workers for floorplanning
    std::vector<std::unique_ptr<SACore>> workers;
    workers.reserve(num_fp_workers_);
    
    // Distribute cooling rates among workers
    float delta_cooling_rate = (0.99 - 0.9) / (num_fp_workers_ > 1 ? num_fp_workers_ - 1 : 1);
    
    // Adjust steps and perturbations per worker
    int per_worker_steps = max_steps / (num_fp_workers_ > 0 ? num_fp_workers_ : 1);
    int per_worker_perturbations = perturbations / (num_fp_workers_ > 0 ? num_fp_workers_ : 1);
    
    // Ensure reasonable minimum values
    per_worker_steps = std::max(10, per_worker_steps);
    per_worker_perturbations = std::max(5, per_worker_perturbations);
    
    // Create workers
    for (int worker_id = 0; worker_id < num_fp_workers_; worker_id++) {
      try {
        // Assign cooling rate for this worker
        float worker_cooling_rate = 0.9 + worker_id * delta_cooling_rate;
        
        // Create worker
        auto sa = std::make_unique<SACore>(
            worker_id,                     // worker_id
            chiplets_,                     // chiplets
            bundled_nets_,                 // bundled_nets
            1.0,                           // area_penalty_weight
            1.0,                           // package_penalty_weight
            1.0,                           // net_penalty_weight
            0.2,                           // pos_swap_prob
            0.2,                           // neg_swap_prob
            0.2,                           // double_swap_prob
            0.2,                           // resize_prob
            0.2,                           // expand_prob
            per_worker_steps,              // max_num_step
            per_worker_perturbations,      // num_perturb_per_step
            worker_cooling_rate,           // cooling_rate
            42 + worker_id);               // seed
        
        // Set sequence pairs
        sa->setPosSeq(pos_seq);
        sa->setNegSeq(neg_seq);
        
        // Add to workers vector
        workers.push_back(std::move(sa));
      } catch (const std::exception& e) {
        std::cerr << "[KL-FLOORPLAN] Exception creating worker " << worker_id 
                  << ": " << e.what() << std::endl;
      }
    }
    
    if (workers.empty()) {
      std::cerr << "[KL-FLOORPLAN] Failed to create any workers" << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Initialize workers
    try {
      workers[0]->initialize();
      float norm_area_penalty = workers[0]->getNormAreaPenalty();
      float norm_package_penalty = workers[0]->getNormPackagePenalty();
      float norm_net_penalty = workers[0]->getNormNetPenalty();
      
      // Apply normalization to all workers
      for (auto& worker : workers) {
        worker->setNormAreaPenalty(norm_area_penalty);
        worker->setNormPackagePenalty(norm_package_penalty);
        worker->setNormNetPenalty(norm_net_penalty);
      }
    } catch (const std::exception& e) {
      std::cerr << "[KL-FLOORPLAN] Exception during worker initialization: " 
                << e.what() << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Run workers
    for (auto& worker : workers) {
      try {
        worker->run();
      } catch (const std::exception& e) {
        std::cerr << "[KL-FLOORPLAN] Exception running worker " 
                  << worker->getWorkerId() << ": " << e.what() << std::endl;
      }
    }
    
    // Find best solution
    SACore* best_sa = nullptr;
    float best_cost = std::numeric_limits<float>::max();
    bool is_valid = false;
    
    for (auto& worker : workers) {
      try {
        float current_cost = worker->getCost();
        bool current_valid = worker->isValid();
        
        // Prefer valid solutions, then lower cost
        if ((current_valid && !is_valid) || 
            (current_valid == is_valid && current_cost < best_cost)) {
          best_sa = worker.get();
          best_cost = current_cost;
          is_valid = current_valid;
        }
      } catch (const std::exception& e) {
        std::cerr << "[KL-FLOORPLAN] Exception evaluating worker: " << e.what() << std::endl;
      }
    }
    
    // Use first worker if no valid solution found
    if (!best_sa && !workers.empty()) {
      best_sa = workers[0].get();
      is_valid = false;
    }
    
    // Extract results
    std::vector<Chiplet> best_chiplets;
    if (best_sa) {
      try {
        best_sa->getMacros(best_chiplets);
        
        // Store sequence pairs
        std::vector<int> final_pos_seq, final_neg_seq;
        best_sa->getPosSeq(final_pos_seq);
        best_sa->getNegSeq(final_neg_seq);
        
        if (local) {
          local_pos_seq_ = final_pos_seq;
          local_neg_seq_ = final_neg_seq;
        } else {
          global_pos_seq_ = final_pos_seq;
          global_neg_seq_ = final_neg_seq;
        }
      } catch (const std::exception& e) {
        std::cerr << "[KL-FLOORPLAN] Exception extracting best solution: " 
                  << e.what() << std::endl;
        std::vector<float> dummy_result(1, 1.0);
        return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
      }
    } else {
      std::cerr << "[KL-FLOORPLAN] No solution found" << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Calculate return values
    std::vector<float> aspect_ratios;
    std::vector<float> x_locations;
    std::vector<float> y_locations;
    
    aspect_ratios.reserve(best_chiplets.size());
    x_locations.reserve(best_chiplets.size());
    y_locations.reserve(best_chiplets.size());
    
    for (const auto& chiplet : best_chiplets) {
      aspect_ratios.push_back(chiplet.getRealWidth() / std::max(0.001f, chiplet.getRealHeight()));
      x_locations.push_back(chiplet.getRealX());
      y_locations.push_back(chiplet.getRealY());
    }
    
    // Update timing stats
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    total_fplan_time_ += duration.count() / 1000.0f;
    
    return std::make_tuple(aspect_ratios, x_locations, y_locations, is_valid);
    
  } catch (const std::exception& e) {
    std::cerr << "[KL-FLOORPLAN] Exception in RunFloorplanner: " << e.what() << std::endl;
    std::vector<float> dummy_result(1, 1.0);
    return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
  }
}

// Build chiplets for floorplanning
void KLRefiner::BuildChiplets(const HGraphPtr &hgraph) {
  // Clear existing chiplets and nets
  bundled_nets_.clear();
  chiplets_.clear();
  
  // Create bundled nets for each hyperedge
  const int num_edges = hgraph->GetNumHyperedges();
  bundled_nets_.reserve(num_edges);
  
  for (int i = 0; i < num_edges; ++i) {
    const auto& vertices = hgraph->Vertices(i);
    
    // Skip hyperedges with fewer than 2 vertices
    if (vertices.size() < 2) {
      continue;
    }
    
    // Get the first two vertices as terminals
    const int term_a = *vertices.begin();
    const int term_b = *(++vertices.begin());
    
    // Get edge weight and reach
    const float weight = hgraph->GetHyperedgeWeights(i)[0];
    float reach = 1.0f;
    
    // Get reach value if available
    try {
      reach = hgraph->GetReach(i);
    } catch (const std::exception& e) {
      // Default to 1.0 if not available
    }
    
    // Get IO size if available
    float io_area = 1.0f;
    try {
      io_area = hgraph->GetIoSize(i);
    } catch (const std::exception& e) {
      // Default to 1.0 if not available
    }
    
    // Create bundled net
    bundled_nets_.emplace_back(std::pair<int, int>(term_a, term_b), weight, reach, io_area);
  }
  
  // Create chiplets for each vertex
  const int num_vertices = hgraph->GetNumVertices();
  chiplets_.reserve(num_vertices);
  
  for (int i = 0; i < num_vertices; ++i) {
    // Get vertex weight as area
    const auto& vertex_weight = hgraph->GetVertexWeights(i);
    const float area = std::accumulate(vertex_weight.begin(), vertex_weight.end(), 0.0f);
    
    // Calculate dimensions (square aspect ratio by default)
    const float width = std::sqrt(area);
    const float height = width;
    
    // Create chiplet
    chiplets_.emplace_back(0.0f, 0.0f, width, height, area, separation_);
  }
}

// Generate a netlist for chiplet-level floorplanning
HGraphPtr KLRefiner::GenerateNetlist(const HGraphPtr hgraph, const std::vector<int>& partition) {
  // Calculate block balances once and reuse
  Matrix<float> vertex_weights_c;
  vertex_weights_c.resize(num_parts_);
  
  for (int i = 0; i < num_parts_; ++i) {
    vertex_weights_c[i].resize(hgraph->GetVertexDimensions(), 0.0f);
  }
  
  // Compute weights for each partition
  for (int v = 0; v < hgraph->GetNumVertices(); ++v) {
    int part_id = partition[v];
    if (part_id >= 0 && part_id < num_parts_) {
      const auto& weights = hgraph->GetVertexWeights(v);
      for (size_t i = 0; i < weights.size(); ++i) {
        vertex_weights_c[part_id][i] += weights[i];
      }
    }
  }
  
  // Only include partitions with non-zero weight
  Matrix<float> new_vertex_weights_c;
  std::vector<int> part_map;
  
  for (int i = 0; i < vertex_weights_c.size(); ++i) {
    if (vertex_weights_c[i][0] > 0.0f) {
      new_vertex_weights_c.push_back(vertex_weights_c[i]);
      part_map.push_back(i);
    }
  }
  
  // Create hyperedges for the netlist
  Matrix<int> hyperedges_c;
  Matrix<float> hyperedges_weights_c;
  std::vector<float> reaches;
  std::vector<float> io_cell_sizes;
  
  // Process hyperedges to create chiplet-level connections
  for (int e = 0; e < hgraph->GetNumHyperedges(); ++e) {
    // Track which partitions this hyperedge spans
    std::unordered_set<int> parts_seen;
    for (const int vertex_id : hgraph->Vertices(e)) {
      int part_id = partition[vertex_id];
      if (part_id >= 0 && part_id < num_parts_) {
        parts_seen.insert(part_id);
      }
    }
    
    // Only include hyperedges that span multiple partitions
    if (parts_seen.size() <= 1) {
      continue;
    }
    
    // Create hyperedge connecting the partitions
    std::vector<int> he_parts;
    for (int part_id : parts_seen) {
      // Map to the new index in new_vertex_weights_c
      auto it = std::find(part_map.begin(), part_map.end(), part_id);
      if (it != part_map.end()) {
        int new_id = std::distance(part_map.begin(), it);
        he_parts.push_back(new_id);
      }
    }
    
    // Add the hyperedge
    hyperedges_c.push_back(he_parts);
    hyperedges_weights_c.push_back(hgraph->GetHyperedgeWeights(e));
    
    // Get reach and IO size if available
    float reach = 1.0f;
    try {
      reach = hgraph->GetReach(e);
    } catch (const std::exception& e) {
      // Default to 1.0 if not available
    }
    reaches.push_back(reach);
    
    float io_size = 1.0f;
    try {
      io_size = hgraph->GetIoSize(e);
    } catch (const std::exception& e) {
      // Default to 1.0 if not available
    }
    io_cell_sizes.push_back(io_size);
  }
  
  // Create the chiplet-level hypergraph
  return std::make_shared<Hypergraph>(
      hgraph->GetVertexDimensions(),
      hgraph->GetHyperedgeDimensions(),
      hyperedges_c,
      new_vertex_weights_c,
      hyperedges_weights_c,
      reaches,
      io_cell_sizes);
}

} // namespace chiplet 