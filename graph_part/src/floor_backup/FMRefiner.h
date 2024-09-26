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
#pragma once

#include "Hypergraph.h"
#include "PriorityQueue.h"
#include "Utilities.h"
#include "floorplan.h"
#include <Python.h>
#include <deque>
#include <set>

namespace chiplet {

using Partition = std::vector<int>;
class VertexGain;
using GainCell = std::shared_ptr<VertexGain>; // for abbreviation

class HyperedgeGain;
using HyperedgeGainPtr = std::shared_ptr<HyperedgeGain>;

// Priority-queue based gain bucket
using GainBucket = std::shared_ptr<PriorityQueue>;
using GainBuckets = std::vector<GainBucket>;

struct cblock {
  std::string name;
  float area;
  float power;
  std::string tech;
};

// Hyperedge Gain.
// Compared to VertexGain, there is no source_part_
// Because this hyperedge spans multiple blocks
class HyperedgeGain {
public:
  HyperedgeGain(int hyperedge_id, int destination_part, float gain);

  float GetGain() const { return gain_; }
  void SetGain(float gain) { gain_ = gain; }

  int GetHyperedge() const { return hyperedge_id_; }

  int GetDestinationPart() const { return destination_part_; }

private:
  const int hyperedge_id_ = -1;
  const int destination_part_ = -1; // the destination block id
  float gain_ = 0.0;

  // The updated DELTA path cost after moving vertex the path_cost
  // will change because we will dynamically update the the weight of
  // the path based on the number of the cut on the path
};

class ChipletRefiner {
public:
  ChipletRefiner(int num_parts, int refiner_iters,
                 int max_move, // the maximum number of vertices or hyperedges
                               // can be moved in each pass
                 std::vector<int> reaches);

  ChipletRefiner(const ChipletRefiner &) = delete;
  ChipletRefiner(ChipletRefiner &) = delete;
  ~ChipletRefiner() = default;

  void SetTechArray(const std::vector<std::string> &tech_array) {
    tech_array_ = tech_array;
  }

  void SetAspectRatios(const std::vector<float> &aspect_ratios) {
    aspect_ratios_ = aspect_ratios;
  }

  void SetXLocations(const std::vector<float> &x_locations) {
    x_locations_ = x_locations;
  }

  void SetYLocations(const std::vector<float> &y_locations) {
    y_locations_ = y_locations;
  }

  void SetNumParts(int num_parts) { num_parts_ = num_parts; }

  void SetReach(float reach) { reach_ = reach; }

  void SetSeparation(float separation) { separation_ = separation; }

  std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, bool>
  RunFloorplanner(std::vector<int> &partition, HGraphPtr hgraph, int max_steps,
                  int perturbations, float cooling_acceleration_factor,
                  bool local = false) {
    auto cnetlist = GenerateNetlist(hgraph, partition);
    BuildChiplets(cnetlist);
    if (local == false) {
      ClearLocalSequences();
      ClearGlobalSequences();
    }
    return Floorplanner(max_steps, perturbations, cooling_acceleration_factor);
  }

  Matrix<float> GetBlockBalance(const HGraphPtr hgraph,
                                const Partition &solution) const {
    Matrix<float> block_balance(
        num_parts_, std::vector<float>(hgraph->GetVertexDimensions(), 0.0));
    // update the block_balance
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      block_balance[solution[v]] =
          block_balance[solution[v]] + hgraph->GetVertexWeights(v);
    }
    return block_balance;
  }
  Matrix<int> GetNetDegrees(const std::vector<int> &partition);

  // Cost model specific
  void InitCostModel(std::string io_file, std::string layer_file,
                     std::string wafer_process_file,
                     std::string assembly_process_file, std::string test_file,
                     std::string netlist_file, std::string blocks_file) {
    libraryDicts_ =
        Init(io_file, layer_file, wafer_process_file, assembly_process_file,
             test_file, netlist_file, blocks_file);
    chiplet_blocks_ = ReadBlocks(blocks_file);
  }

  // Get cost for single partition from scratch.
  float GetCostFromScratch(const std::vector<int> &partitionIds);

  float GetApproxDelta(float &delta_area, float &delta_bandwidth, int &from_pid,
                       int &to_pid);

  // Give a base partition -> cost for moving each block to each other
  // partition. (Incremental)
  std::vector<std::vector<float>>
  GetCostIncremental(const std::vector<int> &basePartitionIds);

