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
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace chiplet {

VertexGain::VertexGain(const int vertex, const int src_block_id,
                       const int destination_block_id, const float gain)
    : vertex_(vertex), source_part_(src_block_id),
      destination_part_(destination_block_id), gain_(gain) {}

HyperedgeGain::HyperedgeGain(const int hyperedge_id, const int destination_part,
                             const float gain)
    : hyperedge_id_(hyperedge_id), destination_part_(destination_part),
      gain_(gain) {}

ChipletRefiner::ChipletRefiner(
    const int num_parts, const int refiner_iters,
    const int max_move, // the maximum number of vertices or
                        // hyperedges can be moved in each pass
    std::vector<int> reaches)
    : num_parts_(num_parts), refiner_iters_(refiner_iters), max_move_(max_move),
      refiner_iters_default_(refiner_iters), max_move_default_(max_move),
      reaches_(reaches) {
  // get max threads
  num_threads_ = std::thread::hardware_concurrency();
  // get 30% of these threads and round down
  num_threads_ = num_threads_ * 0.1;
}

void ChipletRefiner::SetMaxMove(const int max_move) { max_move_ = max_move; }

void ChipletRefiner::SetRefineIters(const int refiner_iters) {
  refiner_iters_ = refiner_iters;
}

void ChipletRefiner::RestoreDefaultParameters() {
  max_move_ = max_move_default_;
  refiner_iters_ = refiner_iters_default_;
}

void ChipletRefiner::InitFloorPlan(const HGraphPtr hgraph, int max_steps,
                                   int perturbations,
                                   float cooling_acceleration_factor,
                                   std::vector<int> &solution) {
  HGraphPtr chiplet_level_netlist = GenerateNetlist(hgraph, solution);
  BuildChiplets(chiplet_level_netlist);
  auto floor_tupple =
      Floorplanner(max_steps, perturbations, cooling_acceleration_factor);
  bool success = std::get<3>(floor_tupple);

  if (success == false) {
    std::cout << "Cannot find a valid solution" << std::endl;
  } else {
    // auto pos_seq = std::get<0>(floor_tupple);
    // auto neg_seq = std::get<1>(floor_tupple);
    // SetSequences(pos_seq, neg_seq);
  }
}

void ChipletRefiner::BuildChiplets(const HGraphPtr &hgraph) {
  bundled_nets_.clear();
  chiplets_.clear();

  for (int i = 0; i < hgraph->GetNumHyperedges(); ++i) {
    float weight = hgraph->GetHyperedgeWeights(i)[0];
    auto vertices = hgraph->Vertices(i);
    std::vector<int> he(vertices.begin(), vertices.end());
    int term_a = he[0];
    int term_b = he[1];
    bundled_nets_.push_back(
        BundledNet(std::pair<int, int>(term_a, term_b), weight, reach_));
  }

  std::vector<float> ones(hgraph->GetVertexDimensions(), 1.0);
  float halo_width = separation_;

  float chiplet_area = 0.0;
  for (int i = 0; i < hgraph->GetNumVertices(); ++i) {
    float x = 0.0;
    float y = 0.0;
    auto vertex_weight = hgraph->GetVertexWeights(i);
    float chiplet_area =
        std::accumulate(vertex_weight.begin(), vertex_weight.end(), 0.0);
    float width = std::sqrt(chiplet_area);
    float height = chiplet_area / width;
    float min_area = chiplet_area;
    chiplets_.push_back(Chiplet(x, y, width, height, min_area, halo_width));
  }
}

// Run SA for floorplanning
// return a tuple of <vector,vector, bool>
std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, bool>
ChipletRefiner::Floorplanner(int max_steps, int perturbations,
                             float cooling_acceleration_factor, bool local) {
  // we first sweep the cooling rate
  SACore *best_sa = nullptr;
  std::vector<SACore *>
      sa_containers; // store all the SA runs to avoid memory leakage
  float best_cost = std::numeric_limits<float>::max();

  std::vector<int> pos_seq;
  std::vector<int> neg_seq;

  if (local == true) {
    pos_seq = local_pos_seq_;
    neg_seq = local_neg_seq_;
  }

  for (int i = 0; i < num_threads_; i++) {
    float cooling_rate =
        max_cooling_rate_ -
        (max_cooling_rate_ - min_cooling_rate_) * i / num_threads_;
    SACore *sa = new SACore(
        chiplets_, bundled_nets_, area_penalty_weight_, package_penalty_weight_,
        net_penalty_weight_, pos_swap_prob_, neg_swap_prob_, double_swap_prob_,
        resize_prob_, expand_prob_, max_steps, perturbations, cooling_rate,
        init_seed_, pos_seq, neg_seq);
    sa_containers.push_back(sa);
  }

  sa_containers[0]->initialize();
  float norm_area_penalty = sa_containers[0]->getNormAreaPenalty();
  float norm_package_penalty = sa_containers[0]->getNormPackagePenalty();
  float norm_net_penalty = sa_containers[0]->getNormNetPenalty();

  for (auto sa : sa_containers) {
    sa->setNormAreaPenalty(norm_area_penalty);
    sa->setNormPackagePenalty(norm_package_penalty);
    sa->setNormNetPenalty(norm_net_penalty);
  }

  std::vector<std::thread> threads;
  threads.reserve(sa_containers.size());
  for (auto &sa : sa_containers) {
    threads.emplace_back(&chiplet::ChipletRefiner::RunSA, this, std::ref(sa),
                         std::ref(cooling_acceleration_factor));
  }

  for (auto &th : threads) {
    th.join();
  }

  threads.clear();

  int min_cost_sa_id = -1;
  int sa_id = 0;
  for (auto &sa : sa_containers) {
    if (sa->isValid() && sa->getCost() < best_cost) {
      best_cost = sa->getCost();
      best_sa = sa;
    }

    if (sa->getCost() < best_cost) {
      min_cost_sa_id = sa_id;
    }

    sa_id++;
  }

  if (best_sa == nullptr) {
    // logger_->report("Cannot find any valid solution");
    best_sa = sa_containers[min_cost_sa_id];
  } else {
    // logger_->report("Successfully find a valid solution");
  }

  std::vector<Chiplet> best_chiplets;
  best_sa->getMacros(best_chiplets);
  best_sa->getPosSeq(pos_seq);
  best_sa->getNegSeq(neg_seq);
  float best_cooling_rate = best_sa->getCoolingRate();
  best_sa = nullptr;

  // clean up
  for (auto &sa : sa_containers) {
    delete sa;
  }

  std::cout << "Best cooling rate: " << best_cooling_rate << std::endl;

  sa_containers.clear();
  best_cost = std::numeric_limits<float>::max();
  for (int i = 0; i < num_threads_; i++) {
    int seed = init_seed_ + i;
    SACore *sa = new SACore(
        chiplets_, bundled_nets_, area_penalty_weight_, package_penalty_weight_,
        net_penalty_weight_, pos_swap_prob_, neg_swap_prob_, double_swap_prob_,
        resize_prob_, expand_prob_, max_steps, perturbations, best_cooling_rate,
        seed, pos_seq, neg_seq);
    sa_containers.push_back(sa);
  }

  sa_containers[0]->initialize();
  norm_area_penalty = sa_containers[0]->getNormAreaPenalty();
  norm_package_penalty = sa_containers[0]->getNormPackagePenalty();
  norm_net_penalty = sa_containers[0]->getNormNetPenalty();

  for (auto sa : sa_containers) {
    sa->setNormAreaPenalty(norm_area_penalty);
    sa->setNormPackagePenalty(norm_package_penalty);
    sa->setNormNetPenalty(norm_net_penalty);
    sa->setPosSeq(pos_seq);
    sa->setNegSeq(neg_seq);
  }

  threads.clear();
  threads.reserve(sa_containers.size());
  for (auto &sa : sa_containers) {
    threads.emplace_back(&chiplet::ChipletRefiner::RunSA, this, std::ref(sa),
                         std::ref(cooling_acceleration_factor));
  }

  for (auto &th : threads) {
    th.join();
  }

  threads.clear();

  // logger_->report("Total number of SA runs: {}", sa_containers.size());

  std::cout << "Total number of SA runs: " << sa_containers.size() << std::endl;

  min_cost_sa_id = -1;
  sa_id = 0;
  for (auto &sa : sa_containers) {
    if (sa->isValid() && sa->getCost() < best_cost) {
      best_cost = sa->getCost();
      best_sa = sa;
    }

    if (sa->getCost() < best_cost) {
      min_cost_sa_id = sa_id;
    }

    sa_id++;
  }

  if (best_sa == nullptr) {
    // logger_->report("Cannot find any valid solution");
    best_sa = sa_containers[min_cost_sa_id];
    best_sa->checkViolation();
    // return with 2 empty vectors and false
    std::vector<float> null_vec;
    std::vector<float> aspect_ratios;
    std::vector<float> x_locations;
    std::vector<float> y_locations;
    for (auto &chiplet : best_chiplets) {
      aspect_ratios.push_back(chiplet.getRealWidth() / chiplet.getRealHeight());
      x_locations.push_back(chiplet.getRealX());
      y_locations.push_back(chiplet.getRealY());
    }
    best_sa->getPosSeq(pos_seq);
    best_sa->getNegSeq(neg_seq);

    if (local == false) {
      global_pos_seq_ = pos_seq;
      global_neg_seq_ = neg_seq;
    } else {
      local_pos_seq_ = pos_seq;
      local_neg_seq_ = neg_seq;
    }
    return std::make_tuple(aspect_ratios, x_locations, y_locations, false);
  } else {
    // logger_->report("Successfully find a valid solution");
  }

  best_sa->getMacros(best_chiplets);

  best_sa->getPosSeq(pos_seq);
  best_sa->getNegSeq(neg_seq);

  std::vector<float> aspect_ratios;
  std::vector<float> x_locations;
  std::vector<float> y_locations;
  for (auto &chiplet : best_chiplets) {
    aspect_ratios.push_back(chiplet.getRealWidth() / chiplet.getRealHeight());
    x_locations.push_back(chiplet.getRealX());
    y_locations.push_back(chiplet.getRealY());
  }

  if (local == false) {
    global_pos_seq_ = pos_seq;
    global_neg_seq_ = neg_seq;
  } else {
    local_pos_seq_ = pos_seq;
    local_neg_seq_ = neg_seq;
  }

  return std::make_tuple(aspect_ratios, x_locations, y_locations, true);
}

bool ChipletRefiner::CheckFloorPlanFeasible(const HGraphPtr hgraph,
                                            int max_steps, int perturbations,
                                            float cooling_acceleration_factor,
                                            int v,        // vertex id
                                            int to_pid,   // to block id
                                            int from_pid, // from block_id
                                            std::vector<int> &partition) {
  partition[v] = to_pid;
  HGraphPtr chiplet_level_netlist = GenerateNetlist(hgraph, partition);
  partition[v] = from_pid;
  BuildChiplets(chiplet_level_netlist);
  std::cout << "Chiplet level netlist has vertices " << chiplet_level_netlist->GetNumVertices() << std::endl;
  std::cout << "Chiplet level netlist has hyperedges " << chiplet_level_netlist->GetNumHyperedges() << std::endl;
  std::cout << "Size of local pos seq " << local_pos_seq_.size() << std::endl;
  std::cout << "Size of local neg seq " << local_neg_seq_.size() << std::endl;
  auto floor_tuple =
      Floorplanner(max_steps, perturbations, cooling_acceleration_factor, true);
  std::cout << "Floorplanner done" << std::endl;
  return std::get<3>(floor_tuple);
}

// The main function of refinement class
void ChipletRefiner::Refine(const HGraphPtr &hgraph,
                            const Matrix<float> &upper_block_balance,
                            const Matrix<float> &lower_block_balance,
                            Partition &solution) {
  if (max_move_ <= 0) {
    return;
  }
  // calculate the basic statistics of current solution
  Matrix<float> cur_block_balance = GetBlockBalance(hgraph, solution);
  Matrix<int> net_degs = GetNetDegrees(hgraph, solution);
  for (int i = 0; i < refiner_iters_; ++i) {
    // the main function for improving the solution
    // mark the vertices can be moved as unvisited
    std::vector<bool> visited_vertices_flag(hgraph->GetNumVertices(), false);
    auto fp_tuple = Floorplanner(2000, 500, 1.0);
    local_pos_seq_ = global_pos_seq_;
    local_neg_seq_ = global_neg_seq_;
    auto start_time = std::chrono::high_resolution_clock::now();
    std::cout << "FM iteration " << i << " starts" << std::endl;
    const float gain =
        Pass(hgraph, upper_block_balance, lower_block_balance,
             cur_block_balance, net_degs, solution, visited_vertices_flag);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "FM iteration " << i << " takes " << elapsed.count()
              << " seconds" << std::endl;
    ClearLocalSequences();
    ClearGlobalSequences();
    if (gain <= 0.0) {
      return; // stop if there is no improvement
    }
  }
}

bool ChipletRefiner::Terminate(std::deque<float> &history, float &new_cost) {
  if (history.size() < 2) {
    return false;
  }

  return (new_cost == history.back() || new_cost == history.front());
}

float ChipletRefiner::Pass(
    const HGraphPtr &hgraph, const Matrix<float> &upper_block_balance,
    const Matrix<float> &lower_block_balance,
    Matrix<float> &block_balance, // the current block balance
    Matrix<int> &net_degs,        // the current net degree
    Partition &solution, std::vector<bool> &visited_vertices_flag) {
  // initialize the gain buckets
  GainBuckets buckets;
  for (int i = 0; i < num_parts_; ++i) {
    // the maxinum size of each bucket is hgraph->GetNumVertices()
    auto bucket = std::make_shared<PriorityQueue>(
        hgraph->GetNumVertices(), total_corking_passes_, hgraph);
    buckets.push_back(bucket);
  }
  std::vector<int> boundary_vertices =
      FindBoundaryVertices(hgraph, net_degs, visited_vertices_flag);
  if (boundary_vertices.empty()) {
    boundary_vertices.clear();
    boundary_vertices.resize(hgraph->GetNumVertices());
    std::iota(boundary_vertices.begin(), boundary_vertices.end(), 0);
  }

  approx_state_ = 0;
  InitializeGainBucketsKWay(buckets, hgraph, boundary_vertices, net_degs,
                            solution);
  // approx_state_ = 1;
  // exit(EXIT_SUCCESS);
  std::vector<GainCell> moves_trace; // store the moved vertex_gain in sequence
  float total_delta_gain = 0.0;

  float best_gain = 0.0;
  for (int block_id = 0; block_id < num_parts_; block_id++) {
    if (upper_block_balance[block_id] < block_balance[block_id] ||
        block_balance[block_id] < lower_block_balance[block_id]) {
      best_gain = -std::numeric_limits<float>::max();
      break;
    }
  }

  int best_vertex_id = -1; // dummy best vertex id
  int termination_count = 0;
  // main loop of FM pass
  for (int i = 0; i < max_move_; i++) {
    std::cout << "Move " << i << std::endl;
    auto candidate =
        PickMoveKWay(buckets, hgraph, block_balance, upper_block_balance,
                     lower_block_balance, solution);
    std::cout << "Picking done " << std::endl;
    // check the status of candidate
    const int vertex = candidate->GetVertex(); // candidate vertex
    if (vertex < 0) {
      break; // no valid vertex found
    }

    std::cout << "Move vertex " << vertex << " from " << candidate->GetSourcePart()
              << " to " << candidate->GetDestinationPart() << std::endl;

    int from_part = solution[vertex];
    AcceptKWayMove(candidate, buckets, moves_trace, total_delta_gain,
                   visited_vertices_flag, hgraph, block_balance, net_degs,
                   solution);
    legacy_cost_ -= candidate->GetGain();
    int to_part = solution[vertex];
    std::vector<int> neighbors;
    for (auto &v : boundary_vertices) {
      if (visited_vertices_flag[v] == false) {
        neighbors.emplace_back(v);
      }
    }
    /*std::vector<int> direct_nbrs =
        FindNeighbors(hgraph, vertex, visited_vertices_flag, solution,
                      std::pair<int, int>(from_part, to_part));
    // add 50% of direct neighbors to the neighbors
    int num_direct_nbrs = direct_nbrs.size() * 0.5;
    for (int i = 0; i < num_direct_nbrs; i++) {
      neighbors.insert(direct_nbrs[i]);
    }

    std::vector<int> nbrs(neighbors.begin(), neighbors.end());*/

    std::cout << "Update gain buckets" << std::endl;

    for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
      UpdateSingleGainBucket(to_pid, buckets, hgraph, neighbors, net_degs,
                             solution);
    }

    std::cout << "Update gain buckets done" << std::endl;

    if (total_delta_gain > best_gain) {
      best_gain = total_delta_gain;
      best_vertex_id = vertex;
    }
  }

  std::cout << "Best vertex id: " << best_vertex_id << std::endl;

  // find the best solution and restore the status which achieves the best
  // solution traverse the moves_trace in the reversing order
  for (auto move_iter = moves_trace.rbegin(); move_iter != moves_trace.rend();
       move_iter++) {
    // stop when we encounter the best_vertex_id
    auto &vertex_move = *move_iter;
    legacy_cost_ += vertex_move->GetGain();
    if (vertex_move->GetVertex() == best_vertex_id) {
      break; // stop here
    }
    RollBackVertexGain(vertex_move, hgraph, visited_vertices_flag, solution,
                       block_balance, net_degs);
  }

  // clear the move traces
  moves_trace.clear();
  // clear the buckets
  for (auto block_id = 0; block_id < num_parts_; block_id++) {
    buckets[block_id]->Clear();
  }

  std::cout << "Best gain: " << best_gain << std::endl;

  return best_gain;
}

