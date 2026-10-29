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
#include "FMRefiner.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <functional>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>

// Include the unified OpenMP support header instead of direct include
#include "OpenMPSupport.h"

namespace chiplet {

// Extra FM passes granted to feasibility-aware refinement while the reach
// violation is still decreasing (see ChipletRefiner::Refine).
constexpr int kExtraRepairPasses = 5;

ChipletRefiner::ChipletRefiner(
    const int num_parts, const int refiner_iters,
    const int max_move, // the maximum number of vertices or
                        // hyperedges can be moved in each pass
    std::vector<int> reaches,
    bool floorplanner,
    const std::string& io_file,
    const std::string& layer_file,
    const std::string& wafer_process_file,
    const std::string& assembly_process_file,
    const std::string& test_file,
    const std::string& netlist_file,
    const std::string& blocks_file)
    : num_parts_(num_parts), refiner_iters_(refiner_iters), max_move_(max_move),
      refiner_iters_default_(refiner_iters), max_move_default_(max_move),
      reaches_(reaches), floorplanner_(floorplanner) {
  // Get available threads using our OpenMP utilities
  num_threads_ = omp_utils::get_max_threads();
  
  // Use 10% of available threads and ensure at least 2
  num_threads_ = std::max(2, static_cast<int>(num_threads_ * 0.1));
  
  
  // Set cost model files if provided
  if (!io_file.empty() && !layer_file.empty() && !wafer_process_file.empty() && 
      !assembly_process_file.empty() && !test_file.empty() && !netlist_file.empty() && 
      !blocks_file.empty()) {
    
    // Set the file paths
    SetCostModelFiles(
        io_file, 
        layer_file, 
        wafer_process_file, 
        assembly_process_file, 
        test_file, 
        netlist_file, 
        blocks_file);
    
    // Initialize the cost model
    bool success = InitializeCostModel();
    if (success) {
    } else {
      std::cerr << "[WARNING] Failed to initialize cost model in constructor" << std::endl;
    }
  }
}

void ChipletRefiner::SetMaxMove(const int max_move) { max_move_ = max_move; }

void ChipletRefiner::SetRefineIters(const int refiner_iters) {
  refiner_iters_ = refiner_iters;
}

void ChipletRefiner::BuildChiplets(const HGraphPtr &hgraph) {
  // Clear existing data structures, but keep allocated capacity where possible
  bundled_nets_.clear();
  chiplets_.clear();

  // Pre-allocate space to avoid reallocations
  const int num_edges = hgraph->GetNumHyperedges();
  const int num_vertices = hgraph->GetNumVertices();
  
  bundled_nets_.reserve(num_edges);
  chiplets_.reserve(num_vertices);
  
  // Bundle all nets between the same pair of chiplets that share a reach:
  // their IO cells sit on the same shoreline, so the IO areas add up (the
  // shoreline term of the reach check needs the total), and the SA has one
  // net per pair instead of hundreds of parallel ones.
  std::map<std::tuple<int, int, float>, int> bundle_index;
  for (int i = 0; i < num_edges; ++i) {
    const float weight = hgraph->GetHyperedgeWeights(i)[0];
    const auto& vertices = hgraph->Vertices(i);
    if (vertices.size() < 2) {
      continue;
    }
    int term_a = *vertices.begin();
    int term_b = *(++vertices.begin());
    if (term_a == term_b) {
      continue;
    }
    if (term_a > term_b) std::swap(term_a, term_b);
    const float reach = hgraph->GetReach(i);
    const float io_area = hgraph->GetIoSize(i);
    const auto key = std::make_tuple(term_a, term_b, reach);
    auto it = bundle_index.find(key);
    if (it == bundle_index.end()) {
      bundle_index.emplace(key, static_cast<int>(bundled_nets_.size()));
      bundled_nets_.emplace_back(std::pair<int, int>(term_a, term_b), weight, reach, io_area);
    } else {
      bundled_nets_[it->second].weight += weight;
      bundled_nets_[it->second].io_area += io_area;
    }
  }

  // Cache the halo width to avoid multiple lookups
  const float halo_width = separation_;

  // Process all vertices to create chiplets
  for (int i = 0; i < num_vertices; ++i) {
    // Initial positioning
    constexpr float x = 0.0f;
    constexpr float y = 0.0f;
    
    // Get vertex weight and calculate area
    const auto& vertex_weight = hgraph->GetVertexWeights(i);
    
    // Cache total area calculation
    const float area = std::accumulate(vertex_weight.begin(), vertex_weight.end(), 0.0f);
    
    // Calculate dimensions based on area (square aspect ratio initialization)
    const float width = std::sqrt(area);
    const float height = area / width;
    
    // Create chiplet with emplace_back to avoid copies
    chiplets_.emplace_back(x, y, width, height, area, halo_width);
  }
}

// Run SA for floorplanning
// return a tuple of <vector,vector, bool>
std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, bool>
ChipletRefiner::Floorplanner(int max_steps, int perturbations,
                             float cooling_acceleration_factor, bool local) {
  // Start timing for performance tracking
  auto start_time = std::chrono::high_resolution_clock::now();
  
  try {
  // Initialize with either local or global sequences
  std::vector<int> pos_seq, neg_seq;
  if (local) {
    pos_seq = local_pos_seq_;
    neg_seq = local_neg_seq_;
  } else {
    pos_seq = global_pos_seq_;
    neg_seq = global_neg_seq_;
    }
    
    
    // Generate default sequences if they're empty
    if (pos_seq.empty() || neg_seq.empty()) {
      if (!chiplet_graph_) {
        // Create a dummy floorplan result when the graph is null
        std::cout << "[ERROR] chiplet_graph_ is null, returning dummy result" << std::endl;
        std::vector<float> dummy_result(1, 1.0);
        return std::make_tuple(dummy_result, dummy_result, dummy_result, true);
      }
      
      int chip_count = chiplet_graph_->GetNumVertices();
      
      // Initialize with identity sequences
      pos_seq.resize(chip_count);
      neg_seq.resize(chip_count);
      std::iota(pos_seq.begin(), pos_seq.end(), 0);
      std::iota(neg_seq.begin(), neg_seq.end(), 0);
    }
    
    // Build the chiplets from the hypergraph if needed
    if (chiplets_.empty() && chiplet_graph_) {
      BuildChiplets(chiplet_graph_);
    }
    
    // Check if we have valid chiplets
    if (chiplets_.empty()) {
      std::cout << "[ERROR] No chiplets available for floorplanning" << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Check sequence lengths against chiplet count
    if (pos_seq.size() != chiplets_.size() || neg_seq.size() != chiplets_.size()) {
                
      pos_seq.resize(chiplets_.size());
      neg_seq.resize(chiplets_.size());
      std::iota(pos_seq.begin(), pos_seq.end(), 0);
      std::iota(neg_seq.begin(), neg_seq.end(), 0);
    }
    // Feasibility-aware refinement carries the best arrangement it found for
    // this partition (and evaluation mode may load the arrangement saved
    // with a partition); start the walkers there so a feasible arrangement
    // known to the refiner is never lost by the floorplanner.
    const bool seeded_from_probe =
        fp_warm_.pos_seq.size() == chiplets_.size() &&
        fp_warm_.neg_seq.size() == chiplets_.size();
    if (seeded_from_probe) {
      pos_seq = fp_warm_.pos_seq;
      neg_seq = fp_warm_.neg_seq;
    }
    
    // Number of GWTW workers (independent SA instances). Each worker gets a
    // share of the step budget, so the total work is roughly independent of
    // the worker count.
    const int num_workers = (num_fp_workers_ > 0)
                                ? num_fp_workers_
                                : std::max(2, std::min(num_threads_, 4));
    
    // Create vector of worker instances
    std::vector<std::unique_ptr<SACore>> workers;
    workers.reserve(num_workers);
    
    // Initialize workers with simple cooling rates
    float delta_cooling_rate = (max_cooling_rate_ - min_cooling_rate_) / (num_workers > 1 ? num_workers - 1 : 1);
    
    // Every walker runs the full schedule (max_steps x perturbations
    // perturbations); walkers run in parallel, so more walkers means more
    // search, not less per walker.
    const int per_worker_steps = std::max(1, max_steps);
    const int per_worker_perturbations = std::max(1, perturbations);
    
    for (int worker_id = 0; worker_id < num_workers; worker_id++) {
      // Simple linear distribution of cooling rates
      float worker_cooling_rate = min_cooling_rate_ + worker_id * delta_cooling_rate;
      worker_cooling_rate = std::clamp(worker_cooling_rate, min_cooling_rate_, max_cooling_rate_);
      
      try {
       
        // Create worker with unique ID and parameters
        auto sa = std::make_unique<SACore>(
            worker_id,                        // worker_id
            chiplets_,
            bundled_nets_,
            area_penalty_weight_,
            package_penalty_weight_,
            net_penalty_weight_,
            pos_swap_prob_,
            neg_swap_prob_,
            double_swap_prob_,
            resize_prob_,
            expand_prob_,
            per_worker_steps,                // max_num_step - use smaller steps for each worker
            per_worker_perturbations,        // num_perturb_per_step - reduce perturbations
            worker_cooling_rate,             // cooling_rate
            init_seed_ + worker_id);         // seed
              
                  
        sa->setPosSeq(pos_seq);
        sa->setNegSeq(neg_seq);
        if (seeded_from_probe) {
          sa->applySeed(fp_warm_);  // sequence pair and shapes
        }
          
        // Add to workers vector
        workers.push_back(std::move(sa));
      } catch (const std::bad_alloc& e) {
        std::cerr << "[ERROR] Memory allocation failed creating worker " << worker_id 
                  << ": " << e.what() << std::endl;
        
        // If we have at least one worker, proceed with what we have
        if (!workers.empty()) {
          std::cerr << "[ERROR] Continuing with " << workers.size() 
                    << " workers due to memory limitations" << std::endl;
          break;
        } else {
          // Rethrow if we couldn't create any workers
          throw;
        }
      } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception creating worker " << worker_id << ": " 
                  << e.what() << std::endl;
        // Continue with other workers
      }
    }
    
    if (workers.empty()) {
      std::cerr << "[ERROR] Failed to create any workers" << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    try {
      // Initialize normalization values from first worker
      workers[0]->initialize();
      float norm_area_penalty = workers[0]->getNormAreaPenalty();
      float norm_package_penalty = workers[0]->getNormPackagePenalty();
      float norm_net_penalty = workers[0]->getNormNetPenalty();
      
      // Apply normalization to all workers
      for (auto& worker : workers) {
        worker->setNormAreaPenalty(norm_area_penalty);
        worker->setNormPackagePenalty(norm_package_penalty);
        worker->setNormNetPenalty(norm_net_penalty);
        if (seeded_from_probe) {
          worker->evaluateCurrent();  // remember the seed state if it is feasible
        }
      }
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] Exception during worker initialization: " << e.what() << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Go-with-the-winners (GWTW) annealing.
    //
    // The per-worker schedule of `per_worker_steps` is split into
    // `gwtw_iter_` segments. After every segment the workers are ranked
    // (valid solutions first, then by normalised cost); the top
    // `gwtw_top_k_` solutions are copied onto the remaining workers in the
    // proportions given by `gwtw_top_k_ratio_`, and every worker continues
    // from its own (derated) temperature with its own RNG. Workers are
    // independent objects, so a segment can run them in parallel.
    const int num_rounds = std::max(1, gwtw_iter_);
    const int steps_per_round =
        std::max(1, (per_worker_steps + num_rounds - 1) / num_rounds);

    // Cost-model scoring of feasible arrangements. The SA objective (area,
    // package, reach penalty) is only a proxy: the design cost depends on
    // the chiplet shapes through dies-per-wafer, reticle fit and the IO
    // shoreline, and two feasible floorplans of the same partition can
    // differ by a few percent in cost. When RunFloorplanner() gave us the
    // partition, feasible walkers are ranked by the real cost instead.
    int num_parts_in_partition = 0;
    for (int p : fp_cost_partition_) num_parts_in_partition = std::max(num_parts_in_partition, p + 1);
    const bool cost_aware = fp_cost_aware_ && cost_model_initialized_ &&
        libraryDicts_ != nullptr &&
        num_parts_in_partition == static_cast<int>(chiplets_.size());
    auto cost_fn = [&](const std::vector<float>& ratios) {
      return CostWithAspectRatios(fp_cost_partition_, ratios);
    };
    std::vector<float> ratios_scratch;
    auto worker_cost = [&](SACore& w) {
      if (!cost_aware) return w.getCost();
      w.getAspectRatios(ratios_scratch);
      return cost_fn(ratios_scratch);
    };
    fp_run_stats_ = FloorplanRunStats();
    fp_run_stats_.walkers = static_cast<int>(workers.size());

    // Rank workers: valid first, then lower cost. Returns indices.
    auto rank_workers = [&](std::vector<int>& order) {
      order.resize(workers.size());
      std::iota(order.begin(), order.end(), 0);
      std::vector<std::pair<bool, float>> key(workers.size());
      for (size_t w = 0; w < workers.size(); ++w) {
        if (!workers[w]->isValid()) {
          workers[w]->restoreValid();  // rank by the best feasible state seen
        }
        const bool valid = workers[w]->isValid();
        key[w] = {valid, valid ? worker_cost(*workers[w]) : workers[w]->getCost()};
      }
      std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        if (key[a].first != key[b].first) return key[a].first;  // valid first
        return key[a].second < key[b].second;
      });
    };

    for (int round = 0; round < num_rounds; ++round) {
      const int steps_this_round =
          std::min(steps_per_round, per_worker_steps - round * steps_per_round);
      if (steps_this_round <= 0) break;

      #if HAVE_OPENMP
      #pragma omp parallel for schedule(static) if(workers.size() > 1)
      #endif
      for (int w = 0; w < static_cast<int>(workers.size()); ++w) {
        try {
          workers[w]->runSteps(steps_this_round);
        } catch (const std::exception& e) {
          #if HAVE_OPENMP
          #pragma omp critical(fp_worker_error)
          #endif
          std::cerr << "[ERROR] Exception running worker " << w << ": "
                    << e.what() << std::endl;
        }
      }

      if (round == num_rounds - 1 || workers.size() < 2) {
        continue;  // no synchronisation after the last segment
      }

      // Select winners and redistribute them.
      std::vector<int> order;
      rank_workers(order);
      const int top_k = std::max(1, std::min<int>(gwtw_top_k_, workers.size()));

      // Cumulative share of the population each winner receives.
      std::vector<float> ratio(gwtw_top_k_ratio_.begin(), gwtw_top_k_ratio_.end());
      ratio.resize(top_k, 0.0f);
      float ratio_sum = std::accumulate(ratio.begin(), ratio.end(), 0.0f);
      if (ratio_sum <= 0.0f) {
        std::fill(ratio.begin(), ratio.end(), 1.0f / top_k);
        ratio_sum = 1.0f;
      }

      // Assign each non-winning worker to a winner proportionally.
      const int num_followers = static_cast<int>(workers.size()) - top_k;
      std::vector<int> winner_of_follower(num_followers, 0);
      {
        float acc = 0.0f;
        int next = 0;
        for (int k = 0; k < top_k; ++k) {
          acc += ratio[k] / ratio_sum;
          const int upto = (k == top_k - 1)
                               ? num_followers
                               : static_cast<int>(std::round(acc * num_followers));
          for (; next < upto && next < num_followers; ++next) {
            winner_of_follower[next] = k;
          }
        }
      }
      for (int f = 0; f < num_followers; ++f) {
        SACore* follower = workers[order[top_k + f]].get();
        const SACore* winner = workers[order[winner_of_follower[f]]].get();
        follower->adoptSolution(*winner);
      }

      // Derate the temperature for the next segment (1.0 = unchanged).
      if (gwtw_temp_derate_factor_ > 0.0f && gwtw_temp_derate_factor_ != 1.0f) {
        for (auto& worker : workers) {
          worker->setTemp(std::max(worker->getTemp() * gwtw_temp_derate_factor_,
                                   gwtw_min_temp_));
        }
      }
    }
    
    // A walker may have passed through a feasible state and drifted out of
    // it again (the SA cost blends area and package with the reach
    // penalty); bring such walkers back to their last feasible state.
    for (auto& worker : workers) {
      if (!worker->isValid()) {
        worker->restoreValid();
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
        
        // Always prefer valid solutions
    if ((current_valid && !is_valid) || 
        (current_valid == is_valid && current_cost < best_cost)) {
          best_sa = worker.get();
      best_cost = current_cost;
      is_valid = current_valid;
      
        }
      } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception evaluating worker " << worker->getWorkerId() 
                  << ": " << e.what() << std::endl;
        // Continue with other workers
      }
    }

