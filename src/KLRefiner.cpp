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
#include <cstdlib>
#include <iostream>
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
      floorplanner_(floorplanner) {}

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
      RunFloorplanner(solution, hgraph, max_fp_steps_, max_fp_perturbations_, 0.95);
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

  // Floorplan-feasibility gate (see ChipletRefiner::SetFloorplanAware): a
  // swap is only executed if the resulting partition does not increase the
  // reach violation. Rejected pairs have their first vertex locked for the
  // rest of the pass so the search moves on.
  const bool fp_aware = cost_evaluator_ && cost_evaluator_->IsFloorplanAware();
  if (fp_aware) {
    cost_evaluator_->SetFloorplanBaseline(hgraph, solution);
  }
  const int max_fp_rejects = 4 * max_swaps_ + 8;
  int fp_rejects = 0;
  // Best prefix of swaps is chosen lexicographically: lowest reach violation
  // first, then highest cumulative gain (same policy as ChipletRefiner::Pass).
  const float start_violation =
      fp_aware ? cost_evaluator_->GetFloorplanBaselineViolation() : 0.0f;
  float best_violation = start_violation;
  float current_violation = start_violation;
  FloorplanSeed start_seed;
  std::vector<FloorplanSeed> swap_seeds;
  if (fp_aware) {
    start_seed = cost_evaluator_->GetFloorplanSeed();
  }
  const auto violation_lt = [](float a, float b) {
    return a < b - 1e-4f * std::max(1.0f, b);
  };
  
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

    if (fp_aware) {
      Partition test_solution = solution;
      std::swap(test_solution[vertex_a], test_solution[vertex_b]);
      if (!cost_evaluator_->IsFloorplanAcceptable(hgraph, test_solution)) {
        locked_vertices[vertex_a] = true;
        if (++fp_rejects >= max_fp_rejects) {
          break;
        }
        --swap_count;  // this iteration did not consume a swap
        continue;
      }
    }
    
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
    if (fp_aware) {
      cost_evaluator_->CommitFloorplanProbe();
      current_violation = cost_evaluator_->GetFloorplanBaselineViolation();
      swap_seeds.push_back(cost_evaluator_->GetFloorplanSeed());
    }
    
    // Lock these vertices for the remainder of this pass
    locked_vertices[vertex_a] = true;
    locked_vertices[vertex_b] = true;
    
    // Record the swap
    swaps.push_back(best_pair);
    
    // Update total gain
    total_gain += swap_gain;
    cumulative_gains.push_back(total_gain);
    
    // Track the best prefix (violation first, then cumulative gain)
    const bool better_violation = violation_lt(current_violation, best_violation);
    const bool same_violation = !better_violation &&
                                !violation_lt(best_violation, current_violation);
    if (better_violation || (same_violation && total_gain > best_total_gain)) {
      best_total_gain = total_gain;
      best_violation = current_violation;
      best_swap_index = swap_count;
    }
    
    // Early termination: if we haven't improved in a while, break
    if (swap_count - best_swap_index > 5) {
      break;
    }
  }
  
  // Keep the swaps up to the best point if they improved the gain or
  // repaired the floorplan violation; otherwise roll back all swaps.
  const bool repaired = violation_lt(best_violation, start_violation);
  static const bool fp_debug = std::getenv("CHIPLETPART_FP_DEBUG") != nullptr;
  if (fp_debug && fp_aware) {
    std::cerr << "[fp-debug] KL pass: start_viol=" << start_violation
              << " best_viol=" << best_violation << " swaps=" << swaps.size()
              << " best_idx=" << best_swap_index << " best_gain=" << best_total_gain
              << " fp_rejects=" << fp_rejects << "\n";
  }
  if ((best_total_gain > 0 || repaired) && best_swap_index >= 0) {
    // Rollback swaps after the best index (in reverse order)
    for (int i = swaps.size() - 1; i > best_swap_index; --i) {
      int vertex_a = swaps[i].first;
      int vertex_b = swaps[i].second;
      ExecuteSwap(hgraph, vertex_a, vertex_b, block_balance, net_degs, solution);
    }
    if (fp_aware) {
      cost_evaluator_->SetFloorplanSeed(swap_seeds[best_swap_index]);
      cost_evaluator_->SetFloorplanBaseline(hgraph, solution);
    }
    return repaired ? std::max(best_total_gain, std::numeric_limits<float>::min())
                    : best_total_gain;
  } else {
    // Rollback all swaps (in reverse order)
    for (int i = swaps.size() - 1; i >= 0; --i) {
      int vertex_a = swaps[i].first;
      int vertex_b = swaps[i].second;
      ExecuteSwap(hgraph, vertex_a, vertex_b, block_balance, net_degs, solution);
    }
    if (fp_aware) {
      cost_evaluator_->SetFloorplanSeed(start_seed);
      cost_evaluator_->SetFloorplanBaseline(hgraph, solution);
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

  // One evaluation of the current solution for the whole pair search.
  if (cost_evaluator_) {
    base_cost_ = cost_evaluator_->GetCostFromScratch(solution, true);
    base_cost_valid_ = true;
  }
  
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
  base_cost_valid_ = false;  // the solution is about to change
  
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
//
// Thread-safety: this is called from inside `#pragma omp parallel` loops in
// FindBestSwapPairs, so it must not touch shared mutable state. The previous
// implementation wrote to a shared unordered_map cache (data race) whose key
// ignored the current partition (stale values), and dereferenced
// cost_evaluator_ without a null check.
KLGain KLRefiner::CalculateSwapGain(const HGraphPtr& hgraph,
                                    int vertex_a, int block_a,
                                    int vertex_b, int block_b,
                                    const Partition& solution,
                                    const Matrix<int>& net_degs) {
  if (cost_evaluator_) {
    // Cost-model gain. The approximate model (approx_state = true) is used,
    // as in the FM refiner's move gains; the exact model is reserved for the
    // final reported cost.
    Partition test_solution = solution;
    test_solution[vertex_a] = block_b;
    test_solution[vertex_b] = block_a;
    const float cost_before = base_cost_valid_
        ? base_cost_
        : cost_evaluator_->GetCostFromScratch(solution, true);
    const float cost_after = cost_evaluator_->GetCostFromScratch(test_solution, true);
    return (cost_before - cost_after) * weight_scale_factor_;
  }

  // Fallback without a cost model: cut-size delta of the swap.
  float fast_gain = 0.0f;
  for (const int edge_a : hgraph->Edges(vertex_a)) {
    const int a_part_count = net_degs[edge_a][block_a];
    const int b_part_count = net_degs[edge_a][block_b];
    const float w = hgraph->GetHyperedgeWeights(edge_a)[0];
    if (a_part_count == 1) {
      fast_gain += w; // edge leaves block_a entirely
    } else if (a_part_count == 2 && hgraph->Vertices(edge_a).size() == 2) {
      fast_gain += w; // 2-pin net becomes internal
    }
    if (b_part_count == 0) {
      fast_gain -= w; // edge newly reaches block_b
    }
  }
  for (const int edge_b : hgraph->Edges(vertex_b)) {
    bool shared_with_a = false;
    for (const int vertex : hgraph->Vertices(edge_b)) {
      if (vertex == vertex_a) {
        shared_with_a = true;
        break;
      }
    }
    if (shared_with_a) continue;
    const int a_part_count = net_degs[edge_b][block_a];
    const int b_part_count = net_degs[edge_b][block_b];
    const float w = hgraph->GetHyperedgeWeights(edge_b)[0];
    if (b_part_count == 1) {
      fast_gain += w;
    } else if (b_part_count == 2 && hgraph->Vertices(edge_b).size() == 2) {
      fast_gain += w;
    }
    if (a_part_count == 0) {
      fast_gain -= w;
    }
  }
  return fast_gain * weight_scale_factor_;
}

// Floorplanner integration: delegate to the cost evaluator (ChipletRefiner),
// which owns the chiplet netlist / sequence-pair machinery. The result is
// applied to the evaluator so subsequent swap costs see the new shapes.
std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, bool>
KLRefiner::RunFloorplanner(std::vector<int>& partition,
                           HGraphPtr hgraph,
                           int max_steps,
                           int perturbations,
                           float cooling_acceleration_factor,
                           bool local) {
  if (!cost_evaluator_) {
    std::vector<float> dummy(1, 1.0f);
    return std::make_tuple(dummy, dummy, dummy, false);
  }
  auto start_time = std::chrono::high_resolution_clock::now();
  auto result = cost_evaluator_->RunFloorplanner(
      partition, hgraph, max_steps, perturbations, cooling_acceleration_factor, local);
  if (std::get<3>(result)) {
    cost_evaluator_->SetAspectRatios(std::get<0>(result));
    cost_evaluator_->SetXLocations(std::get<1>(result));
    cost_evaluator_->SetYLocations(std::get<2>(result));
    base_cost_valid_ = false;
  }
  auto end_time = std::chrono::high_resolution_clock::now();
  total_fplan_time_ +=
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0f;
  return result;
}

} // namespace chiplet 