void ChipletRefiner::InitializeGainBucketsKWay(
    GainBuckets &buckets, const HGraphPtr &hgraph,
    const std::vector<int> &boundary_vertices, const Matrix<int> &net_degs,
    const Partition &solution) {
  InitSlopes(num_parts_);
  // print the solution to file
  std::ofstream solution_file("dbg_ptn.txt");
  for (int i = 0; i < solution.size(); i++) {
    solution_file << solution[i] << std::endl;
  }
  solution_file.close();
  for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
    InitializeSingleGainBucket(buckets, to_pid, hgraph, boundary_vertices,
                               net_degs, solution);
  }
}

// Initialize the single bucket
void ChipletRefiner::InitializeSingleGainBucket(
    GainBuckets &buckets,
    int to_pid, // move the vertex into this block (block_id = to_pid)
    const HGraphPtr &hgraph, const std::vector<int> &boundary_vertices,
    const Matrix<int> &net_degs, const Partition &solution) {
  // set current bucket to active
  buckets[to_pid]->SetActive();
  // traverse the boundary vertices
  for (const int &v : boundary_vertices) {
    const int from_part = solution[v];
    if (from_part == to_pid) {
      continue; // the boundary vertex is the current bucket
    }
    auto gain_cell =
        CalculateVertexGain(v, from_part, to_pid, hgraph, solution, net_degs);
    buckets[to_pid]->InsertIntoPQ(gain_cell);
  }
  // if the current bucket is empty, set the bucket to deactive
  if (buckets[to_pid]->GetTotalElements() == 0) {
    buckets[to_pid]->SetDeactive();
  }
}