    // Cost-aware selection among the feasible walkers: refine the chiplet
    // shapes of each distinct feasible arrangement against the cost model
    // (sequence pair fixed, reach constraint enforced) and return the
    // cheapest. The SA pick above is the fallback when nothing is feasible.
    if (cost_aware && is_valid && best_sa) {
      fp_run_stats_.sa_pick_cost = worker_cost(*best_sa);
      // width/height grid over the allowed range, log-spaced, plus a finer
      // band around 1 (most chiplets end up near square).
      std::vector<float> candidates;
      for (int k = 0; k <= 24; ++k) {
        candidates.push_back(std::exp(std::log(0.2f) + k * (std::log(5.0f) - std::log(0.2f)) / 24.0f));
      }
      for (int k = -6; k <= 6; ++k) {
        candidates.push_back(std::exp(k * 0.05f));
      }
      std::sort(candidates.begin(), candidates.end());
      constexpr int kShapeRounds = 4;

      struct Seen { std::vector<int> pos, neg; std::vector<float> ratios; };
      std::vector<Seen> seen;
      SACore* best_shaped = nullptr;
      float best_shaped_cost = std::numeric_limits<float>::max();
      for (auto& worker : workers) {
        if (!worker->isValid()) continue;
        ++fp_run_stats_.feasible;
        Seen s;
        worker->getPosSeq(s.pos);
        worker->getNegSeq(s.neg);
        worker->getAspectRatios(s.ratios);
        bool duplicate = false;
        for (const auto& t : seen) {
          if (t.pos != s.pos || t.neg != s.neg) continue;
          bool same = true;
          for (size_t i = 0; i < s.ratios.size() && same; ++i) {
            same = std::fabs(t.ratios[i] - s.ratios[i]) <= 1e-4f * std::max(1.0f, t.ratios[i]);
          }
          if (same) { duplicate = true; break; }
        }
        if (duplicate) continue;
        seen.push_back(std::move(s));
        const float c = worker->refineShapes(cost_fn, candidates, kShapeRounds,
                                             &fp_run_stats_.shape_evals);
        if (worker->isValid() && c < best_shaped_cost) {
          best_shaped_cost = c;
          best_shaped = worker.get();
        }
      }
      if (best_shaped) {
        best_sa = best_shaped;
        best_cost = best_shaped_cost;
      }
      fp_run_stats_.best_cost = best_shaped ? best_shaped_cost : fp_run_stats_.sa_pick_cost;
      if (std::getenv("CHIPLETPART_FP_DEBUG")) {
        std::cerr << "[fp-debug] floorplan: walkers=" << fp_run_stats_.walkers
                  << " feasible=" << fp_run_stats_.feasible
                  << " distinct=" << seen.size()
                  << " sa_pick_cost=" << fp_run_stats_.sa_pick_cost
                  << " best_cost=" << fp_run_stats_.best_cost
                  << " shape_evals=" << fp_run_stats_.shape_evals << std::endl;
      }
    }
    
    // If no valid solution found, use the best invalid one
    if (best_sa == nullptr && !workers.empty()) {
      best_sa = workers[0].get();
      is_valid = false;
    }
    
    // Extract results
    std::vector<Chiplet> best_chiplets;
    if (best_sa) {
      try {
        if (!is_valid && floorplan_verbose_) {
          std::cerr << "[FLOORPLAN] best worker " << best_sa->getWorkerId()
                    << " is infeasible:" << std::endl;
          best_sa->reportViolations(std::cerr, 5);
        }
  best_sa->getMacros(best_chiplets);
        
        // Store sequence pairs
        std::vector<int> final_pos_seq, final_neg_seq;
        best_sa->getPosSeq(final_pos_seq);
        best_sa->getNegSeq(final_neg_seq);
        fp_last_result_.pos_seq = final_pos_seq;
        fp_last_result_.neg_seq = final_neg_seq;
        fp_last_result_.aspect_ratios.clear();
        for (const auto& chiplet : best_chiplets) {
          fp_last_result_.aspect_ratios.push_back(
              chiplet.getRealWidth() > 0.0f
                  ? chiplet.getRealHeight() / chiplet.getRealWidth() : 1.0f);
        }
        
  if (local) {
          local_pos_seq_ = final_pos_seq;
          local_neg_seq_ = final_neg_seq;
  } else {
          global_pos_seq_ = final_pos_seq;
          global_neg_seq_ = final_neg_seq;
        }
      } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception extracting best solution: " << e.what() << std::endl;
        std::vector<float> dummy_result(1, 1.0);
        return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
      }
    } else {
      // Return empty results if no solution found
      std::cout << "[ERROR] No solution found, returning dummy result" << std::endl;
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
    
    
    // Calculate and record performance metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    total_fplan_time_ += duration.count() / 1000.0f;
    
  return std::make_tuple(aspect_ratios, x_locations, y_locations, is_valid);
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] Exception in Floorplanner: " << e.what() << std::endl;
    return std::make_tuple(std::vector<float>(), std::vector<float>(), std::vector<float>(), false);
  } catch (...) {
    std::cerr << "[ERROR] Unknown exception in Floorplanner" << std::endl;
    return std::make_tuple(std::vector<float>(), std::vector<float>(), std::vector<float>(), false);
  }
}