  // Give block Id,  fromPartitionId, toPartitionId -> const
  float GetSingleMoveCost(const std::vector<int> &basePartitionIds,
                          const int blockId, const int fromPartitionId,
                          const int toPartitionId);

  float GetCostAndSlopes(const std::vector<int> &partitionIds);

  void InitSlopes(int vec_size) {
    areaSlopes_.resize(vec_size);
    powerAreaSlopes_.resize(vec_size);
    costBandwidthSlopes_.resize(vec_size);
    powerBandwidthSlopes_.resize(vec_size);
    std::fill(areaSlopes_.begin(), areaSlopes_.end(), 0.0);
    std::fill(powerAreaSlopes_.begin(), powerAreaSlopes_.end(), 0.0);
    std::fill(costBandwidthSlopes_.begin(), costBandwidthSlopes_.end(), 0.0);
    std::fill(powerBandwidthSlopes_.begin(), powerBandwidthSlopes_.end(), 0.0);
  }

  void SetBaseCost(float baseCost) { base_cost_ = baseCost; }

  float GetBaseCost() { return base_cost_; }

  float GetCostConfidenceInterval() { return costConfidenceInterval_; }

  float GetPowerConfidenceInterval() { return powerConfidenceInterval_; }

  // Floorplan specific
  void SetLocalSequences(std::vector<int> &pos_seq, std::vector<int> &neg_seq) {
    local_pos_seq_ = pos_seq;
    local_neg_seq_ = neg_seq;
  }

  void ClearLocalSequences() {
    local_pos_seq_.clear();
    local_neg_seq_.clear();
  }

  void SetGlobalSequences(std::vector<int> &pos_seq,
                          std::vector<int> &neg_seq) {
    global_pos_seq_ = pos_seq;
    global_neg_seq_ = neg_seq;
  }

  void ClearGlobalSequences() {
    global_pos_seq_.clear();
    global_neg_seq_.clear();
  }

  std::vector<int> GetLocalPosSeq() const { return local_pos_seq_; }
  std::vector<int> GetLocalNegSeq() const { return local_neg_seq_; }
  std::vector<int> GetGlobalPosSeq() const { return global_pos_seq_; }
  std::vector<int> GetGlobalNegSeq() const { return global_neg_seq_; }

  bool CheckFloorPlanFeasible(const HGraphPtr hgraph, int max_steps,
                              int perturbations,
                              float cooling_acceleration_factor,
                              int v,        // vertex id
                              int to_pid,   // to block id
                              int from_pid, // from block_id
                              std::vector<int> &top_partition);
  void InitFloorPlan(const HGraphPtr hgraph, int max_steps, int perturbations,
                     float cooling_acceleration_factor,
                     std::vector<int> &solution);
  void BuildChiplets(const HGraphPtr &hgraph);
  void RunSA(SACore *sa, float cool) { sa->run(cool); }
  std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, bool>
  Floorplanner(int max_steps, int perturbations,
               float cooling_acceleration_factor, bool local = false);

  // The main function
  void Refine(const HGraphPtr &hgraph, const Matrix<float> &upper_block_balance,
              const Matrix<float> &lower_block_balance, Partition &solution);

  void SetMaxMove(int max_move);
  void SetRefineIters(int refiner_iters);
  void SetReaches(const std::vector<int> &reaches) { reaches_ = reaches; }
  int GetNetReach(int net_id) const { return reaches_[net_id]; }

  void RestoreDefaultParameters();

  float Pass(const HGraphPtr &hgraph, const Matrix<float> &upper_block_balance,
             const Matrix<float> &lower_block_balance,
             Matrix<float> &block_balance, // the current block balance
             Matrix<int> &net_degs,        // the current net degree
             Partition &solution, std::vector<bool> &visited_vertices_flag);

  HGraphPtr GenerateNetlist(const HGraphPtr hgraph,
                            const std::vector<int> &partition);

  void SetLegacyCost(float legacyCost) { legacy_cost_ = legacyCost; }
private:
  bool Terminate(std::deque<float> &history, float &new_cost);
  void InitializeSingleGainBucket(
      GainBuckets &buckets,
      int to_pid, // move the vertex into this block (block_id = to_pid)
      const HGraphPtr &hgraph, const std::vector<int> &boundary_vertices,
      const Matrix<int> &net_degs, const Partition &solution);