// Determine which vertex gain to be picked
std::shared_ptr<VertexGain>
ChipletRefiner::PickMoveKWay(GainBuckets &buckets, const HGraphPtr &hgraph,
                             const Matrix<float> &curr_block_balance,
                             const Matrix<float> &upper_block_balance,
                             const Matrix<float> &lower_block_balance,
                             std::vector<int> &partition) {
  // dummy candidate
  int to_pid = -1;
  auto candidate = std::make_shared<VertexGain>();

  // best gain bucket for "corking effect".
  // i.e., if there is no normal candidate available,
  // we will traverse the best_to_pid bucket
  int best_to_pid = -1; // block id with best_gain
  float best_gain = -std::numeric_limits<float>::max();
  auto dummy_cell = std::make_shared<VertexGain>();

  // checking the first elements in each bucket
  for (int i = 0; i < num_parts_; ++i) {
    if (buckets[i]->GetStatus() == false) {
      continue; // This bucket is empty
    }
    auto ele = buckets[i]->GetMax();
    const int vertex = ele->GetVertex();
    const float gain = ele->GetGain();
    const int from_pid = ele->GetSourcePart();

    std::cout << "Checking vertex " << vertex << " from " << from_pid
              << " to " << i << " with gain " << gain << std::endl;

    bool feasible = CheckFloorPlanFeasible(hgraph, 100, 100, 0.00001, vertex, i,
                                           from_pid, partition);
    
    std::cout << "Feasible: " << feasible << std::endl;

    // bool feasible = true;
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
  // Case 1:  if there is a candidate available or no vertex to move
  if (to_pid > -1 || best_to_pid == -1) {
    return candidate;
  }
  // Case 2:  "corking effect", i.e., no candidate
  return buckets.at(best_to_pid)
      ->GetBestCandidate(curr_block_balance, upper_block_balance,
                         lower_block_balance, hgraph);
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
  // Remove vertex from all buckets where vertex is present
  std::vector<std::thread> deletion_threads;
  deletion_threads.reserve(num_parts_);
  for (int i = 0; i < num_parts_; ++i) {
    deletion_threads.emplace_back(&chiplet::ChipletRefiner::HeapEleDeletion,
                                  this, vertex_id, i, std::ref(gain_buckets));
  }
  for (auto &th : deletion_threads) {
    th.join();
  }
}

// Remove vertex from a heap
// Remove the vertex id related vertex gain
void ChipletRefiner::HeapEleDeletion(int vertex_id, int part,
                                     GainBuckets &buckets) const {
  buckets[part]->Remove(vertex_id);
}

// After moving one vertex, the gain of its neighbors will also need
// to be updated. This function is used to update the gain of neighbor vertices
// notices that the neighbors has been calculated based on solution, visited
// status, boundary vertices status
void ChipletRefiner::UpdateSingleGainBucket(int part, GainBuckets &buckets,
                                            const HGraphPtr &hgraph,
                                            const std::vector<int> &neighbors,
                                            const Matrix<int> &net_degs,
                                            const Partition &solution) {
  std::set<int> neighboring_hyperedges;
  for (const int &v : neighbors) {
    const int from_part = solution[v];
    if (from_part == part) {
      continue;
    }
    // recalculate the current gain of the vertex v
    auto gain_cell =
        CalculateVertexGain(v, from_part, part, hgraph, solution, net_degs);
    /*auto gain_cell = CalculateVertexGainApprox(v, from_part, part, hgraph,
                                               solution, net_degs);*/
    // check if the vertex exists in current bucket
    if (buckets[part]->CheckIfVertexExists(v) == true) {
      // update the bucket with new gain
      buckets[part]->ChangePriority(v, gain_cell);
    } else {
      buckets[part]->InsertIntoPQ(gain_cell);
    }
  }
}

// Find all the boundary vertices.
// The boundary vertices do not include fixed verticesa
std::vector<int> ChipletRefiner::FindBoundaryVertices(
    const HGraphPtr &hgraph, const Matrix<int> &net_degs,
    const std::vector<bool> &visited_vertices_flag) const {
  // Step 1 : found all the boundary hyperedges
  std::vector<bool> boundary_net_flag(hgraph->GetNumHyperedges(), false);
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    int num_span_part = 0;
    for (int i = 0; i < num_parts_; i++) {
      if (net_degs[e][i] > 0) {
        num_span_part++;
        if (num_span_part >= 2) {
          boundary_net_flag[e] = true;
          break;
        }
      }
    }
  }
  // Step 2: check all the non-fixed vertices
  std::vector<int> boundary_vertices;
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    if (visited_vertices_flag[v] == true) {
      continue; // This vertex has been visited
    }
    for (const int edge_id : hgraph->Edges(v)) {
      if (boundary_net_flag[edge_id]) {
        boundary_vertices.push_back(v);
        break;
      }
    }
  }
  return boundary_vertices;
}

std::vector<int> ChipletRefiner::FindBoundaryVertices(
    const HGraphPtr &hgraph, const Matrix<int> &net_degs,
    const std::vector<bool> &visited_vertices_flag,
    const std::vector<int> &solution,
    const std::pair<int, int> &partition_pair) const {
  // Step 1 : found all the boundary hyperedges
  std::vector<bool> boundary_net_flag(hgraph->GetNumHyperedges(), false);
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    if (net_degs[e][partition_pair.first] > 0 &&
        net_degs[e][partition_pair.second] > 0) {
      boundary_net_flag[e] = true;
    }
  }
  // Step 2: check all the non-fixed vertices
  std::vector<int> boundary_vertices;
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    if (visited_vertices_flag[v] == true) {
      continue;
    }
    for (const int edge_id : hgraph->Edges(v)) {
      if (boundary_net_flag[edge_id]) {
        boundary_vertices.push_back(v);
        break;
      }
    }
  }
  return boundary_vertices;
}

// Find the neighboring vertices
std::vector<int> ChipletRefiner::FindNeighbors(
    const HGraphPtr &hgraph, const int vertex_id,
    const std::vector<bool> &visited_vertices_flag) const {

  // neighbors comprises of all direct connections and
  // nodes lying on boundaries of the chiplet
  std::set<int> neighbors;
  for (const int e : hgraph->Edges(vertex_id)) {
    for (const int v : hgraph->Vertices(e)) {
      if (visited_vertices_flag[v] == false) {
        // This vertex has not been visited yet
        neighbors.insert(v);
      }
    }
  }
  return std::vector<int>(neighbors.begin(), neighbors.end());
}

// Find the neighboring vertices in specified blocks
std::vector<int>
ChipletRefiner::FindNeighbors(const HGraphPtr &hgraph, const int vertex_id,
                              const std::vector<bool> &visited_vertices_flag,
                              const std::vector<int> &solution,
                              const std::pair<int, int> &partition_pair) const {
  std::set<int> neighbors;
  for (const int e : hgraph->Edges(vertex_id)) {
    for (const int v : hgraph->Vertices(e)) {
      if (visited_vertices_flag[v] == false &&
          (solution[v] == partition_pair.first ||
           solution[v] == partition_pair.second)) {
        // This vertex has not been visited yet
        neighbors.insert(v);
      }
    }
  }
  return std::vector<int>(neighbors.begin(), neighbors.end());
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
  // We assume from_pid == solution[v] when we call CalculateGain
  // we need solution argument to update the score related to path
  float cut_score = 0.0;
  // map path_id to the change of path cost
  if (from_pid == to_pid) { // no gain for this case
    return std::make_shared<VertexGain>(v, from_pid, to_pid, 0.0f);
  }

  // define lambda function
  // for checking connectivity (number of blocks connected by a hyperedge)
  // function : check the connectivity for the hyperedge
  auto GetConnectivity = [&](int e) {
    int connectivity = 0;
    for (auto &num_v : net_degs[e]) {
      if (num_v > 0) {
        connectivity++;
      }
    }
    return connectivity;
  };

  // traverse all the hyperedges connected to v
  for (const int e : hgraph->Edges(v)) {
    const int connectivity = GetConnectivity(e);
    const float e_score = hgraph->GetHyperedgeWeights(e)[0];
    if (connectivity == 0) {
      // ignore the hyperedge consisting of multiple vertices
      // ignore single-vertex hyperedge
      continue;
    }
    if (connectivity == 1 && net_degs[e][from_pid] > 1) {
      // move from_pid to to_pid will have negative score
      // all the vertices are with block from_id
      cut_score -= e_score;
    } else if (connectivity == 2 && net_degs[e][from_pid] == 1 &&
               net_degs[e][to_pid] > 0) {
      // all the vertices excluding v are all within block to_pid
      // move from_pid to to_pid will increase the score
      cut_score += e_score;
    }
  }

  float delta_cost = 0.0;
  if (approx_state_ == 0) {
    delta_cost = GetSingleMoveCost(solution, v, from_pid, to_pid);
    /*float scalar_vertex_weight = hgraph->GetVertexWeights(v)[0];
    float approx_delta_cost =
        GetApproxDelta(scalar_vertex_weight, cut_score, from_pid, to_pid);*/
    /*if (std::abs(delta_cost) > GetCostConfidenceInterval()) {
      delta_cost = GetSingleMoveCost(solution, v, from_pid, to_pid);
    }*/
  } else {
    // get weight of the vertex
    auto vertex_weight = hgraph->GetVertexWeights(v);
    // convert vector of vertex weight to singular scalar value
    float scalar_vertex_weight =
        std::accumulate(vertex_weight.begin(), vertex_weight.end(), 0.0);
    delta_cost =
        GetApproxDelta(scalar_vertex_weight, cut_score, from_pid, to_pid);
    if (std::abs(delta_cost) > GetCostConfidenceInterval()) {
      float base_cost = GetCostAndSlopes(solution);
      delta_cost =
          GetApproxDelta(scalar_vertex_weight, cut_score, from_pid, to_pid);
      if (std::abs(delta_cost) > GetCostConfidenceInterval()) {
        delta_cost = GetSingleMoveCost(solution, v, from_pid, to_pid);
      }
    }
  }

  return std::make_shared<VertexGain>(v, from_pid, to_pid, delta_cost);
}