// Short single-walker floorplan probe of `partition`; returns the total reach
// violation of the best state seen (0 = feasible). Warm-started from the last
// feasible probe with the same chiplet count. Rebuilds chiplet_graph_ /
// chiplets_ / bundled_nets_ as a side effect (like RunFloorplanner).
float ChipletRefiner::FloorplanViolation(const HGraphPtr &hgraph,
                                         const std::vector<int> &partition) {
  chiplet_graph_ = GenerateNetlist(hgraph, partition);
  if (!chiplet_graph_) {
    return std::numeric_limits<float>::max();
  }
  BuildChiplets(chiplet_graph_);
  const int n = static_cast<int>(chiplets_.size());
  if (static_cast<int>(fp_warm_.pos_seq.size()) != n ||
      static_cast<int>(fp_warm_.neg_seq.size()) != n) {
    fp_warm_.clear();
    fp_warm_.pos_seq.resize(n);
    fp_warm_.neg_seq.resize(n);
    std::iota(fp_warm_.pos_seq.begin(), fp_warm_.pos_seq.end(), 0);
    std::iota(fp_warm_.neg_seq.begin(), fp_warm_.neg_seq.end(), 0);
  }
  if (n <= 1 || bundled_nets_.empty()) {
    last_probe_violation_ = 0.0f;
    last_probe_ = fp_warm_;
    return 0.0f;  // a single chiplet (or no inter-chiplet nets) is trivially feasible
  }
  ++fp_probe_count_;
  ++fp_stats_.probes;
  SACore sa(0, chiplets_, bundled_nets_, area_penalty_weight_, package_penalty_weight_,
            net_penalty_weight_, pos_swap_prob_, neg_swap_prob_, double_swap_prob_,
            resize_prob_, expand_prob_, fp_check_steps_, fp_check_perturbs_,
            max_cooling_rate_, init_seed_ + fp_probe_count_);
  // Warm start from the best arrangement seen so far (the partition changes
  // by one vertex per move, so the previous floorplan is a good seed). Then:
  // greedy descent on the violation, a short annealing walk for diversity,
  // and a final descent from the best state of the walk.
  sa.applySeed(fp_warm_);
  sa.evaluateCurrent();
  const int quench_budget = fp_check_steps_ * fp_check_perturbs_;
  if (!sa.hasSeenValid()) {
    sa.quench(quench_budget);
  }
  if (!sa.hasSeenValid()) {
    sa.runSteps(fp_check_steps_);
    sa.restoreMin();
    sa.quench(quench_budget);
  }
  // The warm start is only advanced when the probed partition is adopted
  // (CommitFloorplanProbe); probes of rejected moves must not disturb it, or
  // re-measuring the same partition would drift.
  sa.getMinSeed(last_probe_);
  last_probe_violation_ = sa.hasSeenValid() ? 0.0f : sa.getMinNetPenaltySeen();
  return last_probe_violation_;
}

void ChipletRefiner::SetFloorplanBaseline(const HGraphPtr &hgraph,
                                          const std::vector<int> &partition) {
  if (!floorplan_aware_) {
    current_violation_ = 0.0f;
    return;
  }
  FloorplanViolation(hgraph, partition);
  CommitFloorplanProbe();
}

bool ChipletRefiner::IsFloorplanAcceptable(const HGraphPtr &hgraph,
                                           const std::vector<int> &partition) {
  if (!floorplan_aware_) {
    return true;
  }
  const float v = FloorplanViolation(hgraph, partition);
  const float tol = 1e-4f * std::max(1.0f, current_violation_);
  ++fp_stats_.gated;
  if (v <= current_violation_ + tol) {
    return true;
  }
  ++fp_stats_.rejected;
  AuditFloorplanRejection(hgraph, partition);
  return false;
}

void ChipletRefiner::AuditFloorplanRejection(const HGraphPtr &hgraph,
                                             const std::vector<int> &partition) {
  if (fp_audit_factor_ <= 1) {
    return;
  }
  const FloorplanSeed saved_probe = last_probe_;
  const float saved_violation = last_probe_violation_;
  const int saved_count = fp_probe_count_;
  const int saved_steps = fp_check_steps_;
  const int saved_perturbs = fp_check_perturbs_;
  const int saved_probes = fp_stats_.probes;
  const float scale = std::sqrt(static_cast<float>(fp_audit_factor_));
  fp_check_steps_ = std::max(1, static_cast<int>(std::lround(saved_steps * scale)));
  fp_check_perturbs_ = std::max(1, static_cast<int>(std::lround(saved_perturbs * scale)));
  fp_probe_count_ += 1000000;  // keep the audit's RNG stream apart from the search's
  const float v = FloorplanViolation(hgraph, partition);
  ++fp_stats_.audit_checked;
  if (v <= current_violation_ + 1e-4f * std::max(1.0f, current_violation_)) {
    ++fp_stats_.audit_false;
  }
  fp_check_steps_ = saved_steps;
  fp_check_perturbs_ = saved_perturbs;
  fp_probe_count_ = saved_count;
  fp_stats_.probes = saved_probes;
  last_probe_ = saved_probe;
  last_probe_violation_ = saved_violation;
}

// The main function of refinement class
void ChipletRefiner::Refine(const HGraphPtr &hgraph,
                            const Matrix<float> &upper_block_balance,
                            const Matrix<float> &lower_block_balance,
                            Partition &solution) {
  // floorplanner_ = false;
  
  // Initialize cost model if not already done and files are set
  if (!cost_model_initialized_ && !io_file_.empty()) {
    bool success = InitializeCostModel();
    if (!success) {
      std::cerr << "[WARNING] Failed to initialize cost model in Refine method" << std::endl;
    }
  }

  // Seed legacy_cost_ (the baseline every move gain is measured against).
  // Use the member GetCostFromScratch with approx_state=true: it is the SAME
  // evaluation GetSingleMoveCost uses for the "after" cost, and it fixes the
  // tech/aspect-ratio/location vectors to the partition count. Calling the raw
  // evaluator with the unadjusted member vectors (as before) made the model
  // build fail and returned FLT_MAX, so the first move of every pass had gain
  // ~FLT_MAX and was accepted unconditionally, after which legacy_cost_
  // collapsed to 0 and all other gains were negative -> each pass applied one
  // arbitrary move.
  // (Seeding happens inside the pass loop, right before each Pass(), so that
  // it also reflects any aspect-ratio/location update made by the floorplanner
  // and the roll-back performed by the previous pass.)
  
  if (max_move_ <= 0) {
    return;
  }
  // calculate the basic statistics of current solution
  Matrix<float> cur_block_balance = GetBlockBalance(hgraph, solution);
  Matrix<int> net_degs = GetNetDegrees(hgraph, solution);
  // Feasibility-aware refinement may need more passes than cost-only
  // refinement to drive the reach violation to zero; allow extra passes
  // while the violation is still improving.
  const int max_iters = floorplan_aware_ ? refiner_iters_ + kExtraRepairPasses : refiner_iters_;
  fp_repair_mode_ = false;
  for (int i = 0; i < max_iters; ++i) {
    // the main function for improving the solution
    // mark the vertices can be moved as unvisited
    std::vector<bool> visited_vertices_flag(hgraph->GetNumVertices(), false);
    if (floorplanner_ == true) {
      try {
        
        // Ensure we have a valid hypergraph for chiplet creation
        if (!chiplet_graph_) {
          chiplet_graph_ = GenerateNetlist(hgraph, solution);
        }
        
        // Build chiplets if needed
        if (chiplets_.empty()) {
          BuildChiplets(chiplet_graph_);
        }
        
        // Ensure we have valid chiplets
        if (!chiplets_.empty()) {
          // Initialize global sequences if they're empty
          if (global_pos_seq_.empty() || global_neg_seq_.empty()) {
            const int chiplet_count = chiplets_.size();
            
            // Create default sequential ordering
            global_pos_seq_.resize(chiplet_count);
            global_neg_seq_.resize(chiplet_count);
            std::iota(global_pos_seq_.begin(), global_pos_seq_.end(), 0);
            std::iota(global_neg_seq_.begin(), global_neg_seq_.end(), 0);
          }
          
          // Copy to local sequences
      local_pos_seq_ = global_pos_seq_;
      local_neg_seq_ = global_neg_seq_;
          
          
          // Now call the floorplanner with properly initialized sequences
          // Use a smaller number of steps and perturbations for speed
          auto fp_tuple = Floorplanner(200, 50, 1.0);
          
          bool success = std::get<3>(fp_tuple);
          
          // Keep sequences for next iteration
        } 
      } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception in Refine floorplanner: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "[ERROR] Unknown exception in Refine floorplanner" << std::endl;
      }
    }
    auto start_time = std::chrono::high_resolution_clock::now();
    if (cost_model_initialized_ && libraryDicts_ != nullptr) {
      legacy_cost_ = GetCostFromScratch(solution, true);
    }
    const float violation_before = current_violation_;
    const float gain =
        Pass(hgraph, upper_block_balance, lower_block_balance,
             cur_block_balance, net_degs, solution, visited_vertices_flag);
    // Switch to violation-guided moves once gain-ordered passes stop
    // reducing the violation (or the planned passes are used up).
    if (floorplan_aware_ && current_violation_ > 1e-4f &&
        (current_violation_ > 0.9f * violation_before || i >= refiner_iters_ - 1)) {
      fp_repair_mode_ = true;
    }
    
    // Only clear sequences if we're done with refinement or no longer need them
    if (floorplanner_ == true && (gain <= 0.0 || i == max_iters - 1)) {
      ClearLocalSequences();
      ClearGlobalSequences();
    }
    
    if (gain <= 0.0) {
      if (floorplan_aware_ && current_violation_ > 1e-4f && !fp_repair_mode_) {
        fp_repair_mode_ = true;  // one more try, violation-guided
        continue;
      }
      return; // stop if there is no improvement
    }
    if (i >= refiner_iters_ - 1 && floorplan_aware_ &&
        !(current_violation_ > 1e-4f && current_violation_ < violation_before)) {
      return; // extra passes only while still repairing
    }
  }
}