  void UpdateSingleGainBucket(int part, GainBuckets &buckets,
                              const HGraphPtr &hgraph,
                              const std::vector<int> &neighbors,
                              const Matrix<int> &net_degs,
                              const Partition &solution);

  // Determine which vertex gain to be picked
  std::shared_ptr<VertexGain>
  PickMoveKWay(GainBuckets &buckets, const HGraphPtr &hgraph,
               const Matrix<float> &curr_block_balance,
               const Matrix<float> &upper_block_balance,
               const Matrix<float> &lower_block_balance,
               std::vector<int> &partition);

  // move one vertex based on the calculated gain_cell
  void AcceptKWayMove(const std::shared_ptr<VertexGain> &gain_cell,
                      GainBuckets &gain_buckets,
                      std::vector<GainCell> &moves_trace,
                      float &total_delta_gain,
                      std::vector<bool> &visited_vertices_flag,
                      const HGraphPtr &hgraph,
                      Matrix<float> &curr_block_balance, Matrix<int> &net_degs,
                      std::vector<int> &solution) const;

  // Remove vertex from a heap
  // Remove the vertex id related vertex gain
  void HeapEleDeletion(int vertex_id, int part, GainBuckets &buckets) const;

  void InitializeGainBucketsKWay(GainBuckets &buckets, const HGraphPtr &hgraph,
                                 const std::vector<int> &boundary_vertices,
                                 const Matrix<int> &net_degs,
                                 const Partition &solution);
  // Cost model specific
  PyObject *Init(std::string io_file, std::string layer_file,
                 std::string wafer_process_file,
                 std::string assembly_process_file, std::string test_file,
                 std::string netlist_file, std::string blocks_file);

  float AreaScalingFactor(std::string initial_tech_node,
                          std::string actual_tech_node) {
    // Define the scaling factors for the area.
    std::vector<std::vector<float>> area_scaling_factors = {
        {1, 0.34, 0.15, 0.08, 0.053, 0.025, 0.011, 0.01, 0.0093, 0.0055,
         0.0032},
        {2.9, 1, 0.44, 0.23, 0.16, 0.072, 0.033, 0.03, 0.027, 0.016, 0.0092},
        {6.6, 2.3, 1, 0.53, 0.35, 0.16, 0.075, 0.067, 0.061, 0.036, 0.021},
        {12, 4.3, 1.9, 1, 0.66, 0.31, 0.14, 0.13, 0.12, 0.068, 0.039},
        {19, 6.4, 2.8, 1.5, 1, 0.46, 0.21, 0.19, 0.17, 0.1, 0.059},
        {40, 14, 6.1, 3.3, 2.2, 1, 0.46, 0.41, 0.38, 0.22, 0.13},
        {88, 30, 13, 7.1, 4.7, 2.2, 1, 0.89, 0.82, 0.48, 0.28},
        {99, 34, 15, 7.9, 5.3, 2.4, 1.1, 1, 0.91, 0.54, 0.31},
        {110, 37, 16, 8.7, 5.8, 2.7, 1.2, 1.1, 1, 0.59, 0.34},
        {180, 63, 28, 15, 9.8, 4.5, 2.1, 1.9, 1.7, 1, 0.58},
        {320, 110, 48, 25, 17, 7.8, 3.6, 3.2, 2.9, 1.7, 1}};

    // Tech Nodes
    std::vector<std::string> tech_nodes = {"180nm", "130nm", "90nm", "65nm",
                                           "45nm",  "32nm",  "20nm", "16nm",
                                           "14nm",  "10nm",  "7nm"};
    // Find the index of the initial_tech_node and actual_tech_node in the
    // tech_nodes vector.
    int initial_index = -1;
    int actual_index = -1;
    for (int i = 0; i < tech_nodes.size(); i++) {
      if (tech_nodes[i] == initial_tech_node) {
        initial_index = i;
      }
      if (tech_nodes[i] == actual_tech_node) {
        actual_index = i;
      }
    }

    // If either of the tech nodes are not found, return -1.
    if (initial_index == -1 || actual_index == -1) {
      std::cout << "Initial or actual tech node not found. Exiting..."
                << std::endl;
      exit(1);
    }
    // Return the scaling factor.
    return area_scaling_factors[initial_index][actual_index];
  }