GainCell ChipletRefiner::CalculateVertexGainApprox(
    int v, int from_pid, int to_pid, const HGraphPtr &hgraph,
    const std::vector<int> &solution, const Matrix<int> &net_degs) {
  // We assume from_pid == solution[v] when we call CalculateGain
  // we need solution argument to update the score related to path
  float cut_score = 0.0;
  // define lambda function
  // for checking connectivity (number of blocks connected by a hyperedge)
  // function : check the connectivity for the hyperedge
  auto GetConnectivity = [&](int e) {
    int connectivity = 0;
    for (auto &num_v : net_degs[e]) {
      if (num_v > 0) {
        connectivity++;
      }
    }
    return connectivity;
  };

  // traverse all the hyperedges connected to v
  for (const int e : hgraph->Edges(v)) {
    const int connectivity = GetConnectivity(e);
    const float e_score = hgraph->GetHyperedgeWeights(e)[0];
    if (connectivity == 0) {
      // ignore the hyperedge consisting of multiple vertices
      // ignore single-vertex hyperedge
      continue;
    }
    if (connectivity == 1 && net_degs[e][from_pid] > 1) {
      // move from_pid to to_pid will have negative score
      // all the vertices are with block from_id
      cut_score -= e_score;
    } else if (connectivity == 2 && net_degs[e][from_pid] == 1 &&
               net_degs[e][to_pid] > 0) {
      // all the vertices excluding v are all within block to_pid
      // move from_pid to to_pid will increase the score
      cut_score += e_score;
    }
  }

  // check if chiplet flag is true
  // if it is true then calculate cost using chiplet evaluator

  // get weight of the vertex
  auto vertex_weight = hgraph->GetVertexWeights(v);
  // convert vector of vertex weight to singular scalar value
  float scalar_vertex_weight =
      std::accumulate(vertex_weight.begin(), vertex_weight.end(), 0.0);
  float delta_cost =
      GetApproxDelta(scalar_vertex_weight, cut_score, from_pid, to_pid);
  if (delta_cost > GetCostConfidenceInterval()) {
    float base_cost = GetCostAndSlopes(solution);
    delta_cost =
        GetApproxDelta(scalar_vertex_weight, cut_score, from_pid, to_pid);
  }
  return std::make_shared<VertexGain>(v, from_pid, to_pid, delta_cost);
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
  // Update the partition balance
  curr_block_balance[pre_part_id] =
      curr_block_balance[pre_part_id] - hgraph->GetVertexWeights(vertex_id);
  curr_block_balance[new_part_id] =
      curr_block_balance[new_part_id] + hgraph->GetVertexWeights(vertex_id);
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
  // Update the partition balance
  curr_block_balance[pre_part_id] =
      curr_block_balance[pre_part_id] + hgraph->GetVertexWeights(vertex_id);
  curr_block_balance[new_part_id] =
      curr_block_balance[new_part_id] - hgraph->GetVertexWeights(vertex_id);
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
  const std::vector<float> total_wt_to_block =
      curr_block_balance[to_pid] + hgraph->GetVertexWeights(v);
  const std::vector<float> total_wt_from_block =
      curr_block_balance[from_pid] - hgraph->GetVertexWeights(v);
  return total_wt_to_block <= upper_block_balance[to_pid] &&
         lower_block_balance[from_pid] <= total_wt_from_block;
}

// calculate the possible gain of moving a entire hyperedge
// We can view the process of moving the vertices in hyperege
// one by one, then restore the moving sequence to make sure that
// the current status is not changed. Solution should not be const
HyperedgeGainPtr ChipletRefiner::CalculateHyperedgeGain(
    int hyperedge_id, int to_pid, const HGraphPtr &hgraph,
    std::vector<int> &solution, const Matrix<int> &net_degs) {
  // if chiplet partitioning is done get the cost
  // from chiplet evaluator
  if (chiplet_flag_ == true) {
    std::vector<int> temp_solution = solution;
    // move all vertices in hyperedge to to_pid
    for (const int v : hgraph->Vertices(hyperedge_id)) {
      if (solution[v] != to_pid) {
        temp_solution[v] = to_pid;
      }
    }
    float score = GetCostFromScratch(temp_solution);
    return std::make_shared<HyperedgeGain>(hyperedge_id, to_pid, score);
  }
}

// accpet the hyperedge gain
void ChipletRefiner::AcceptHyperedgeGain(const HyperedgeGainPtr &hyperedge_gain,
                                         const HGraphPtr &hgraph,
                                         float &total_delta_gain,
                                         std::vector<int> &solution,
                                         Matrix<float> &cur_block_balance,
                                         Matrix<int> &net_degs) const {
  const int hyperedge_id = hyperedge_gain->GetHyperedge();
  total_delta_gain += hyperedge_gain->GetGain();
  // get block id
  const int new_part_id = hyperedge_gain->GetDestinationPart();
  // update the solution vector block_balance and net_degs
  for (const int vertex_id : hgraph->Vertices(hyperedge_id)) {
    const int pre_part_id = solution[vertex_id];
    if (pre_part_id == new_part_id) {
      continue; // the vertex is in current block
    }
    // update solution
    solution[vertex_id] = new_part_id;
    // Update the partition balance
    cur_block_balance[pre_part_id] =
        cur_block_balance[pre_part_id] - hgraph->GetVertexWeights(vertex_id);
    cur_block_balance[new_part_id] =
        cur_block_balance[new_part_id] + hgraph->GetVertexWeights(vertex_id);
    // update net_degs
    // not just this hyperedge, we need to update all the related hyperedges
    for (const int he : hgraph->Edges(vertex_id)) {
      --net_degs[he][pre_part_id];
      ++net_degs[he][new_part_id];
    }
  }
}

bool ChipletRefiner::CheckHyperedgeMoveLegality(
    int e,      // hyperedge id
    int to_pid, // to block id
    const HGraphPtr &hgraph, const std::vector<int> &solution,
    const Matrix<float> &curr_block_balance,
    const Matrix<float> &upper_block_balance,
    const Matrix<float> &lower_block_balance) const {
  Matrix<float> update_block_balance = curr_block_balance;
  for (const int v : hgraph->Vertices(e)) {
    const int pid = solution[v];
    if (solution[v] != to_pid) {
      update_block_balance[to_pid] =
          update_block_balance[to_pid] + hgraph->GetVertexWeights(v);
      update_block_balance[pid] =
          update_block_balance[pid] - hgraph->GetVertexWeights(v);
    }
  }
  // Violate the upper bound
  if (upper_block_balance[to_pid] < update_block_balance[to_pid]) {
    return false;
  }
  // Violate the lower bound
  for (int pid = 0; pid < num_parts_; pid++) {
    if (pid != to_pid) {
      if (update_block_balance[pid] < lower_block_balance[pid]) {
        return false;
      }
    }
  }
  // valid move
  return true;
}