// Main FM pass function - the core of the refinement algorithm
float ChipletRefiner::Pass(
    const HGraphPtr &hgraph, 
    const Matrix<float> &upper_block_balance,
    const Matrix<float> &lower_block_balance,
    Matrix<float> &block_balance,  // Current block balance, modified during the pass
    Matrix<int> &net_degs,         // Current net degree, modified during the pass
    Partition &solution,           // Current partitioning solution
    std::vector<bool> &visited_vertices_flag) {
  
  
  // Calculate the number of expected moves to pre-allocate data structures
  const int num_vertices = hgraph->GetNumVertices();
  const int expected_moves = std::min(max_move_, num_vertices / 10);
  
  // Initialize gain buckets with capacity pre-allocation
  GainBuckets buckets;
  buckets.reserve(num_parts_);
  
  for (int i = 0; i < num_parts_; ++i) {
    // Create priority queue for each destination partition
    auto bucket = std::make_shared<PriorityQueue>(
        num_vertices, total_corking_passes_, hgraph);
    buckets.push_back(bucket);
  }
  
  // Find boundary vertices once and reuse
  std::vector<int> boundary_vertices; 
  if (boundary_flag_ == true) {
    boundary_vertices = FindBoundaryVertices(
        hgraph, net_degs, visited_vertices_flag, random_non_boundary_ratio_);
  } else {
    // Use all vertices if no boundary vertices found
    boundary_vertices.resize(num_vertices);
    std::iota(boundary_vertices.begin(), boundary_vertices.end(), 0);
  }
  
  // Allocate working memory for gain updates
  std::vector<int> neighbors;
  neighbors.reserve(boundary_vertices.size() / 2);
  
  // Pre-allocate thread-local storage for parallel processing
  #if HAVE_OPENMP
  // Create a pool of temporary vectors for thread use
  const int max_threads = omp_get_max_threads();
  std::vector<std::vector<int>> thread_local_vertices(max_threads);
  for (int i = 0; i < max_threads; ++i) {
    thread_local_vertices[i].reserve(boundary_vertices.size() / 4);
  }
  #endif
  
  // Initialize gain buckets
  InitializeGainBucketsKWay(buckets, hgraph, boundary_vertices, net_degs, solution);

  // Allocate move history with capacity pre-reservation
  std::vector<GainCell> moves_trace;
  moves_trace.reserve(expected_moves);
  
  float total_delta_gain = 0.0f;
  float best_gain = 0.0f;
  int best_move_index = -1;

  // Scratch for the neighbourhood computation
  std::vector<char> touched(num_vertices, 0);
  const bool lazy_reeval = cost_model_initialized_ && libraryDicts_ != nullptr;
  // Bound on re-evaluations per accepted move (each one costs a model build)
  const int max_reevals_per_move = std::max(4, 2 * num_parts_);
  // Floorplan-aware mode: baseline violation of the starting partition and a
  // bound on the number of candidates rejected for feasibility per move.
  if (floorplan_aware_) {
    SetFloorplanBaseline(hgraph, solution);
  }
  const int max_fp_rejects_per_move = std::max(8, 4 * num_parts_);
  const int max_fp_repair_scan = std::max(16, 8 * num_parts_);
  int fp_rejected_total = 0;
  // Arrangement (sequence pair) after each accepted move, so the rollback to
  // the best prefix can restore the matching warm start.
  std::vector<FloorplanSeed> move_seeds;
  const FloorplanSeed start_seed = fp_warm_;
  // The best prefix is chosen lexicographically:
  // lower violation first, then higher cumulative gain, so an infeasible
  // start is repaired even at a cost increase (feasibility is a hard
  // constraint of the design; cost is optimised among feasible designs).
  const float start_violation = current_violation_;
  float best_violation = start_violation;
  const auto violation_lt = [](float a, float b) {
    return a < b - 1e-4f * std::max(1.0f, b);
  };

  // Main FM pass loop - execute up to max_move_ vertex moves
  for (int move_count = 0; move_count < max_move_; ++move_count) {
    // Pick the best move using the optimized gain buckets. Gains of vertices
    // outside the last move's neighbourhood may be stale (see below), so the
    // picked move is re-evaluated against the current solution and, if its
    // gain changed, re-inserted and the pick repeated ("lazy greedy"). Every
    // accepted move therefore carries an exact gain.
    std::shared_ptr<VertexGain> candidate;
    int reevals = 0;
    float candidate_violation = current_violation_;
    // Feasibility gate (floorplan-aware mode). Every candidate is probed;
    // whether the probe may veto it depends on the regime:
    //  - feasible start: a move must keep the violation at zero;
    //  - infeasible start: moves follow the cost gains (they usually reduce
    //    the violation as well, and the prefix selection below prefers the
    //    lowest violation);
    //  - infeasible start that the previous pass could not improve
    //    (fp_repair_mode_): a move must not increase the violation.
    // When the gate vetoes, candidates are scanned in gain order; if none of
    // the first `max_scan` qualifies the lowest-violation one is taken anyway
    // (the rollback to the best prefix makes this safe, and it lets the pass
    // climb out of a local minimum). Probed-but-not-taken candidates go back
    // into their buckets.
    const bool infeasible_start = floorplan_aware_ && current_violation_ > 1e-4f;
    const bool repair_mode = infeasible_start && fp_repair_mode_;
    const bool gate_moves = floorplan_aware_ && (!infeasible_start || repair_mode);
    const int max_scan = repair_mode ? max_fp_repair_scan : max_fp_rejects_per_move;
    struct ScannedMove {
      std::shared_ptr<VertexGain> cell;
      float violation;
      FloorplanSeed seed;
    };
    FloorplanSeed candidate_seed = fp_warm_;
    std::vector<ScannedMove> scanned;
    bool candidate_acceptable = false;
    while (true) {
      candidate = PickMoveKWay(
          buckets, hgraph, block_balance, 
          upper_block_balance, lower_block_balance, solution);
      if (candidate->GetVertex() < 0) {
        break;
      }
      const int v = candidate->GetVertex();
      float exact_gain = candidate->GetGain();
      bool gain_is_exact = true;
      if (lazy_reeval && reevals < max_reevals_per_move) {
        exact_gain = GetSingleMoveCost(
            solution, v, candidate->GetSourcePart(), candidate->GetDestinationPart());
        const float stale_gain = candidate->GetGain();
        const float tol = 1e-5f * std::max(1.0f, std::fabs(stale_gain));
        gain_is_exact = std::fabs(exact_gain - stale_gain) <= tol;
      }
      if (gain_is_exact) {
        if (!floorplan_aware_) {
          break;
        }
        const int from = candidate->GetSourcePart();
        const int to = candidate->GetDestinationPart();
        solution[v] = to;
        const float viol = FloorplanViolation(hgraph, solution);
        solution[v] = from;
        const float tol = 1e-4f * std::max(1.0f, current_violation_);
        if (gate_moves) {
          ++fp_stats_.gated;
        }
        if (!gate_moves || viol <= current_violation_ + tol) {
          candidate_violation = viol;
          candidate_seed = last_probe_;
          candidate_acceptable = true;
          break;
        }
        scanned.push_back({candidate, viol, last_probe_});
        buckets[to]->Remove(v);
        ++fp_stats_.rejected;
        if (fp_audit_factor_ > 1) {
          solution[v] = to;
          AuditFloorplanRejection(hgraph, solution);
          solution[v] = from;
        }
        if (static_cast<int>(scanned.size()) < max_scan) {
          continue;
        }
        candidate = std::make_shared<VertexGain>();
        break;
      }
      // `candidate` is the heap's own element; ChangePriority compares the
      // new gain against the stored one, so a fresh cell is required (an
      // in-place SetGain would leave the heap unordered).
      auto refreshed = std::make_shared<VertexGain>(
          v, candidate->GetSourcePart(), candidate->GetDestinationPart(),
          exact_gain);
      buckets[candidate->GetDestinationPart()]->ChangePriority(v, refreshed);
      ++reevals;
    }
    
    if (!scanned.empty()) {
      size_t best = scanned.size();  // none
      if (!candidate_acceptable) {
        // Lowest violation wins; ties go to the earlier (higher-gain) entry.
        best = 0;
        for (size_t k = 1; k < scanned.size(); ++k) {
          if (scanned[k].violation < scanned[best].violation -
                  1e-4f * std::max(1.0f, scanned[best].violation)) {
            best = k;
          }
        }
        candidate = scanned[best].cell;
        candidate_violation = scanned[best].violation;
        candidate_seed = scanned[best].seed;
        fp_rejected_total += static_cast<int>(scanned.size());
      }
      for (size_t k = 0; k < scanned.size(); ++k) {
        if (k == best) {
          continue;
        }
        const auto& cell = scanned[k].cell;
        buckets[cell->GetDestinationPart()]->InsertIntoPQ(std::make_shared<VertexGain>(
            cell->GetVertex(), cell->GetSourcePart(), cell->GetDestinationPart(),
            cell->GetGain()));
      }
    }

    // Check if there are any more valid moves
    const int vertex = candidate->GetVertex();
    if (vertex < 0) {
      break;  // No more valid candidates
    }
    
    // Get source partition before move
    const int from_part = candidate->GetSourcePart();
    // Get destination partition after the move
    const int to_part = candidate->GetDestinationPart();
    // Update cost tracking
    legacy_cost_ -= candidate->GetGain();
    if (floorplan_aware_) {
      current_violation_ = candidate_violation;
      fp_warm_ = std::move(candidate_seed);
      move_seeds.push_back(fp_warm_);
    }
    
    // Accept the move (updates all needed data structures)
    AcceptKWayMove(
        candidate, buckets, moves_trace, total_delta_gain,
        visited_vertices_flag, hgraph, block_balance, net_degs, solution);
     
    // Vertices whose gains are refreshed eagerly: those sharing a hyperedge
    // with the moved vertex and those in the source/destination partitions
    // (their chiplet areas changed). Other boundary vertices keep their
    // previous gains until they are picked and lazily re-evaluated above.
    // (Refreshing the whole boundary after every move - the previous
    // behaviour - cost O(|boundary| x k) model builds per move.)
    std::fill(touched.begin(), touched.end(), 0);
    for (const int e : hgraph->Edges(vertex)) {
      for (const int u : hgraph->Vertices(e)) {
        touched[u] = 1;
      }
    }
    neighbors.clear();
    for (const auto &v : boundary_vertices) {
      if (visited_vertices_flag[v]) continue;
      if (touched[v] || solution[v] == from_part || solution[v] == to_part) {
        neighbors.push_back(v);
      }
    }

    // Refresh gains in EVERY destination bucket for those vertices,
    // including the bucket of the partition the vertex just left.
    #if HAVE_OPENMP
    // Parallel update for large numbers of partitions
    if (num_parts_ > 8) {
      #pragma omp parallel for schedule(dynamic)
      for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
        UpdateSingleGainBucket(to_pid, buckets, hgraph, neighbors, net_degs, solution);
      }
    } else 
    #endif
    {
      // Sequential update for small numbers of partitions
      for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
        UpdateSingleGainBucket(to_pid, buckets, hgraph, neighbors, net_degs, solution);
      }
    }

    // Track the best solution seen so far
    const bool better_violation = violation_lt(current_violation_, best_violation);
    const bool same_violation = !better_violation &&
                                !violation_lt(best_violation, current_violation_);
    if (better_violation || (same_violation && total_delta_gain > best_gain)) {
      best_gain = total_delta_gain;
      best_violation = current_violation_;
      best_move_index = move_count;
    }
  }

  // Rollback to best solution
  for (int i = moves_trace.size() - 1; i > best_move_index; --i) {
    auto move = moves_trace[i];
    RollBackVertexGain(move, hgraph, visited_vertices_flag, solution, block_balance, net_degs);
  }
  current_violation_ = best_violation;
  if (floorplan_aware_) {
    fp_warm_ = best_move_index >= 0 ? move_seeds[best_move_index] : start_seed;
  }
  static const bool fp_debug = std::getenv("CHIPLETPART_FP_DEBUG") != nullptr;
  if (fp_debug && floorplan_aware_) {
    std::cerr << "[fp-debug] pass: parts=" << num_parts_ << (fp_repair_mode_ ? " repair" : "")
              << " start_viol=" << start_violation
              << " best_viol=" << best_violation << " moves=" << moves_trace.size()
              << " best_idx=" << best_move_index << " best_gain=" << best_gain
              << " fp_rejected=" << fp_rejected_total << " probes=" << fp_probe_count_ << "\n";
  }
  // Report a repaired violation as progress so Refine() keeps iterating.
  if (violation_lt(best_violation, start_violation) && best_gain <= 0.0f) {
    best_gain = std::numeric_limits<float>::min();
  }

  // Clean up resources efficiently
  moves_trace.clear();
  
  for (auto& bucket : buckets) {
    if (bucket->GetSize() > 0) {
      bucket->Clear();
    }
  }

  // Return gain improvement from this pass
  return best_gain;
}