  float PowerScalingFactor(std::string initial_tech_node,
                           std::string actual_tech_node) {
    // Define the scaling factors for the power.
    std::vector<float> power_scaling_factors = {
        105, 26.1, 13.0, 8.58, 5.19, 2.47, 1.51, 1.28, 0.995, 0.866, 0.789};
    // Tech Nodes
    std::vector<std::string> tech_nodes = {"180nm", "130nm", "90nm", "65nm",
                                           "45nm",  "32nm",  "20nm", "16nm",
                                           "14nm",  "10nm",  "7nm"};
    // Find the index of the initial_tech_node and actual_tech_node in the
    // tech_nodes vector.
    int initial_index = -1;
    int actual_index = -1;
    for (int i = 0; i < tech_nodes.size(); i++) {
      if (tech_nodes[i] == initial_tech_node) {
        initial_index = i;
      }
      if (tech_nodes[i] == actual_tech_node) {
        actual_index = i;
      }
    }
    // If either of the tech nodes are not found, return -1.
    if (initial_index == -1 || actual_index == -1) {
      return -1;
    }
    // Return the scaling factor.
    return power_scaling_factors[actual_index] /
           power_scaling_factors[initial_index];
  }

  PyObject *BuildModel(const std::vector<int> &partitionIDs,
                       const std::vector<std::string> &tech_array,
                       const std::vector<float> &aspect_ratios,
                       const std::vector<float> &x_locations,
                       const std::vector<float> &y_locations);

  PyObject *ReadLibraries(std::string io_file, std::string layer_file,
                          std::string wafer_process_file,
                          std::string assembly_process_file,
                          std::string test_file, std::string netlist_file);

  std::vector<cblock> ReadBlocks(std::string blocks_file);
  int DestroyModel(PyObject *model);
  int DestroyDatabase();

  // Floorplan specific functions