// Cost model specific functions
PyObject *ChipletRefiner::ReadLibraries(std::string io_file,
                                        std::string layer_file,
                                        std::string wafer_process_file,
                                        std::string assembly_process_file,
                                        std::string test_file,
                                        std::string netlist_file) {
  PyObject *libraryDict = PyDict_New();
  PyObject *readModule = PyImport_ImportModule("readDesignFromFile");

  // If readModule is not NULL, continue.
  if (readModule != NULL) {
    // io_file
    PyObject *ioFileArg =
        PyUnicode_DecodeUTF8(io_file.c_str(), io_file.size(), "strict");
    PyObject *ioFunction = NULL;
    PyObject *ioDict = NULL;
    if (ioFileArg == NULL) {
      std::cout << "ioFileArg is NULL, problem with file name" << std::endl;
    } else {
      ioFunction =
          PyObject_GetAttrString(readModule, "io_definition_list_from_file");
      if (ioFunction == NULL) {
        std::cout << "ioFunction is NULL, problem with loading io file read "
                     "function"
                  << std::endl;
      } else {
        ioDict = PyObject_CallFunctionObjArgs(ioFunction, ioFileArg, NULL);
      }
      if (ioDict == NULL) {
        std::cout << "ioDict is NULL, problem with calling io file read "
                     "function"
                  << std::endl;
      } else {
        PyDict_SetItemString(libraryDict, "io", ioDict);
      }
    }
    Py_XDECREF(ioFunction);
    Py_XDECREF(ioFileArg);

    Py_ssize_t numEntries = PyDict_Size(libraryDict);
    PyObject *ioDictObject = PyDict_GetItemString(libraryDict, "io");

    // layer_file
    PyObject *layerFileArg =
        PyUnicode_DecodeUTF8(layer_file.c_str(), layer_file.size(), "strict");
    PyObject *layerFunction = NULL;
    PyObject *layerDict = NULL;
    if (layerFileArg == NULL) {
      std::cout << "layerFileArg is NULL, problem with file name" << std::endl;
    } else {
      layerFunction =
          PyObject_GetAttrString(readModule, "layer_definition_list_from_file");
      if (layerFunction == NULL) {
        std::cout << "layerFunction is NULL, problem with loading layer file "
                     "read function"
                  << std::endl;
      } else {
        layerDict =
            PyObject_CallFunctionObjArgs(layerFunction, layerFileArg, NULL);
      }
      if (layerDict == NULL) {
        std::cout << "layerDict is NULL, problem with calling layer file read "
                     "function"
                  << std::endl;
      } else {
        PyDict_SetItemString(libraryDict, "layer", layerDict);
      }
    }
    Py_XDECREF(layerFunction);
    Py_XDECREF(layerFileArg);
    numEntries = PyDict_Size(libraryDict);
    PyObject *layerObject = PyDict_GetItemString(libraryDict, "layer");

    // wafer_process_file
    PyObject *waferFileArg = PyUnicode_DecodeUTF8(
        wafer_process_file.c_str(), wafer_process_file.size(), "strict");
    PyObject *waferProcessFunction = NULL;
    PyObject *waferProcessDict = NULL;
    if (waferFileArg == NULL) {
      std::cout << "waferFileArg is NULL, problem with file name" << std::endl;
    } else {
      waferProcessFunction = PyObject_GetAttrString(
          readModule, "wafer_process_definition_list_from_file");
      if (waferProcessFunction == NULL) {
        std::cout << "waferProcessFunction is NULL, problem with loading "
                     "wafer process file read function"
                  << std::endl;
      } else {
        waferProcessDict = PyObject_CallFunctionObjArgs(waferProcessFunction,
                                                        waferFileArg, NULL);
      }
      if (waferProcessDict == NULL) {
        std::cout << "waferProcessDict is NULL, problem with calling wafer "
                     "process file read function"
                  << std::endl;
      } else {
        PyDict_SetItemString(libraryDict, "wafer", waferProcessDict);
      }
    }
    // Py_XDECREF(waferProcessDict);
    Py_XDECREF(waferProcessFunction);
    Py_XDECREF(waferFileArg);
    numEntries = PyDict_Size(libraryDict);
    PyObject *waferObject = PyDict_GetItemString(libraryDict, "wafer");

    // assembly_process_file
    PyObject *assemblyFileArg = PyUnicode_DecodeUTF8(
        assembly_process_file.c_str(), assembly_process_file.size(), "strict");
    PyObject *assemblyProcessFunction = NULL;
    PyObject *assemblyProcessDict = NULL;
    if (assemblyFileArg == NULL) {
      std::cout << "assemblyFileArg is NULL, problem with file name"
                << std::endl;
    } else {
      assemblyProcessFunction = PyObject_GetAttrString(
          readModule, "assembly_process_definition_list_from_file");
      if (assemblyProcessFunction == NULL) {
        std::cout << "assemblyProcessFunction is NULL, problem with loading "
                     "assembly process file read function"
                  << std::endl;
      } else {
        assemblyProcessDict = PyObject_CallFunctionObjArgs(
            assemblyProcessFunction, assemblyFileArg, NULL);
      }
      if (assemblyProcessDict == NULL) {
        std::cout << "assemblyProcessDict is NULL, problem with calling "
                     "assembly process file read function"
                  << std::endl;
      } else {
        PyDict_SetItemString(libraryDict, "assembly", assemblyProcessDict);
      }
    }
    Py_XDECREF(assemblyProcessFunction);
    Py_XDECREF(assemblyFileArg);

    numEntries = PyDict_Size(libraryDict);
    PyObject *assemblyObject = PyDict_GetItemString(libraryDict, "assembly");

    // test_file
    PyObject *testFileArg =
        PyUnicode_DecodeUTF8(test_file.c_str(), test_file.size(), "strict");
    PyObject *testFunction = NULL;
    PyObject *testDict = NULL;
    if (testFileArg == NULL) {
      std::cout << "testFileArg is NULL, problem with file name" << std::endl;
    } else {
      testFunction = PyObject_GetAttrString(
          readModule, "test_process_definition_list_from_file");
      if (testFunction == NULL) {
        std::cout << "testFunction is NULL, problem with loading test file "
                     "read function"
                  << std::endl;
      } else {
        testDict =
            PyObject_CallFunctionObjArgs(testFunction, testFileArg, NULL);
      }
      if (testDict == NULL) {
        std::cout << "testDict is NULL, problem with calling test file read "
                     "function"
                  << std::endl;
      } else {
        PyDict_SetItemString(libraryDict, "test", testDict);
      }
    }
    Py_XDECREF(testFunction);
    Py_XDECREF(testFileArg);

    numEntries = PyDict_Size(libraryDict);
    PyObject *testObject = PyDict_GetItemString(libraryDict, "test");

    PyObject *netlistFileNameArg = NULL;
    PyObject *netlistFileArg = NULL;
    PyObject *netlistFunction = NULL;
    PyObject *netlist = NULL;
    if (ioDict == NULL) {
      std::cout << "ioDict is NULL, problem with loading io library"
                << std::endl;
    } else {
      netlistFileNameArg = PyUnicode_DecodeUTF8(netlist_file.c_str(),
                                                netlist_file.size(), "strict");
      if (PyErr_Occurred()) {
        std::cout << "Python error occurred" << std::endl;
        PyErr_Print();
      }
      if (netlistFileNameArg == NULL) {
        std::cout << "netlistFileNameArg is NULL, problem with file name"
                  << std::endl;
      } else {
        netlistFileArg = PyTuple_Pack(2, netlistFileNameArg, ioDict);
        if (netlistFileArg == NULL) {
          std::cout << "netlistFileArg is NULL, problem with creating tuple"
                    << std::endl;
        } else {
          netlistFunction = PyObject_GetAttrString(
              readModule, "global_adjacency_matrix_from_file");
          if (PyErr_Occurred()) {
            std::cout << "Python error occurred" << std::endl;
            PyErr_Print();
          }
          if (netlistFunction == NULL) {
            std::cout << "netlistFunction is NULL, problem with loading "
                         "netlist file read function"
                      << std::endl;
          } else {
            netlist = PyObject_CallFunctionObjArgs(
                netlistFunction, netlistFileNameArg, ioDict, NULL);
            if (PyErr_Occurred()) {
              std::cout << "Python error occurred" << std::endl;
              PyErr_Print();
            }
            if (netlist == NULL) {
              std::cout << "netlist is NULL, problem with calling netlist file "
                           "read function"
                        << std::endl;
            } else {
              PyDict_SetItemString(libraryDict, "netlist", netlist);
              if (PyErr_Occurred()) {
                std::cout << "Python error occurred" << std::endl;
                PyErr_Print();
              }
            }
          }
        }
      }
    }
    Py_XDECREF(netlistFunction);
    Py_XDECREF(netlistFileArg);
    Py_XDECREF(netlistFileNameArg);
    numEntries = PyDict_Size(libraryDict);
    PyObject *netlistObject = PyDict_GetItemString(libraryDict, "netlist");
  }

  Py_XDECREF(readModule);

  return libraryDict;
}