void ChipletRefiner::InitializeGainBucketsKWay(
    GainBuckets &buckets, const HGraphPtr &hgraph,
    const std::vector<int> &boundary_vertices, const Matrix<int> &net_degs,
    const Partition &solution) {
  // Initialize the gain calculation slopes before bucket initialization
  
  // Set the number of threads for this parallel operation
  const int num_threads = std::min(omp_utils::get_max_threads(), 
                                  static_cast<int>(num_parts_));
  
  #if HAVE_OPENMP
  // Parallel initialization of gain buckets for all partitions
  // This creates an additional level of parallelism beyond the per-bucket level
  #pragma omp parallel for schedule(dynamic) num_threads(num_threads) if(num_parts_ > 4)
  for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
    InitializeSingleGainBucket(buckets, to_pid, hgraph, boundary_vertices,
                              net_degs, solution);
  }
  #else
  // Sequential initialization when OpenMP is not available
  for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
    InitializeSingleGainBucket(buckets, to_pid, hgraph, boundary_vertices,
                             net_degs, solution);
  }
  #endif
}

// Initialize the single bucket - thread-safe implementation
void ChipletRefiner::InitializeSingleGainBucket(
    GainBuckets &buckets,
    int to_pid,
    const HGraphPtr &hgraph,
    const std::vector<int> &boundary_vertices,
    const Matrix<int> &net_degs,
    const Partition &solution) {
  
  // Create a local set to store calculated gains before insertion
  // This reduces critical section size and improves parallel performance
  std::vector<GainCell> local_gains;
  local_gains.reserve(boundary_vertices.size() / 10); // Reasonable estimate
  
  // Process boundary vertices - store locally first
  #if HAVE_OPENMP
  if (boundary_vertices.size() > 1000) { // Only parallelize if enough work
    const int chunk_size = std::max(1, (int)(boundary_vertices.size() / omp_get_max_threads()));
    
    #pragma omp parallel
    {
      std::vector<GainCell> thread_local_gains;
      thread_local_gains.reserve(boundary_vertices.size() / (10 * omp_get_num_threads()));
      
      #pragma omp for schedule(dynamic, chunk_size)
      for (int i = 0; i < boundary_vertices.size(); i++) {
        const int v = boundary_vertices[i];
        const int from_part = solution[v];
        if (from_part == to_pid) {
          continue; // Skip if already in target partition
        }
        auto gain_cell = CalculateVertexGain(v, from_part, to_pid, hgraph, solution, net_degs);
        thread_local_gains.push_back(gain_cell);
      }
      
      // Merge thread-local results into the local gains vector
      #pragma omp critical(merge_gains)
      {
        local_gains.insert(local_gains.end(), thread_local_gains.begin(), thread_local_gains.end());
      }
    }
  } else {
  #endif
    // Sequential processing for small boundary sets
  for (const int &v : boundary_vertices) {
    const int from_part = solution[v];
    if (from_part == to_pid) {
        continue; // Skip if already in target partition
      }
      auto gain_cell = CalculateVertexGain(v, from_part, to_pid, hgraph, solution, net_degs);
      local_gains.push_back(gain_cell);
    }
  #if HAVE_OPENMP
  }
  #endif
  
  // Synchronize access to buckets - a single critical section for all insertions
  #if HAVE_OPENMP
  #pragma omp critical(bucket_update)
  {
  #endif
    // Set bucket to active (only do this once per bucket)
    buckets[to_pid]->SetActive();
    
    // Insert all gains into the bucket in a single critical section
    for (const auto& gain_cell : local_gains) {
    buckets[to_pid]->InsertIntoPQ(gain_cell);
  }
    
    // Deactivate if empty
  if (buckets[to_pid]->GetTotalElements() == 0) {
    buckets[to_pid]->SetDeactive();
  }
  #if HAVE_OPENMP
  }
  #endif
}

// Determine which vertex gain to be picked
std::shared_ptr<VertexGain>
ChipletRefiner::PickMoveKWay(GainBuckets &buckets, const HGraphPtr &hgraph,
                             const Matrix<float> &curr_block_balance,
                             const Matrix<float> &upper_block_balance,
                             const Matrix<float> &lower_block_balance,
                             std::vector<int> &partition) {
  // Initialize a default candidate
  auto candidate = std::make_shared<VertexGain>();
  int to_pid = -1;

  // Best gain bucket for "corking effect" - if no normal candidate is available
  int best_to_pid = -1; // block id with best_gain
  float best_gain = -std::numeric_limits<float>::max();
  

  #if HAVE_OPENMP
  // Parallel bucket inspection when many partitions exist (threshold: 8)
  if (num_parts_ > 8) {
    // Thread-local storage for best candidates and gains
    struct ThreadLocalBest {
      std::shared_ptr<VertexGain> candidate;
      int to_pid;
      float best_gain;
      int best_to_pid;
      
      ThreadLocalBest() : candidate(std::make_shared<VertexGain>()), 
                         to_pid(-1), 
                         best_gain(-std::numeric_limits<float>::max()), 
                         best_to_pid(-1) {}
    };
    
    std::vector<ThreadLocalBest> thread_local_results(omp_get_max_threads());
    
    #pragma omp parallel
    {
      int thread_id = omp_get_thread_num();
      auto& local_result = thread_local_results[thread_id];
      
      #pragma omp for schedule(dynamic, 4)
      for (int i = 0; i < num_parts_; ++i) {
        // Skip empty or inactive buckets
        if (buckets[i]->GetStatus() == false || buckets[i]->GetSize() == 0) {
          continue;
        }
        
        // Get maximum gain element from this bucket
        auto ele = buckets[i]->GetMax();
        const int vertex = ele->GetVertex();
        const float gain = ele->GetGain();
        const int from_pid = ele->GetSourcePart();
        
        // Balance legality: the move must respect the caller's block-balance
        // bounds. (The floorplan feasibility check that used to live here is
        // still disabled; see CheckFloorPlanFeasible.)
        const bool feasible = CheckVertexMoveLegality(
            vertex, i, from_pid, hgraph, curr_block_balance,
            upper_block_balance, lower_block_balance);
        
        // Update thread-local best candidate
        if (feasible == true && gain > local_result.candidate->GetGain()) {
          local_result.to_pid = i;
          local_result.candidate = ele;
        }
        
        // Record partition for corking effect
        if (gain > local_result.best_gain) {
          local_result.best_gain = gain;
          local_result.best_to_pid = i;
        }
      }
    }
    
    // Reduce thread-local results to find global best
    for (const auto& local_result : thread_local_results) {
      // Update global best candidate
      if (local_result.to_pid > -1 && 
          (to_pid == -1 || local_result.candidate->GetGain() > candidate->GetGain())) {
        to_pid = local_result.to_pid;
        candidate = local_result.candidate;
      }
      
      // Update global best gain for corking effect
      if (local_result.best_gain > best_gain) {
        best_gain = local_result.best_gain;
        best_to_pid = local_result.best_to_pid;
      }
    }
  } else {
  #endif
    // Original sequential implementation for smaller number of partitions
  // checking the first elements in each bucket
  for (int i = 0; i < num_parts_; ++i) {
    if (buckets[i]->GetStatus() == false || buckets[i]->GetSize() == 0) {
      continue; // This bucket is empty
    }
    auto ele = buckets[i]->GetMax();
    const int vertex = ele->GetVertex();
    const float gain = ele->GetGain();
    const int from_pid = ele->GetSourcePart();
    
    // Balance legality: the move must respect the caller's block-balance
    // bounds. (The floorplan feasibility check that used to live here is
    // still disabled; see CheckFloorPlanFeasible.)
    const bool feasible = CheckVertexMoveLegality(
        vertex, i, from_pid, hgraph, curr_block_balance,
        upper_block_balance, lower_block_balance);
    
    if (feasible == true && gain > candidate->GetGain()) {
      to_pid = i;
      candidate = ele;
    }

    // record part for solving corking effect
    if (gain > best_gain) {
      best_gain = gain;
      best_to_pid = i;
    }
  }
  #if HAVE_OPENMP
  }
  #endif
  
  // Case 1: if there is a candidate available or no vertex to move
  if (to_pid > -1 || best_to_pid == -1) {
    return candidate;
  }

  return candidate;
}

// move one vertex based on the calculated gain_cell
void ChipletRefiner::AcceptKWayMove(
    const std::shared_ptr<VertexGain> &gain_cell, GainBuckets &gain_buckets,
    std::vector<GainCell> &moves_trace, float &total_delta_gain,
    std::vector<bool> &visited_vertices_flag, const HGraphPtr &hgraph,
    Matrix<float> &curr_block_balance, Matrix<int> &net_degs,
    std::vector<int> &solution) const {
  const int vertex_id = gain_cell->GetVertex();
  moves_trace.push_back(gain_cell);
  AcceptVertexGain(gain_cell, hgraph, total_delta_gain, visited_vertices_flag,
                   solution, curr_block_balance, net_degs);
  
  // Remove vertex from all buckets in parallel
  #if HAVE_OPENMP
  // OpenMP implementation
  #pragma omp parallel for num_threads(num_parts_)
  for (int i = 0; i < num_parts_; ++i) {
    HeapEleDeletion(vertex_id, i, gain_buckets);
  }
  #else
  for (int i = 0; i < num_parts_; ++i) {
    HeapEleDeletion(vertex_id, i, gain_buckets);
  }
  #endif
}

// Remove vertex from a heap
// Remove the vertex id related vertex gain
void ChipletRefiner::HeapEleDeletion(int vertex_id, int part,
                                     GainBuckets &buckets) const {
  buckets[part]->Remove(vertex_id);
}