  Matrix<int> GetNetDegrees(const HGraphPtr &hgraph,
                            const Partition &solution) const {
    Matrix<int> net_degs(hgraph->GetNumHyperedges(),
                         std::vector<int>(num_parts_, 0));
    for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
      for (const int vertex_id : hgraph->Vertices(e)) {
        net_degs[e][solution[vertex_id]]++;
      }
    }
    return net_degs;
  }

  // Find all the boundary vertices. The boundary vertices will not include any
  // fixed vertices
  std::vector<int>
  FindBoundaryVertices(const HGraphPtr &hgraph, const Matrix<int> &net_degs,
                       const std::vector<bool> &visited_vertices_flag) const;

  std::vector<int>
  FindBoundaryVertices(const HGraphPtr &hgraph, const Matrix<int> &net_degs,
                       const std::vector<bool> &visited_vertices_flag,
                       const std::vector<int> &solution,
                       const std::pair<int, int> &partition_pair) const;

  std::vector<int>
  FindNeighbors(const HGraphPtr &hgraph, int vertex_id,
                const std::vector<bool> &visited_vertices_flag) const;

  std::vector<int>
  FindNeighbors(const HGraphPtr &hgraph, int vertex_id,
                const std::vector<bool> &visited_vertices_flag,
                const std::vector<int> &solution,
                const std::pair<int, int> &partition_pair) const;

  // Functions related to move a vertex and hyperedge
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
  // cur_paths_cost : current path cost
  // net_degs : current net degrees
  GainCell CalculateVertexGain(int v, int from_pid, int to_pid,
                               const HGraphPtr &hgraph,
                               const std::vector<int> &solution,
                               const Matrix<int> &net_degs);

  GainCell CalculateVertexGainApprox(int v, int from_pid, int to_pid,
                                     const HGraphPtr &hgraph,
                                     const std::vector<int> &solution,
                                     const Matrix<int> &net_degs);

  // accept the vertex gain
  void AcceptVertexGain(const GainCell &gain_cell, const HGraphPtr &hgraph,
                        float &total_delta_gain,
                        std::vector<bool> &visited_vertices_flag,
                        std::vector<int> &solution,
                        Matrix<float> &curr_block_balance,
                        Matrix<int> &net_degs) const;

  // restore the vertex gain
  void RollBackVertexGain(const GainCell &gain_cell, const HGraphPtr &hgraph,
                          std::vector<bool> &visited_vertices_flag,
                          std::vector<int> &solution,
                          Matrix<float> &curr_block_balance,
                          Matrix<int> &net_degs) const;

  // check if we can move the vertex to some block
  bool CheckVertexMoveLegality(int v,        // vertex_id
                               int to_pid,   // to block id
                               int from_pid, // from block id
                               const HGraphPtr &hgraph,
                               const Matrix<float> &curr_block_balance,
                               const Matrix<float> &upper_block_balance,
                               const Matrix<float> &lower_block_balance) const;

  // Calculate the possible gain of moving a entire hyperedge.
  // We can view the process of moving the vertices in hyperege
  // one by one, then restore the moving sequence to make sure that
  // the current status is not changed. Solution should not be const
  // calculate the possible gain of moving a hyperedge
  HyperedgeGainPtr CalculateHyperedgeGain(int hyperedge_id, int to_pid,
                                          const HGraphPtr &hgraph,
                                          std::vector<int> &solution,
                                          const Matrix<int> &net_degs);

  // check if we can move the hyperegde into some block
  bool
  CheckHyperedgeMoveLegality(int e,      // hyperedge id
                             int to_pid, // to block id
                             const HGraphPtr &hgraph,
                             const std::vector<int> &solution,
                             const Matrix<float> &curr_block_balance,
                             const Matrix<float> &upper_block_balance,
                             const Matrix<float> &lower_block_balance) const;

  // accpet the hyperedge gain
  void AcceptHyperedgeGain(const HyperedgeGainPtr &hyperedge_gain,
                           const HGraphPtr &hgraph, float &total_delta_gain,
                           std::vector<int> &solution,
                           Matrix<float> &cur_block_balance,
                           Matrix<int> &net_degs) const;

  // Note that there is no RollBackHyperedgeGain
  // Because we only use greedy hyperedge refinement

  // user specified parameters
  int num_parts_ = 2;     // number of blocks in the partitioning
  int refiner_iters_ = 2; // number of refinement iterations

  // the maxinum number of vertices can be moved in each pass
  int max_move_ = 50;

  // default parameters
  // during partitioning, we may need to update the value
  // of refiner_iters_ and max_move_ for the coarsest hypergraphs
  const int refiner_iters_default_ = 2;
  const int max_move_default_ = 20;
  int total_corking_passes_ = 25;

  bool chiplet_flag_ = false;
  std::vector<BundledNet> bundled_nets_;
  std::vector<Chiplet> chiplets_;
  HGraphPtr chiplet_graph_ = nullptr;
  float reach_ = 2.0;
  float separation_ = 0.1;
  SACorePtr sa_core_ = nullptr;
  // SA specific:
  // define the parameters here
  float area_penalty_weight_ = 1.0;
  float package_penalty_weight_ = 1.0;
  float net_penalty_weight_ = 1.0;
  float pos_swap_prob_ = 0.2;
  float neg_swap_prob_ = 0.2;
  float double_swap_prob_ = 0.2;
  float resize_prob_ = 0.2;
  float expand_prob_ = 0.2;
  int max_num_step_ = 2000;
  int num_perturb_per_step_ = 500;
  int num_oscillations_ = 4; // number of oscillations allowed in FM
  int num_threads_ = 10;
  unsigned init_seed_ = 0;
  float max_cooling_rate_ = 0.99;
  float min_cooling_rate_ = 0.9;
  std::vector<int> reaches_;
  std::vector<int> local_pos_seq_;
  std::vector<int> local_neg_seq_;
  std::vector<int> global_pos_seq_;
  std::vector<int> global_neg_seq_;
  // chiplet specific data structures
  PyObject *libraryDicts_;
  std::vector<cblock> chiplet_blocks_;
  std::vector<int> partition_ids_;
  float base_cost_ = 0.0;
  float legacy_cost_ = 0.0;
  float costConfidenceInterval_ = -1.0;
  float powerConfidenceInterval_ = -1.0;
  float cost_coefficient_ = 1.0;
  float power_coefficient_ = 1.0;
  std::vector<float> areaSlopes_;
  std::vector<float> powerAreaSlopes_;
  std::vector<float> costBandwidthSlopes_;
  std::vector<float> powerBandwidthSlopes_;
  std::vector<std::string> tech_array_;
  std::vector<float> aspect_ratios_;
  std::vector<float> x_locations_;
  std::vector<float> y_locations_;
  bool approx_state_ = 0;
  // 0 stands for gain bucket initialization
  // 1 stands for gain bucket neighbor update
  HGraphPtr soc_;
};

using ChipletRefinerPtr = std::shared_ptr<ChipletRefiner>;

} // namespace chiplet