float ChipletRefiner::GetCostAndSlopes(const std::vector<int> &partitionIds) {
  float base_cost = 0.0;
  float base_power = 0.0;
  float cost = 0.0;
  float cost_weight = cost_coefficient_;
  float power = 0.0;
  float power_weight = power_coefficient_;
  // Build the model, but iterate through modifications.
  // In build model we build it once, but we want to create each partition same
  // as in build model as a baseline, then change the area of each partition by
  // a small amount (defined as a percentage of total design area) and get the
  // cost for each change.
  float percentage_area_change_for_slope = 0.01;
  float percentage_power_change_for_slope = 0.01;

  // Get a list of block numbers for each partition.
  Matrix<int> partition_vector =
      std::vector<std::vector<int>>(num_parts_, std::vector<int>());

  for (int block_id = 0; block_id < partitionIds.size(); block_id++) {
    partition_vector[partitionIds[block_id]].push_back(block_id);
  }

  // Get the total area and power for each partition.
  std::vector<float> chiplet_areas;
  std::vector<float> chiplet_powers;
  for (int i = 0; i < num_parts_; ++i) {
    chiplet_areas.push_back(0.0);
    chiplet_powers.push_back(0.0);
  }

  float total_power = 0.0;
  float total_area = 0.0;

  for (int partitionId = 0; partitionId < num_parts_; partitionId++) {
    for (int blockId = 0; blockId < partition_vector[partitionId].size();
         ++blockId) {
      chiplet_areas[partitionId] +=
          chiplet_blocks_[partition_vector[partitionId][blockId]].area *
          AreaScalingFactor(
              chiplet_blocks_[partition_vector[partitionId][blockId]].tech,
              tech_array_[partitionId]);
      chiplet_powers[partitionId] +=
          chiplet_blocks_[partition_vector[partitionId][blockId]].power *
          PowerScalingFactor(
              chiplet_blocks_[partition_vector[partitionId][blockId]].tech,
              tech_array_[partitionId]);
    }
    total_power += chiplet_powers[partitionId];
    total_area += chiplet_areas[partitionId];
  }

  float area_change = total_area * percentage_area_change_for_slope;
  float power_change = total_power * percentage_power_change_for_slope;

  PyObject *block_names = PyList_New(num_parts_);
  for (int partitionId = 0; partitionId < num_parts_; partitionId++) {
    PyObject *block_chiplet_name =
        PyUnicode_DecodeUTF8((std::to_string(partitionId)).c_str(),
                             (std::to_string(partitionId)).size(), "strict");
    PyList_SetItem(block_names, partitionId, block_chiplet_name);
  }

  // Note that the bandwidth check can be implemented by running this from -1 to
  // numPartition*2, but at this point it is not implemented that way.
  for (int partition_id = -1; partition_id < num_parts_; partition_id++) {
    if (partition_id != -1 && partition_id < num_parts_) {
      chiplet_areas[partition_id] += area_change;
    } else if (partition_id >= num_parts_ && partition_id < num_parts_ * 2) {
      chiplet_powers[partition_id - num_parts_] += power_change;
    }
    PyObject *chip = NULL;
    // Initialize connectivity to None.
    PyObject *connectivity = NULL;
    PyObject *connectivity_avebw_tuple = NULL;
    PyObject *average_bandwidth_utilization_combined = NULL;
    // Build the element tree.
    if (Py_IsInitialized()) {
      PyObject *pModule = PyImport_ImportModule("constructChip");
      if (PyErr_Occurred()) {
        std::cout << "Python error occurred" << std::endl;
        PyErr_Print();
      }

      if (pModule != nullptr) {
        PyObject *pFunction =
            PyObject_GetAttrString(pModule, "create_element_tree");
        if (PyErr_Occurred()) {
          std::cout << "Python error occurred" << std::endl;
          PyErr_Print();
        }

        if (PyCallable_Check(pFunction)) {
          // Convert C++ vectors to Python lists
          PyObject *pTechList = PyList_New(tech_array_.size());
          PyObject *pAspectRatioList = PyList_New(aspect_ratios_.size());
          PyObject *pXLocationList = PyList_New(x_locations_.size());
          PyObject *pYLocationList = PyList_New(y_locations_.size());
          PyObject *pPowerList = PyList_New(chiplet_powers.size());
          PyObject *pCoreAreaList = PyList_New(chiplet_areas.size());
          if (PyErr_Occurred()) {
            std::cout << "Python error occurred" << std::endl;
            PyErr_Print();
          }

          for (size_t i = 0; i < chiplet_powers.size(); i++) {
            PyObject *pTech = PyUnicode_DecodeUTF8(
                tech_array_[i].c_str(), tech_array_[i].size(), "strict");
            PyObject *pAspectRatio = PyFloat_FromDouble(aspect_ratios_[i]);
            PyObject *pXLocation = PyFloat_FromDouble(x_locations_[i]);
            PyObject *pYLocation = PyFloat_FromDouble(y_locations_[i]);
            PyObject *pPower = PyFloat_FromDouble(chiplet_powers[i]);
            PyObject *pCoreArea = PyFloat_FromDouble(chiplet_areas[i]);
            PyList_SetItem(pTechList, i, pTech);
            PyList_SetItem(pAspectRatioList, i, pAspectRatio);
            PyList_SetItem(pXLocationList, i, pXLocation);
            PyList_SetItem(pYLocationList, i, pYLocation);
            PyList_SetItem(pPowerList, i, pPower);
            PyList_SetItem(pCoreAreaList, i, pCoreArea);
          }
          if (PyErr_Occurred()) {
            std::cout << "Python error occurred" << std::endl;
            PyErr_Print();
          }

          // Call the Python function with the vectors and number of subtrees
          PyObject *pArgs = PyTuple_Pack(
              7, pTechList, pAspectRatioList, pXLocationList, pYLocationList,
              pPowerList, pCoreAreaList, PyLong_FromLong(num_parts_));
          PyObject *chip_params = PyObject_CallObject(pFunction, pArgs);
          if (PyErr_Occurred()) {
            std::cout << "Python error occurred" << std::endl;
            PyErr_Print();
          }

          PyObject *pModuleRead = PyImport_ImportModule(
              "readDesignFromFile"); // Import your test module
          if (pModuleRead != nullptr) {
            PyObject *pFunctionReadChip =
                PyObject_GetAttrString(pModuleRead, "chip_from_dict");
            PyObject *io_list = PyDict_GetItemString(libraryDicts_, "io");
            PyObject *layer_list = PyDict_GetItemString(libraryDicts_, "layer");
            PyObject *wafer_process_list =
                PyDict_GetItemString(libraryDicts_, "wafer");
            PyObject *assembly_process_list =
                PyDict_GetItemString(libraryDicts_, "assembly");
            PyObject *test_process_list =
                PyDict_GetItemString(libraryDicts_, "test");
            if (PyErr_Occurred()) {
              std::cout << "Python error occurred" << std::endl;
              PyErr_Print();
            }

            PyObject *py_block_combinations =
                PyList_New(partition_vector.size());
            if (PyErr_Occurred()) {
              std::cout << "Python error occurred" << std::endl;
              PyErr_Print();
            }
            for (size_t i = 0; i < partition_vector.size(); ++i) {
              PyObject *py_inner_list = PyList_New(partition_vector[i].size());
              if (PyErr_Occurred()) {
                std::cout << "Python error occurred" << std::endl;
                PyErr_Print();
              }
              for (size_t j = 0; j < partition_vector[i].size(); ++j) {
                PyObject *py_block_id = PyLong_FromLong(partition_vector[i][j]);
                PyList_SetItem(py_inner_list, j, py_block_id);
                if (PyErr_Occurred()) {
                  std::cout << "Python error occurred" << std::endl;
                  PyErr_Print();
                }
              }
              PyList_SetItem(py_block_combinations, i, py_inner_list);
              if (PyErr_Occurred()) {
                std::cout << "Python error occurred" << std::endl;
                PyErr_Print();
              }
            }

            PyObject *pFunctionConnectivity =
                PyObject_GetAttrString(pModule, "combine_blocks");
            if (PyCallable_Check(pFunctionConnectivity)) {
              PyObject *netlist =
                  PyDict_GetItemString(libraryDicts_, "netlist");
              // Set global_netlist to netlist[0]
              PyObject *globalNetlist = PyTuple_GetItem(netlist, 0);
              PyObject *averageBwUtilization = PyTuple_GetItem(netlist, 1);
              PyObject *pArgsConnectivity =
                  PyTuple_Pack(4, globalNetlist, averageBwUtilization,
                               block_names, py_block_combinations);
              connectivity_avebw_tuple =
                  PyObject_CallObject(pFunctionConnectivity, pArgsConnectivity);
              connectivity = PyTuple_GetItem(connectivity_avebw_tuple, 0);
              average_bandwidth_utilization_combined =
                  PyTuple_GetItem(connectivity_avebw_tuple, 1);
              Py_XDECREF(pArgsConnectivity);
              // Py_XDECREF(netlist);
              if (PyErr_Occurred()) {
                std::cout << "Python error occurred" << std::endl;
                PyErr_Print();
              }
              // std::cout << "Got connectivity" << std::endl;
            }

            if (PyCallable_Check(pFunctionReadChip)) {
              PyObject *pChipArgs = PyTuple_Pack(
                  9, chip_params, io_list, layer_list, wafer_process_list,
                  assembly_process_list, test_process_list, connectivity,
                  average_bandwidth_utilization_combined, block_names);
              chip = PyObject_CallObject(pFunctionReadChip, pChipArgs);
              Py_XDECREF(pChipArgs);
            }
            if (PyErr_Occurred()) {
              std::cout << "Python error occurred" << std::endl;
              PyErr_Print();
            }
            Py_XDECREF(pFunctionReadChip);
            Py_XDECREF(pModuleRead);
          }

          Py_XDECREF(pArgs);
          Py_XDECREF(chip_params);
        }
        Py_XDECREF(pFunction);
        Py_XDECREF(pModule);
      }
    }

    // Now "return chip;" would be called. Instead just use the chip object
    // before modifying.
    if (PyObject_HasAttrString(chip, "get_cost")) {
      PyObject *get_cost = PyObject_GetAttrString(chip, "get_cost");
      if (PyCallable_Check(get_cost)) {
        if (partition_id == -1) {
          base_cost =
              PyFloat_AsDouble(PyObject_CallFunctionObjArgs(get_cost, NULL));
        } else if (partition_id > -1 && partition_id < num_parts_) {
          cost = PyFloat_AsDouble(PyObject_CallFunctionObjArgs(get_cost, NULL));
          areaSlopes_[partition_id] = (cost - base_cost) / (area_change);
        }
        if (PyErr_Occurred()) {
          std::cout << "Python error occurred" << std::endl;
          PyErr_Print();
        }
      }
      Py_XDECREF(get_cost);
    }

    if (PyObject_HasAttrString(chip, "get_total_power")) {
      PyObject *get_power = PyObject_GetAttrString(chip, "get_total_power");
      if (PyCallable_Check(get_power)) {
        if (partition_id == -1) {
          base_power =
              PyFloat_AsDouble(PyObject_CallFunctionObjArgs(get_power, NULL));
        } else if (partition_id >= num_parts_ &&
                   partition_id < num_parts_ * 2) {
          power =
              PyFloat_AsDouble(PyObject_CallFunctionObjArgs(get_power, NULL));
          powerAreaSlopes_[partition_id - num_parts_] =
              (power - base_power) / (area_change);
        }
        if (PyErr_Occurred()) {
          std::cout << "Python error occurred" << std::endl;
          PyErr_Print();
        }
      }
      Py_XDECREF(get_power);
    }

    DestroyModel(chip);

    if (partition_id != -1 && partition_id < num_parts_) {
      chiplet_areas[partition_id] -= area_change;
    }

    if (partition_id >= num_parts_ && partition_id < num_parts_ * 2) {
      chiplet_powers[partition_id - num_parts_] -= power_change;
    }
  }

  float max_slope = 0.0;
  for (int partition_id = 0; partition_id < num_parts_; partition_id++) {
    if (areaSlopes_[partition_id] > max_slope) {
      max_slope = areaSlopes_[partition_id];
    }
  }

  costConfidenceInterval_ = max_slope * area_change;

  float max_power_slope = 0.0;
  for (int partition_id = 0; partition_id < num_parts_; partition_id++) {
    if (powerAreaSlopes_[partition_id] > max_power_slope) {
      max_power_slope = powerAreaSlopes_[partition_id];
    }
  }

  powerConfidenceInterval_ = max_power_slope * power_change;

  // Implement bandwidthSlopes_
  // GPIO_external_small has a bw per area of 2000Gb/s/mm^2

  for (int i = 0; i < num_parts_; i++) {
    costBandwidthSlopes_[i] = areaSlopes_[i] / 2000;
    powerBandwidthSlopes_[i] = powerAreaSlopes_[i] / 2000;
  }

  float weighted_cost_metric =
      base_cost * cost_weight + base_power * power_weight;

  base_cost_ = weighted_cost_metric;

  return weighted_cost_metric;
}
// Read file and a vector of blocks.
std::vector<cblock> ChipletRefiner::ReadBlocks(std::string blocks_file) {
  std::vector<cblock> blocks;
  std::ifstream blocksFile(blocks_file);
  std::string line;
  while (std::getline(blocksFile, line)) {
    std::istringstream iss(line);
    std::string name;
    float area;
    float power;
    std::string tech;
    if (!(iss >> name >> area >> power >> tech)) {
      std::cout << "Error reading blocks file" << std::endl;
      break;
    }

    cblock newBlock;
    newBlock.name = name;
    newBlock.area = area;
    newBlock.power = power;
    newBlock.tech = tech;
    blocks.push_back(newBlock);
  }

  return blocks;
}