// After moving one vertex, the gain of its neighbors will also need
// to be updated. This function is used to update the gain of neighbor vertices
void ChipletRefiner::UpdateSingleGainBucket(int part, GainBuckets &buckets,
                                            const HGraphPtr &hgraph,
                                            const std::vector<int> &neighbors,
                                            const Matrix<int> &net_degs,
                                            const Partition &solution) {
  
  #if HAVE_OPENMP
  // Parallel processing of neighbors when OpenMP is available and there's enough work
  if (neighbors.size() > 1000) {
    const int chunk_size = std::max(1, (int)(neighbors.size() / omp_get_max_threads()));
    
    #pragma omp parallel for schedule(dynamic, chunk_size)
    for (int i = 0; i < neighbors.size(); i++) {
      const int v = neighbors[i];
      const int from_part = solution[v];
      if (from_part == part) {
        continue;
      }
      
      // recalculate the current gain of the vertex v
      auto gain_cell =
          CalculateVertexGain(v, from_part, part, hgraph, solution, net_degs);
      
      // Critical section for bucket operations
      #pragma omp critical
      {
        // check if the vertex exists in current bucket
        if (buckets[part]->CheckIfVertexExists(v) == true) {
          // update the bucket with new gain
          buckets[part]->ChangePriority(v, gain_cell);
        } else {
          buckets[part]->InsertIntoPQ(gain_cell);
        }
      }
    }
  } else {
  #endif
    // Sequential processing for small neighbor sets or when OpenMP is not available
  for (const int &v : neighbors) {
    const int from_part = solution[v];
    if (from_part == part) {
      continue;
    }
      
    // recalculate the current gain of the vertex v
    auto gain_cell =
        CalculateVertexGain(v, from_part, part, hgraph, solution, net_degs);
      
    // check if the vertex exists in current bucket
    if (buckets[part]->CheckIfVertexExists(v) == true) {
      // update the bucket with new gain
      buckets[part]->ChangePriority(v, gain_cell);
    } else {
      buckets[part]->InsertIntoPQ(gain_cell);
    }
  }
  #if HAVE_OPENMP
  }
  #endif
}

// Find all the boundary vertices.
// The boundary vertices do not include fixed vertices
std::vector<int> ChipletRefiner::FindBoundaryVertices(
    const HGraphPtr &hgraph, const Matrix<int> &net_degs,
    const std::vector<bool> &visited_vertices_flag,
    float random_non_boundary_ratio) const {
  const int num_hyperedges = hgraph->GetNumHyperedges();
  const int num_vertices = hgraph->GetNumVertices();
  
  // Pre-allocate vector for flags with exact size needed
  // std::vector<char>, not <bool>: written concurrently in the parallel loop
  // below (vector<bool> packs bits, so adjacent writes race).
  std::vector<char> boundary_net_flag(num_hyperedges, 0);
  
  // Pre-allocate result vector with a reasonable size estimation
  // In typical circuits, approximately 15-20% of vertices are on boundaries
  std::vector<int> boundary_vertices;
  boundary_vertices.reserve(num_vertices / 5);
  
  // Use a bitmap for tracking which vertices we've already processed
  // This avoids duplicate entries in boundary_vertices
  std::vector<bool> is_boundary(num_vertices, false);
  
  // Step 1: Find boundary hyperedges efficiently 
  // A hyperedge is on the boundary if it connects vertices in different partitions
  #if HAVE_OPENMP
  // Parallel processing for large hypergraphs
  if (num_hyperedges > 1000) {
    #pragma omp parallel
    {
      // Thread-local stack-based buffers for part counting
      // This avoids atomic operations or critical sections
      std::vector<bool> local_parts_seen(num_parts_, false);
      
      #pragma omp for schedule(dynamic, 64) nowait
      for (int e = 0; e < num_hyperedges; e++) {
        // Clear the local buffer
        std::fill(local_parts_seen.begin(), local_parts_seen.end(), false);
        
        // Count partitions this hyperedge spans
        int num_span_parts = 0;
        for (int i = 0; i < num_parts_; i++) {
          if (net_degs[e][i] > 0) {
            local_parts_seen[i] = true;
            num_span_parts++;
            
            // Early exit when we know it's a boundary net
            if (num_span_parts >= 2) {
              boundary_net_flag[e] = 1;
              break;
            }
          }
        }
      }
    }
  } else {
  #endif
    // Sequential processing with optimized memory access pattern
    for (int e = 0; e < num_hyperedges; e++) {
      // Count different partitions this hyperedge connects
      int num_span_parts = 0;
      for (int i = 0; i < num_parts_; i++) {
        if (net_degs[e][i] > 0) {
          num_span_parts++;
          
          // Early exit when we know it's a boundary net
          if (num_span_parts >= 2) {
            boundary_net_flag[e] = 1;
            break;
          }
        }
      }
    }
  #if HAVE_OPENMP
  }
  #endif
  
  // Step 2: Find vertices connected to boundary hyperedges
  #if HAVE_OPENMP
  // Parallel collection with thread-local buffers
  if (num_vertices > 1000) {
    #pragma omp parallel
    {
      // Thread-local buffer for boundary vertices
      std::vector<int> local_boundaries;
      local_boundaries.reserve(num_vertices / (5 * omp_get_num_threads()));
      
      // Thread-local bitmap to track processed vertices
      std::vector<bool> local_is_boundary(num_vertices, false);
      
      #pragma omp for schedule(dynamic, 64)
      for (int v = 0; v < num_vertices; v++) {
        // Skip vertices that are already visited globally
        if (visited_vertices_flag[v]) {
          continue;
        }
        
        // Check if connected to any boundary net
        for (const int edge_id : hgraph->Edges(v)) {
          if (boundary_net_flag[edge_id]) {
            local_boundaries.push_back(v);
            local_is_boundary[v] = true;
            break;
          }
        }
      }
      
      // Merge thread-local results
      #pragma omp critical
      {
        for (const int v : local_boundaries) {
          // Double-check we haven't already added this vertex
          // This can happen if two threads process connected hyperedges
          if (!is_boundary[v]) {
            boundary_vertices.push_back(v);
            is_boundary[v] = true;
          }
        }
      }
    }
  } else {
  #endif
    // Sequential version with bitmap optimization
    for (int v = 0; v < num_vertices; v++) {
      // Skip visited vertices
      if (visited_vertices_flag[v] || is_boundary[v]) {
        continue;
      }
      
      // Check if this vertex is connected to any boundary net
      for (const int edge_id : hgraph->Edges(v)) {
        if (boundary_net_flag[edge_id]) {
          boundary_vertices.push_back(v);
          is_boundary[v] = true;
          break;
        }
      }
    }
  #if HAVE_OPENMP
  }
  #endif

  // Step 3: Add random non-boundary vertices
  if (random_non_boundary_ratio > 0.0f) {
    // Calculate how many random vertices to add
    const int existing_count = boundary_vertices.size();
    const int random_count = static_cast<int>(num_vertices * random_non_boundary_ratio);
    
    if (random_count > 0) {
      // Create a random number generator
      std::random_device rd;
      std::mt19937 gen(rd());

      // We'll use reservoir sampling to efficiently select random vertices
      // This avoids having to construct a full vector of non-boundary vertices
      #if HAVE_OPENMP
      if (num_vertices > 1000) {
        // For parallel execution, we divide the sampling across threads
        const int max_threads = omp_get_max_threads();
        const int per_thread_sample = (random_count + max_threads - 1) / max_threads;
        
        // Store thread-local results
        std::vector<std::vector<int>> thread_samples(max_threads);
        
        #pragma omp parallel
        {
          const int thread_id = omp_get_thread_num();
          std::vector<int>& local_samples = thread_samples[thread_id];
          local_samples.reserve(per_thread_sample);
          
          // Create thread-local RNG with unique seed
          std::mt19937 local_gen(rd() + thread_id);
          
          // Each thread processes a subset of vertices
          #pragma omp for schedule(dynamic, 64)
          for (int v = 0; v < num_vertices; v++) {
            // Skip vertices that are boundary or visited
            if (is_boundary[v] || visited_vertices_flag[v]) {
              continue;
            }
            
            // Use reservoir sampling for the local thread's quota
            if (local_samples.size() < per_thread_sample) {
              local_samples.push_back(v);
            } else {
              // Replace elements with decreasing probability
              std::uniform_int_distribution<int> dis(0, v);
              int pos = dis(local_gen);
              if (pos < per_thread_sample) {
                local_samples[pos] = v;
              }
            }
          }
        }
        
        // Merge thread-local samples and select final random set
        std::vector<int> all_samples;
        for (const auto& samples : thread_samples) {
          all_samples.insert(all_samples.end(), samples.begin(), samples.end());
        }
        
        // Shuffle and take the first random_count elements
        std::shuffle(all_samples.begin(), all_samples.end(), gen);
        const int to_take = std::min(random_count, static_cast<int>(all_samples.size()));
        
        // Add the random non-boundary vertices to the result
        for (int i = 0; i < to_take; i++) {
          const int v = all_samples[i];
          boundary_vertices.push_back(v);
          // No need to update is_boundary since we won't check it again
        }
      } else {
      #endif
        // For sequential execution, use simple reservoir sampling
        std::vector<int> selected_vertices;
        selected_vertices.reserve(random_count);
        
        int seen_non_boundary = 0;
        
        for (int v = 0; v < num_vertices; v++) {
          // Skip vertices that are boundary or visited
          if (is_boundary[v] || visited_vertices_flag[v]) {
            continue;
          }
          
          seen_non_boundary++;
          
          // Use reservoir sampling algorithm
          if (selected_vertices.size() < random_count) {
            selected_vertices.push_back(v);
          } else {
            // Replace elements with decreasing probability
            std::uniform_int_distribution<int> dis(0, seen_non_boundary - 1);
            int pos = dis(gen);
            if (pos < random_count) {
              selected_vertices[pos] = v;
            }
          }
        }
        
        // Add the selected random vertices to the result
        boundary_vertices.insert(
            boundary_vertices.end(), 
            selected_vertices.begin(), 
            selected_vertices.end()
        );
      #if HAVE_OPENMP
      }
      #endif
      
      if (!boundary_vertices.empty()) {
        // Shuffle the boundary vertices to avoid clustering
        std::shuffle(boundary_vertices.begin(), boundary_vertices.end(), gen);
      }
    }
  }
  
  return boundary_vertices;
}

// Find boundary vertices between specific partitions
std::vector<int> ChipletRefiner::FindBoundaryVertices(
    const HGraphPtr &hgraph, const Matrix<int> &net_degs,
    const std::vector<bool> &visited_vertices_flag,
    const std::vector<int> &solution,
    const std::pair<int, int> &partition_pair) const {
  const int num_hyperedges = hgraph->GetNumHyperedges();
  const int num_vertices = hgraph->GetNumVertices();
  const int part_a = partition_pair.first;
  const int part_b = partition_pair.second;
  
  // Pre-allocate with exact size for boundary net flags
  // std::vector<char>, not <bool>: written concurrently in the parallel loop
  // below (vector<bool> packs bits, so adjacent writes race).
  std::vector<char> boundary_net_flag(num_hyperedges, 0);
  
  // Pre-allocate result vector - estimate 10% of vertices on partition boundaries
  std::vector<int> boundary_vertices;
  boundary_vertices.reserve(num_vertices / 10);
  
  // Use bitmap to track vertex processing state
  std::vector<bool> is_boundary(num_vertices, false);
  
  // Step 1: Find hyperedges that span the two specified partitions
  // This is a highly specialized and focused search
  #if HAVE_OPENMP
  // Parallel processing for large hypergraphs
  if (num_hyperedges > 1000) {
    #pragma omp parallel for schedule(static)
    for (int e = 0; e < num_hyperedges; e++) {
      // Check if edge spans both partitions - constant time check
      if (net_degs[e][part_a] > 0 && net_degs[e][part_b] > 0) {
        boundary_net_flag[e] = 1;
      }
    }
  } else {
  #endif
    // Sequential version with optimized locality
    for (int e = 0; e < num_hyperedges; e++) {
      // Direct indexing is fast and maintains cache locality
      if (net_degs[e][part_a] > 0 && net_degs[e][part_b] > 0) {
        boundary_net_flag[e] = 1;
      }
    }
  #if HAVE_OPENMP
  }
  #endif
  
  // Step 2: Find vertices connected to boundary hyperedges
  #if HAVE_OPENMP
  // Parallel collection with thread-local storage
  if (num_vertices > 1000) {
    // Distribute vertices among threads
    #pragma omp parallel
    {
      // Thread-local storage
      std::vector<int> local_boundaries;
      local_boundaries.reserve(num_vertices / (10 * omp_get_num_threads()));
      
      // Thread-local bitmap
      std::vector<bool> local_is_boundary(num_vertices, false);
      
      #pragma omp for schedule(dynamic, 64)
      for (int v = 0; v < num_vertices; v++) {
        // Skip visited vertices
        if (visited_vertices_flag[v]) {
          continue;
        }
        
        // Skip vertices not in either of the target partitions - fast early rejection
        const int p = solution[v];
        if (p != part_a && p != part_b) {
          continue;
        }
        
        // Check if vertex is connected to a boundary net
        for (const int edge_id : hgraph->Edges(v)) {
          if (boundary_net_flag[edge_id]) {
            local_boundaries.push_back(v);
            local_is_boundary[v] = true;
            break;
          }
        }
      }
      
      // Combine thread results
      #pragma omp critical
      {
        for (const int v : local_boundaries) {
          // Ensure no duplicates
          if (!is_boundary[v]) {
            boundary_vertices.push_back(v);
            is_boundary[v] = true;
          }
        }
      }
    }
  } else {
  #endif
    // Sequential version with bitmap tracking
    for (int v = 0; v < num_vertices; v++) {
      // Skip visited vertices
      if (visited_vertices_flag[v] || is_boundary[v]) {
        continue;
      }
      
      // Skip vertices not in target partitions (early rejection)
      const int p = solution[v];
      if (p != part_a && p != part_b) {
        continue;
      }
      
      // Check if connected to a boundary net
      for (const int edge_id : hgraph->Edges(v)) {
        if (boundary_net_flag[edge_id]) {
          boundary_vertices.push_back(v);
          is_boundary[v] = true;
          break;
        }
      }
    }
  #if HAVE_OPENMP
  }
  #endif
  
  return boundary_vertices;
}


// Functions related to move a vertex
// -----------------------------------------------------------
// The most important function for refinent
// If we want to update the score function for other purposes
// we should update this function.
// -----------------------------------------------------------
// calculate the possible gain of moving a vertex
// we need following arguments:
// from_pid : from block id
// to_pid : to block id
// solution : the current solution
// cur_path_cost : current path cost
// net_degs : current net degrees
GainCell ChipletRefiner::CalculateVertexGain(int v, int from_pid, int to_pid,
                                             const HGraphPtr &hgraph,
                                             const std::vector<int> &solution,
                                             const Matrix<int> &net_degs) {
  // No gain if moving to the same partition
  if (from_pid == to_pid) {
    return std::make_shared<VertexGain>(v, from_pid, to_pid, 0.0f);
  }

  // If cost model is available, incorporate its evaluation
  if (cost_model_initialized_ && libraryDicts_ != nullptr) {
    // Get the cost model gain for this move
    float cost_model_gain = GetSingleMoveCost(solution, v, from_pid, to_pid);
    return std::make_shared<VertexGain>(v, from_pid, to_pid, cost_model_gain);
  } else {
    // If no cost model, return zero gain
    std::cout << "[Warning] No cost model available, returning zero gain." << std::endl;  
    return std::make_shared<VertexGain>(v, from_pid, to_pid, 0.0);
  }
}

// move one vertex based on the calculated gain_cell
void ChipletRefiner::AcceptVertexGain(
    const GainCell &gain_cell, const HGraphPtr &hgraph, float &total_delta_gain,
    std::vector<bool> &visited_vertices_flag, std::vector<int> &solution,
    Matrix<float> &curr_block_balance, Matrix<int> &net_degs) const {
  const int vertex_id = gain_cell->GetVertex();
  visited_vertices_flag[vertex_id] = true;
  total_delta_gain += gain_cell->GetGain(); // increase the total gain
  // get partition id
  const int pre_part_id = gain_cell->GetSourcePart();
  const int new_part_id = gain_cell->GetDestinationPart();
  // update the solution vector
  solution[vertex_id] = new_part_id;
  
  // Update the partition balance - safely without using vector operators
  const std::vector<float>& vertex_weights = hgraph->GetVertexWeights(vertex_id);
  
  // Check vector sizes before updating
  if (vertex_weights.size() != curr_block_balance[pre_part_id].size() ||
      vertex_weights.size() != curr_block_balance[new_part_id].size()) {
    std::cerr << "Vector size mismatch in AcceptVertexGain: "
              << "vertex_weights.size()=" << vertex_weights.size() 
              << ", pre_block_balance.size()=" << curr_block_balance[pre_part_id].size()
              << ", new_block_balance.size()=" << curr_block_balance[new_part_id].size()
              << std::endl;
    return; // Can't safely update, return without changing balance
  }
  
  // Update balances manually
  for (size_t i = 0; i < vertex_weights.size(); ++i) {
    curr_block_balance[pre_part_id][i] -= vertex_weights[i];
    curr_block_balance[new_part_id][i] += vertex_weights[i];
  }
  
  // update net_degs
  for (const int he : hgraph->Edges(vertex_id)) {
    --net_degs[he][pre_part_id];
    ++net_degs[he][new_part_id];
  }
}

// restore one vertex based on the calculated gain_cell
void ChipletRefiner::RollBackVertexGain(
    const GainCell &gain_cell, const HGraphPtr &hgraph,
    std::vector<bool> &visited_vertices_flag, std::vector<int> &solution,
    Matrix<float> &curr_block_balance, Matrix<int> &net_degs) const {
  const int vertex_id = gain_cell->GetVertex();
  visited_vertices_flag[vertex_id] = false;
  // get partition id
  const int pre_part_id = gain_cell->GetSourcePart();
  const int new_part_id = gain_cell->GetDestinationPart();
  // update the solution vector
  solution[vertex_id] = pre_part_id;
  
  // Update the partition balance - safely without using vector operators
  const std::vector<float>& vertex_weights = hgraph->GetVertexWeights(vertex_id);
  
  // Check vector sizes before updating
  if (vertex_weights.size() != curr_block_balance[pre_part_id].size() ||
      vertex_weights.size() != curr_block_balance[new_part_id].size()) {
    std::cerr << "Vector size mismatch in RollBackVertexGain: "
              << "vertex_weights.size()=" << vertex_weights.size() 
              << ", pre_block_balance.size()=" << curr_block_balance[pre_part_id].size()
              << ", new_block_balance.size()=" << curr_block_balance[new_part_id].size()
              << std::endl;
    return; // Can't safely update, return without changing balance
  }
  
  // Update balances manually
  for (size_t i = 0; i < vertex_weights.size(); ++i) {
    curr_block_balance[pre_part_id][i] += vertex_weights[i];
    curr_block_balance[new_part_id][i] -= vertex_weights[i];
  }
  
  // update net_degs
  for (const int he : hgraph->Edges(vertex_id)) {
    ++net_degs[he][pre_part_id];
    --net_degs[he][new_part_id];
  }
}

// check if we can move the vertex to some block
// Here we assume the vertex v is not in the block to_pid
bool ChipletRefiner::CheckVertexMoveLegality(
    int v,        // vertex id
    int to_pid,   // to block id
    int from_pid, // from block_id
    const HGraphPtr &hgraph, const Matrix<float> &curr_block_balance,
    const Matrix<float> &upper_block_balance,
    const Matrix<float> &lower_block_balance) const {
  // Defensive bounds checks (bucket entries may be stale / from empty parts)
  if (v < 0 || v >= hgraph->GetNumVertices() || to_pid < 0 || from_pid < 0 ||
      to_pid == from_pid ||
      to_pid >= static_cast<int>(curr_block_balance.size()) ||
      from_pid >= static_cast<int>(curr_block_balance.size()) ||
      to_pid >= static_cast<int>(upper_block_balance.size()) ||
      from_pid >= static_cast<int>(lower_block_balance.size())) {
    return false;
  }
  // Get vertex weights and current block balance
  const std::vector<float>& vertex_weights = hgraph->GetVertexWeights(v);
  const std::vector<float>& to_block_balance = curr_block_balance[to_pid];
  const std::vector<float>& from_block_balance = curr_block_balance[from_pid];
  
  // Check if sizes match to avoid vector size mismatch errors
  if (vertex_weights.size() != to_block_balance.size() || 
      vertex_weights.size() != from_block_balance.size() ||
      to_block_balance.size() != upper_block_balance[to_pid].size() ||
      from_block_balance.size() != lower_block_balance[from_pid].size()) {
    // Sizes don't match, log error and return false (can't move)
    std::cerr << "Vector size mismatch in CheckVertexMoveLegality: "
              << "vertex_weights.size()=" << vertex_weights.size() 
              << ", to_block_balance.size()=" << to_block_balance.size()
              << ", from_block_balance.size()=" << from_block_balance.size()
              << ", upper_balance.size()=" << upper_block_balance[to_pid].size()
              << ", lower_balance.size()=" << lower_block_balance[from_pid].size()
              << std::endl;
    return false;
  }
  
  // Create result vectors manually instead of using operator+
  std::vector<float> total_wt_to_block(to_block_balance.size());
  std::vector<float> total_wt_from_block(from_block_balance.size());
  
  // Manually add elements to avoid using operator+
  for (size_t i = 0; i < to_block_balance.size(); ++i) {
    total_wt_to_block[i] = to_block_balance[i] + vertex_weights[i];
  }
  
  for (size_t i = 0; i < from_block_balance.size(); ++i) {
    total_wt_from_block[i] = from_block_balance[i] - vertex_weights[i];
  }
  
  // Check constraints
  bool to_constraint_satisfied = true;
  bool from_constraint_satisfied = true;
  
  for (size_t i = 0; i < total_wt_to_block.size(); ++i) {
    if (total_wt_to_block[i] > upper_block_balance[to_pid][i]) {
      to_constraint_satisfied = false;
      break;
    }
  }
  
  for (size_t i = 0; i < total_wt_from_block.size(); ++i) {
    if (total_wt_from_block[i] < lower_block_balance[from_pid][i]) {
      from_constraint_satisfied = false;
      break;
    }
  }
  
  return to_constraint_satisfied && from_constraint_satisfied;
}