PyObject *ChipletRefiner::Init(std::string io_file, std::string layer_file,
                               std::string wafer_process_file,
                               std::string assembly_process_file,
                               std::string test_file, std::string netlist_file,
                               std::string blocks_file) {
  Py_Initialize();
  PyRun_SimpleString("import sys\nsys.path.append('.');"); // Add the current
                                                           // directory to the
                                                           // Python path
  PyObject *libraryDicts =
      ReadLibraries(io_file, layer_file, wafer_process_file,
                    assembly_process_file, test_file, netlist_file);
  return libraryDicts;
}

int ChipletRefiner::DestroyDatabase() {
  // Iterate through each PyObject in libraryDicts and Py_XDECREF it.
  Py_XDECREF(libraryDicts_);
  Py_Finalize();
  return 0;
}

// Define a buildModel function that will return a python object that can
// be used in getCostFromScratch.
PyObject *ChipletRefiner::BuildModel(const std::vector<int> &partitionIDs,
                                     const std::vector<std::string> &tech_array,
                                     const std::vector<float> &aspect_ratios,
                                     const std::vector<float> &x_locations,
                                     const std::vector<float> &y_locations) {
  // This should return a chip object.
  // Build a Python nested dictionary of the chip format.
  // Get the largest partition ID in partitionIDs.

  // Get a list of block numbers for each partition.
  std::vector<std::vector<int>> partitionVector =
      std::vector<std::vector<int>>(num_parts_, std::vector<int>());

  for (int blockId = 0; blockId < partitionIDs.size(); blockId++) {
    partitionVector[partitionIDs[blockId]].push_back(blockId);
  }

  // Get the total area and power for each partition.
  std::vector<float> chiplet_areas;
  std::vector<float> chiplet_powers;
  for (int i = 0; i < num_parts_; ++i) {
    chiplet_areas.push_back(0.0);
    chiplet_powers.push_back(0.0);
  }

  float total_area = 0.0;
  float total_power = 0.0;

  for (int partitionId = 0; partitionId < num_parts_; partitionId++) {
    for (int blockId = 0; blockId < partitionVector[partitionId].size();
         ++blockId) {
      chiplet_areas[partitionId] +=
          chiplet_blocks_[partitionVector[partitionId][blockId]].area *
          AreaScalingFactor(
              chiplet_blocks_[partitionVector[partitionId][blockId]].tech,
              tech_array_[partitionId]);
      chiplet_powers[partitionId] +=
          chiplet_blocks_[partitionVector[partitionId][blockId]].power *
          PowerScalingFactor(
              chiplet_blocks_[partitionVector[partitionId][blockId]].tech,
              tech_array_[partitionId]);
    }
    total_area += chiplet_areas[partitionId];
    total_power += chiplet_powers[partitionId];
  }

  // Get block names from the blocks vector and store as a list of python
  // strings in a PyObject*
  PyObject *block_names = PyList_New(num_parts_);
  for (int partitionId = 0; partitionId < num_parts_; partitionId++) {
    PyObject *block_chiplet_name =
        PyUnicode_DecodeUTF8((std::to_string(partitionId)).c_str(),
                             (std::to_string(partitionId)).size(), "strict");
    PyList_SetItem(block_names, partitionId, block_chiplet_name);
  }

  PyObject *chip = NULL;
  // Initialize connectivity to None.
  PyObject *connectivity = NULL;
  PyObject *connectivity_avebw_tuple = NULL;
  PyObject *average_bandwidth_utilization_combined = NULL;
  // Build the element tree.
  if (Py_IsInitialized()) {
    // Import the Python script
    PyObject *pModule = PyImport_ImportModule("constructChip");
    if (PyErr_Occurred()) {
      std::cout << "Python error occurred" << std::endl;
      PyErr_Print();
    }

    if (pModule != nullptr) {
      PyObject *pFunction =
          PyObject_GetAttrString(pModule, "create_element_tree");
      if (PyErr_Occurred()) {
        std::cout << "Python error occurred" << std::endl;
        PyErr_Print();
      }

      if (PyCallable_Check(pFunction)) {
        // Convert C++ vectors to Python lists
        PyObject *pTechList = PyList_New(tech_array_.size());
        PyObject *pAspectRatioList = PyList_New(aspect_ratios_.size());
        PyObject *pXLocationList = PyList_New(x_locations_.size());
        PyObject *pYLocationList = PyList_New(y_locations_.size());
        PyObject *pPowerList = PyList_New(chiplet_powers.size());
        PyObject *pCoreAreaList = PyList_New(chiplet_areas.size());
        if (PyErr_Occurred()) {
          std::cout << "Python error occurred" << std::endl;
          PyErr_Print();
        }

        for (size_t i = 0; i < chiplet_powers.size(); i++) {
          PyObject *pTech = PyUnicode_DecodeUTF8(
              tech_array_[i].c_str(), tech_array_[i].size(), "strict");
          PyObject *pAspectRatio = PyFloat_FromDouble(aspect_ratios_[i]);
          PyObject *pXLocation = PyFloat_FromDouble(x_locations_[i]);
          PyObject *pYLocation = PyFloat_FromDouble(y_locations_[i]);
          PyObject *pPower = PyFloat_FromDouble(chiplet_powers[i]);
          PyObject *pCoreArea = PyFloat_FromDouble(chiplet_areas[i]);
          PyList_SetItem(pTechList, i, pTech);
          PyList_SetItem(pAspectRatioList, i, pAspectRatio);
          PyList_SetItem(pXLocationList, i, pXLocation);
          PyList_SetItem(pYLocationList, i, pYLocation);
          PyList_SetItem(pPowerList, i, pPower);
          PyList_SetItem(pCoreAreaList, i, pCoreArea);
        }
        if (PyErr_Occurred()) {
          std::cout << "Python error occurred" << std::endl;
          PyErr_Print();
        }

        // Call the Python function with the vectors and number of
        // subtrees
        PyObject *pArgs = PyTuple_Pack(
            7, pTechList, pAspectRatioList, pXLocationList, pYLocationList,
            pPowerList, pCoreAreaList, PyLong_FromLong(num_parts_));
        PyObject *chip_params = PyObject_CallObject(pFunction, pArgs);
        if (PyErr_Occurred()) {
          std::cout << "Python error occurred" << std::endl;
          PyErr_Print();
        }

        // Call a different Python function (test_function) and pass the
        // ElementTree
        PyObject *pModuleRead = PyImport_ImportModule("readDesignFromFile");
        if (pModuleRead != nullptr) {
          PyObject *pFunctionReadChip =
              PyObject_GetAttrString(pModuleRead, "chip_from_dict");
          PyObject *io_list = PyDict_GetItemString(libraryDicts_, "io");
          PyObject *layer_list = PyDict_GetItemString(libraryDicts_, "layer");
          PyObject *wafer_process_list =
              PyDict_GetItemString(libraryDicts_, "wafer");
          PyObject *assembly_process_list =
              PyDict_GetItemString(libraryDicts_, "assembly");
          PyObject *test_process_list =
              PyDict_GetItemString(libraryDicts_, "test");
          if (PyErr_Occurred()) {
            std::cout << "Python error occurred" << std::endl;
            PyErr_Print();
          }

          // Get the connectivity between each partition.
          PyObject *py_block_combinations = PyList_New(partitionVector.size());
          if (PyErr_Occurred()) {
            std::cout << "Python error occurred" << std::endl;
            PyErr_Print();
          }
          for (size_t i = 0; i < partitionVector.size(); ++i) {
            PyObject *py_inner_list = PyList_New(partitionVector[i].size());
            if (PyErr_Occurred()) {
              std::cout << "Python error occurred" << std::endl;
              PyErr_Print();
            }
            for (size_t j = 0; j < partitionVector[i].size(); ++j) {
              PyObject *py_block_id = PyLong_FromLong(partitionVector[i][j]);
              PyList_SetItem(py_inner_list, j, py_block_id);
              if (PyErr_Occurred()) {
                std::cout << "Python error occurred" << std::endl;
                PyErr_Print();
              }
            }
            PyList_SetItem(py_block_combinations, i, py_inner_list);
            if (PyErr_Occurred()) {
              std::cout << "Python error occurred" << std::endl;
              PyErr_Print();
            }
          }

          PyObject *pFunctionConnectivity =
              PyObject_GetAttrString(pModule, "combine_blocks");
          if (PyCallable_Check(pFunctionConnectivity)) {
            PyObject *netlist = PyDict_GetItemString(libraryDicts_, "netlist");
            // Set global_netlist to netlist[0]
            PyObject *globalNetlist = PyTuple_GetItem(netlist, 0);
            PyObject *averageBwUtilization = PyTuple_GetItem(netlist, 1);
            PyObject *pArgsConnectivity =
                PyTuple_Pack(4, globalNetlist, averageBwUtilization,
                             block_names, py_block_combinations);
            connectivity_avebw_tuple =
                PyObject_CallObject(pFunctionConnectivity, pArgsConnectivity);
            connectivity = PyTuple_GetItem(connectivity_avebw_tuple, 0);
            average_bandwidth_utilization_combined =
                PyTuple_GetItem(connectivity_avebw_tuple, 1);
            Py_XDECREF(pArgsConnectivity);
            // Py_XDECREF(netlist);
            if (PyErr_Occurred()) {
              std::cout << "Python error occurred" << std::endl;
              PyErr_Print();
            }
          }
          // Py_XDECREF(connectivity);

          if (PyCallable_Check(pFunctionReadChip)) {
            PyObject *pChipArgs = PyTuple_Pack(
                9, chip_params, io_list, layer_list, wafer_process_list,
                assembly_process_list, test_process_list, connectivity,
                average_bandwidth_utilization_combined, block_names);
            chip = PyObject_CallObject(pFunctionReadChip, pChipArgs);
            Py_XDECREF(pChipArgs);
          }
          if (PyErr_Occurred()) {
            std::cout << "Python error occurred" << std::endl;
            PyErr_Print();
          }

          Py_XDECREF(pFunctionReadChip);
          Py_XDECREF(pModuleRead);
        }

        Py_XDECREF(pArgs);
        Py_XDECREF(chip_params);
      }

      Py_XDECREF(pFunction);

      Py_XDECREF(pModule);
    }
  }

  // Return the chip, the new_netlist, and the new_blocknames.
  return chip;
}