HGraphPtr ChipletRefiner::GenerateNetlist(const HGraphPtr hgraph,
                                          const std::vector<int> &partition) {
  // Calculate block balances once and reuse
  Matrix<float> vertex_weights_c = GetBlockBalance(hgraph, partition);
  
  // Create a new vertex weights matrix only including non-empty clusters
  Matrix<float> new_vertex_weights_c;
  new_vertex_weights_c.reserve(vertex_weights_c.size());
  
  // Compact partition ids: chiplet k of the generated netlist is the k-th
  // non-empty partition in ascending id order. The SAME map must be used for
  // the vertex weights (below) and for remapping hyperedges, otherwise the
  // chiplet areas and the nets that connect them refer to different chiplets.
  // (Previously the weights used ascending order while the hyperedges used
  // first-appearance order.)
  std::vector<int> partition_to_cluster(vertex_weights_c.size(), -1);
  for (int i = 0; i < static_cast<int>(vertex_weights_c.size()); i++) {
    if (!vertex_weights_c[i].empty() && vertex_weights_c[i][0] > 0.0) {
      partition_to_cluster[i] = static_cast<int>(new_vertex_weights_c.size());
      new_vertex_weights_c.push_back(vertex_weights_c[i]);
    }
  }

  std::vector<int> vertex_cluster_id_vec(partition.size(), -1);
  for (size_t v = 0; v < partition.size(); v++) {
    const int pid = partition[v];
    if (pid >= 0 && pid < static_cast<int>(partition_to_cluster.size())) {
      vertex_cluster_id_vec[v] = partition_to_cluster[pid];
    }
  }

  // Pre-allocate space for hyperedges and weights
  const int num_hyperedges = hgraph->GetNumHyperedges();
  Matrix<int> hyperedges_c;
  Matrix<float> hyperedges_weights_c;
  
  // Based on typical connectivity, reserve a reasonable amount of space
  hyperedges_c.reserve(num_hyperedges / 2);
  hyperedges_weights_c.reserve(num_hyperedges / 2);
  
  std::vector<float> reaches;
  std::vector<float> io_cell_sizes;
  reaches.reserve(num_hyperedges / 2);
  io_cell_sizes.reserve(num_hyperedges / 2);
  
  // Process all hyperedges
  // Use a flat vector and unordered_set for temp storage to improve cache locality
  std::unordered_set<int> hyperedge_c;
  hyperedge_c.reserve(num_parts_); // Maximum possible size
  
  for (int e = 0; e < num_hyperedges; e++) {
    const auto range = hgraph->Vertices(e);
    const int he_size = range.size();
    
    // Skip small hyperedges
    if (he_size <= 1) {
      continue;
    }
    
    // Clear the set instead of creating a new one each iteration
    hyperedge_c.clear();
    
    // Add all cluster IDs of vertices in this hyperedge
    for (const int vertex_id : range) {
      const int cid = vertex_cluster_id_vec[vertex_id];
      if (cid >= 0) {
        hyperedge_c.insert(cid);
      }
    }
    
    // Skip if hyperedge only spans a single cluster
    if (hyperedge_c.size() <= 1) {
      continue;
    }
    
    // Convert set to vector for the hyperedge
    std::vector<int> he_vec(hyperedge_c.begin(), hyperedge_c.end());
    hyperedges_c.push_back(std::move(he_vec));
    hyperedges_weights_c.push_back(hgraph->GetHyperedgeWeights(e));
    reaches.push_back(hgraph->GetReach(e));
    io_cell_sizes.push_back(hgraph->GetIoSize(e));
  }

  // Create the new chiplet-level hypergraph
  HGraphPtr chiplet_level_hgraph = std::make_shared<Hypergraph>(
      hgraph->GetVertexDimensions(), hgraph->GetHyperedgeDimensions(),
      hyperedges_c, new_vertex_weights_c, hyperedges_weights_c, reaches,
      io_cell_sizes);

  return chiplet_level_hgraph;
}

// First, implement the destructor to clean up cost model resources
ChipletRefiner::~ChipletRefiner() {
  if (libraryDicts_ != nullptr) {
    destroyDatabase(libraryDicts_);
    libraryDicts_ = nullptr;
    cost_model_initialized_ = false;
  }
}

// Add cost model file configuration method
void ChipletRefiner::SetCostModelFiles(
    const std::string& io_file,
    const std::string& layer_file,
    const std::string& wafer_process_file,
    const std::string& assembly_process_file,
    const std::string& test_file,
    const std::string& netlist_file,
    const std::string& blocks_file) {
  
  io_file_ = io_file;
  layer_file_ = layer_file;
  wafer_process_file_ = wafer_process_file;
  assembly_process_file_ = assembly_process_file;
  test_file_ = test_file;
  netlist_file_ = netlist_file;
  blocks_file_ = blocks_file;
  
  // Clean up existing cost model if it was initialized
  if (cost_model_initialized_ && libraryDicts_ != nullptr) {
    destroyDatabase(libraryDicts_);
    libraryDicts_ = nullptr;
    cost_model_initialized_ = false;
  }
}

// Initialize the cost model with the configured files
bool ChipletRefiner::InitializeCostModel() {
  // Check if all required files are set
  if (io_file_.empty() || layer_file_.empty() || wafer_process_file_.empty() ||
      assembly_process_file_.empty() || test_file_.empty() || netlist_file_.empty() ||
      blocks_file_.empty()) {
    std::cerr << "[ERROR] Cannot initialize cost model: file paths not set" << std::endl;
    return false;
  }
  
  // Clean up existing resources if needed
  if (libraryDicts_ != nullptr) {
    destroyDatabase(libraryDicts_);
    libraryDicts_ = nullptr;
  }
  
  // Initialize the cost model
  try {

    // Initialize the library dictionaries
    libraryDicts_ = init(io_file_, layer_file_, wafer_process_file_, 
                          assembly_process_file_, test_file_, netlist_file_, 
                          blocks_file_);
    
    // Load blocks and reorder them into netlist (vertex) order so that
    // blocks_[i] corresponds to partition[i] and to row i of the adjacency
    // matrices in libraryDicts_.
    blocks_.clear();
    if (libraryDicts_ != nullptr) {
      blocks_ = alignBlocksToNetlist(readBlocks(blocks_file_), libraryDicts_->block_names);
    }
    
    cost_model_initialized_ = (libraryDicts_ != nullptr && !blocks_.empty());
    return cost_model_initialized_;
  } catch (const std::exception& e) {
    std::cerr << "[ERROR] Error initializing cost model: " << e.what() << std::endl;
    cost_model_initialized_ = false;
    return false;
  }
}

// Calculate the cost difference of moving a block between partitions
// Returns the gain (current cost - new cost) from the cost model
float ChipletRefiner::GetSingleMoveCost(
    const std::vector<int> &basePartitionIds, const int blockId,
    const int fromPartitionId, const int toPartitionId) {
  // Early return if cost model not initialized
  if (!cost_model_initialized_ || libraryDicts_ == nullptr) {
    return 0.0f;
  }

  // Create a modified partition vector to represent the move
  std::vector<int> newPartitionIds(basePartitionIds);
  newPartitionIds[blockId] = toPartitionId;
  
  // Start timing the cost calculation
  auto start_time = std::chrono::high_resolution_clock::now();
  
  // Calculate cost with proposed move using our class method that includes approx_state
  float newCost = GetCostFromScratch(newPartitionIds, true);
  
  // Compute gain (current cost - new cost)
  float gain = legacy_cost_ - newCost;
  
  // Record time spent in cost model calculation
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time).count();
  
  // This is called from parallel bucket initialisation/update loops; the
  // accumulator is shared, so the update must be atomic.
  const float elapsed_s = duration / 1000000.0f;
  #if HAVE_OPENMP
  #pragma omp atomic
  #endif
  total_cost_model_time_ += elapsed_s;
  
  // Return the gain value
  return gain;
}

// Implementation of GetCostFromScratch that calls the other version with default parameters
float ChipletRefiner::GetCostFromScratch(const std::vector<int>& partition, bool approx_state) const {
  return CostWithAspectRatios(partition, aspect_ratios_, approx_state);
}

float ChipletRefiner::CostWithAspectRatios(const std::vector<int>& partition,
                                           const std::vector<float>& aspect_ratios,
                                           bool approx_state) const {
  if (!cost_model_initialized_ || libraryDicts_ == nullptr) {
    return 0.0f; // Return 0 if cost model not initialized
  }
  
  // Get the number of partitions
  int num_partitions = 0;
  for (int part_id : partition) {
    num_partitions = std::max(num_partitions, part_id + 1);
  }
  
  // Ensure tech_array has the correct size
  std::vector<std::string> local_tech_array = tech_array_;
  if (local_tech_array.size() != num_partitions) {
    //std::cerr << "Warning: Resizing tech_array from " << local_tech_array.size() 
    //          << " to " << num_partitions << " elements" << std::endl;
    local_tech_array.resize(num_partitions);
    // Fill any new elements with a default technology
    for (size_t i = tech_array_.size(); i < num_partitions; ++i) {
      if (!tech_array_.empty()) {
        local_tech_array[i] = tech_array_[0]; // Use first tech as default
      } else {
        local_tech_array[i] = "7nm"; // Fallback default
      }
    }
  }
  
  // Ensure aspect_ratios has the correct size
  std::vector<float> local_aspect_ratios = aspect_ratios;
  if (local_aspect_ratios.size() != num_partitions) {
    /*std::cerr << "Warning: Resizing aspect_ratios from " << local_aspect_ratios.size() 
              << " to " << num_partitions << " elements" << std::endl;*/
    local_aspect_ratios.resize(num_partitions, 1.0f); // Default aspect ratio is 1.0
  }
  
  // Ensure x_locations has the correct size
  std::vector<float> local_x_locations = x_locations_;
  if (local_x_locations.size() != num_partitions) {
    /*std::cerr << "Warning: Resizing x_locations from " << local_x_locations.size() 
              << " to " << num_partitions << " elements" << std::endl;*/
    local_x_locations.resize(num_partitions, 0.0f); // Default x location is 0.0
  }
  
  // Ensure y_locations has the correct size
  std::vector<float> local_y_locations = y_locations_;
  if (local_y_locations.size() != num_partitions) {
    /*std::cerr << "Warning: Resizing y_locations from " << local_y_locations.size() 
              << " to " << num_partitions << " elements" << std::endl;*/
    local_y_locations.resize(num_partitions, 0.0f); // Default y location is 0.0
  }
  
  return getCostFromScratch(
      partition, 
      local_tech_array, 
      local_aspect_ratios, 
      local_x_locations, 
      local_y_locations,
      libraryDicts_, 
      blocks_,
      cost_coefficient_, 
      power_coefficient_,
      approx_state);  // Pass the approx_state parameter
}

} // namespace chiplet