int ChipletRefiner::DestroyModel(PyObject *model) {
  // This should destroy the chip object.
  Py_XDECREF(model);
  return 0;
}

float ChipletRefiner::GetApproxDelta(float &delta_area, float &delta_bandwidth,
                                     int &from_pid, int &to_pid) {
  float total_delta_cost =
      areaSlopes_[to_pid] * delta_area - areaSlopes_[from_pid] * delta_area;

  total_delta_cost += costBandwidthSlopes_[from_pid] * delta_bandwidth +
                      costBandwidthSlopes_[to_pid] * delta_bandwidth;

  return total_delta_cost;
}

// 2 versions of a call. Area of blocks, power of blocks, connectivity are
// static. Communicate through vectors. Partition IDs. -> 1 cost output.
// Give a base partition -> cost for moving each block to each other
// partition. (Incremental) Here we represent the partition id through int
// (type) the cost should be float (type)

// We assume we have read the netlist and basic information about blocks
// into the database i.e., area of blocks, power of blocks and
// connectivity So we just need two header functions

// 1. partition ids -> 1 cost output
// We assume the blocks have been ordered in some order
// We assume the partition id and block id both starts with 0
float ChipletRefiner::GetCostFromScratch(const std::vector<int> &partitionIds) {
  float cost = 0.0;
  float cost_weight = cost_coefficient_;
  float power = 0.0;
  float power_weight = power_coefficient_;
  // Build the chip
  PyObject *chip = BuildModel(partitionIds, tech_array_, aspect_ratios_,
                              x_locations_, y_locations_);

  if (PyObject_HasAttrString(chip, "get_cost")) {
    PyObject *get_cost = PyObject_GetAttrString(chip, "get_cost");
    if (PyCallable_Check(get_cost)) {
      cost = PyFloat_AsDouble(PyObject_CallFunctionObjArgs(get_cost, NULL));
      if (PyErr_Occurred()) {
        std::cout << "Python error occurred" << std::endl;
        PyErr_Print();
      }
    }
    Py_XDECREF(get_cost);
  }

  if (PyObject_HasAttrString(chip, "get_total_power")) {
    PyObject *get_power = PyObject_GetAttrString(chip, "get_total_power");
    if (PyCallable_Check(get_power)) {
      power = PyFloat_AsDouble(PyObject_CallFunctionObjArgs(get_power, NULL));
      if (PyErr_Occurred()) {
        std::cout << "Python error occurred" << std::endl;
        PyErr_Print();
      }
    }
    Py_XDECREF(get_power);
  }

  DestroyModel(chip);

  float weighted_cost_metric = cost * cost_weight + power * power_weight;

  return weighted_cost_metric;
}

// Give a base partition -> cost for moving each block to each other
// partition. (Incremental)
std::vector<std::vector<float>>
ChipletRefiner::GetCostIncremental(const std::vector<int> &basePartitionIds) {
  const int numBlocks = basePartitionIds.size();
  // Compute Base Cost
  float baseCost = GetCostFromScratch(basePartitionIds);
  // Compute Incremental Cost for moving each block to each partition.
  // Declare empty vector named costIncremental
  std::vector<std::vector<float>> costIncremental;
  for (int i = 0; i < numBlocks; i++) {
    costIncremental.push_back(std::vector<float>(num_parts_, 0.0));
  }

  // Iterate through all combinations of block and partition.
  for (int blockId = 0; blockId < numBlocks; blockId++) {
    for (int partitionId = 0; partitionId < num_parts_; partitionId++) {
      // Create vector for new partition ids with after the move.
      std::vector<int> newPartitionIds = basePartitionIds;
      newPartitionIds[blockId] = partitionId;

      // Compute incremental cost.
      costIncremental[blockId][partitionId] =
          GetCostFromScratch(newPartitionIds) - baseCost;
    }
  }

  return costIncremental;
}

// Give block Id,  fromPartitionId, toPartitionId -> const
float ChipletRefiner::GetSingleMoveCost(
    const std::vector<int> &basePartitionIds, const int blockId,
    const int fromPartitionId, const int toPartitionId) {
  float cost = 0.0;

  // Create vector for new partition ids with after the move.
  std::vector<int> newPartitionIds = basePartitionIds;
  newPartitionIds[blockId] = toPartitionId;
  cost = legacy_cost_ - GetCostFromScratch(newPartitionIds);
  return cost;
}

HGraphPtr ChipletRefiner::GenerateNetlist(const HGraphPtr hgraph,
                                          const std::vector<int> &partition) {
  std::vector<int> vertex_cluster_id_vec = partition;
  Matrix<float> vertex_weights_c = GetBlockBalance(hgraph, partition);
  Matrix<int> hyperedges_c; // represent each hyperedge as a set of clusters
  Matrix<float> hyperedges_weights_c; // each element represents the weight of
                                      // the clustered hyperedge
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    const auto range = hgraph->Vertices(e);
    const int he_size = range.size();
    if (he_size <= 1) {
      continue; // ignore the single-vertex hyperedge and large hyperedge
    }
    std::set<int> hyperedge_c;
    long long hash = 0;
    for (const int vertex_id : range) {
      hyperedge_c.insert(vertex_cluster_id_vec[vertex_id]); // get cluster id
    }
    if (hyperedge_c.size() <= 1) {
      continue; // ignore the single-vertex hyperedge
    }
    hyperedges_c.push_back(
        std::vector<int>(hyperedge_c.begin(), hyperedge_c.end()));
    hyperedges_weights_c.push_back(hgraph->GetHyperedgeWeights(e));
  }

  HGraphPtr chiplet_level_hgraph = std::make_shared<Hypergraph>(
      hgraph->GetVertexDimensions(), hgraph->GetHyperedgeDimensions(),
      hyperedges_c, vertex_weights_c, hyperedges_weights_c);
  return chiplet_level_hgraph;
}

} // namespace chiplet