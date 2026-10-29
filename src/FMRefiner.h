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

// Include the implementation of Hypergraph to ensure Vertices and Edges are defined
#include "Hypergraph.h"
#include <boost/range/iterator_range.hpp>
#include "PriorityQueue.h"
#include "Utilities.h"
#include "floorplan.h"
#include "evaluator_cpp.h" // Include the cost model evaluator
#include <chrono>
#include <deque>
#include <set>
namespace chiplet {

using Partition = std::vector<int>;
using GainCell = std::shared_ptr<VertexGain>; // for abbreviation

// Build the block-balance bounds consumed by ChipletRefiner::Refine.
//
// Chiplet partitioning is cost-driven, not balance-driven, so the only
// constraint expressed here is a cap on how large a single chiplet may grow:
// every partition may hold at most `max_part_fraction` of the total vertex
// weight (per weight dimension). `max_part_fraction == 1.0` therefore means
// "no balance constraint"; e.g. 0.6 forbids any chiplet from exceeding 60% of
// the total block area. The lower bound is always zero (a partition may empty
// out; the netlist generator drops empty partitions).
inline std::pair<Matrix<float>, Matrix<float>>
BuildBalanceBounds(const HGraphPtr &hgraph, int num_parts,
                   float max_part_fraction) {
  const int dims = hgraph->GetVertexDimensions();
  std::vector<float> upper = hgraph->GetTotalVertexWeights();
  upper.resize(dims, 0.0f);
  const float frac = (max_part_fraction > 0.0f && max_part_fraction <= 1.0f)
                         ? max_part_fraction
                         : 1.0f;
  for (auto &w : upper) {
    w *= frac;
  }
  const int rows = std::max(num_parts, 1);
  return {Matrix<float>(rows, upper),
          Matrix<float>(rows, std::vector<float>(dims, 0.0f))};
}


// Priority-queue based gain bucket
using GainBucket = std::shared_ptr<PriorityQueue>;
using GainBuckets = std::vector<GainBucket>;

struct cblock {
  std::string name;
  float area;
  float power;
  std::string tech;
  bool is_memory;
};

class ChipletRefiner {
public:
  ChipletRefiner(int num_parts, int refiner_iters,
                 int max_move, // the maximum number of vertices or hyperedges
                               // can be moved in each pass
                 std::vector<int> reaches,
                 bool floorplanner = false,
                 const std::string& io_file = "",
                 const std::string& layer_file = "",
                 const std::string& wafer_process_file = "",
                 const std::string& assembly_process_file = "",
                 const std::string& test_file = "",
                 const std::string& netlist_file = "",
                 const std::string& blocks_file = "");

  ChipletRefiner(const ChipletRefiner &) = delete;
  ChipletRefiner(ChipletRefiner &) = delete;
  ~ChipletRefiner();

  void SetBoundary() {
    boundary_flag_ = true;
  }

  void ResetBoundary() {
    boundary_flag_ = false;
  }

  void SetRefinerIters(int refiner_iters) { refiner_iters_ = refiner_iters; }
  void SetMove(int max_move) { max_move_ = max_move; }

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

  // Cost model integration methods
  void SetCostModelFiles(
      const std::string& io_file,
      const std::string& layer_file,
      const std::string& wafer_process_file,
      const std::string& assembly_process_file,
      const std::string& test_file,
      const std::string& netlist_file,
      const std::string& blocks_file);

  bool InitializeCostModel();
  bool IsCostModelInitialized() const { return cost_model_initialized_; }
  
  // Calculate cost gain of moving a vertex between partitions
  float GetSingleMoveCost(
      const std::vector<int> &basePartitionIds, 
      const int blockId,
      const int fromPartitionId, 
      const int toPartitionId);

  float GetCostFromScratch(const std::vector<int> &basePartitionIds, bool approx_state = false) const; 
  // Same evaluation with an explicit per-partition width/height vector
  // instead of the stored aspect_ratios_.
  float CostWithAspectRatios(const std::vector<int> &basePartitionIds,
                             const std::vector<float> &aspect_ratios,
                             bool approx_state = false) const;
  
  // Control the weight of cost model vs connectivity gain
  void SetCostModelWeight(float weight) { cost_model_weight_ = weight; }

  std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, bool>
  RunFloorplanner(std::vector<int> &partition, HGraphPtr hgraph, int max_steps,
                  int perturbations, float cooling_acceleration_factor,
                  bool local = false) {
  
    // Always generate a fresh netlist from the current partition
    chiplet_graph_ = GenerateNetlist(hgraph, partition);
    
    if (!chiplet_graph_) {
      std::cerr << "[ERROR] Failed to generate chiplet netlist in RunFloorplanner" << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Rebuild chiplets based on the new netlist
    BuildChiplets(chiplet_graph_);
    
    if (chiplets_.empty()) {
      std::cerr << "[ERROR] Failed to build chiplets in RunFloorplanner" << std::endl;
      std::vector<float> dummy_result(1, 1.0);
      return std::make_tuple(dummy_result, dummy_result, dummy_result, false);
    }
    
    // Only clear sequences if they're invalid for the current chiplet count
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
    
    // Let Floorplanner() score feasible arrangements with the cost model.
    fp_cost_partition_ = partition;
    auto result = Floorplanner(max_steps, perturbations, cooling_acceleration_factor, local);
    fp_cost_partition_.clear();
    return result;
  }

  Matrix<float> GetBlockBalance(const HGraphPtr hgraph,
                                const Partition &solution) const {
    // Validate input parameters
    if (!hgraph) {
      std::cerr << "Error in GetBlockBalance: Null hypergraph pointer" << std::endl;
      return Matrix<float>(); // Return empty matrix
    }
    
    if (solution.empty()) {
      std::cerr << "Error in GetBlockBalance: Empty solution vector" << std::endl;
      return Matrix<float>(); // Return empty matrix
    }
    
    if (hgraph->GetNumVertices() != solution.size()) {
      std::cerr << "Error in GetBlockBalance: Mismatch between hypergraph vertices (" 
                << hgraph->GetNumVertices() << ") and solution size (" 
                << solution.size() << ")" << std::endl;
      return Matrix<float>(); // Return empty matrix
    }
    
    // Find the maximum partition ID to ensure we allocate enough space
    int max_part_id = -1;
    for (int part_id : solution) {
      if (part_id < 0) {
        std::cerr << "Error in GetBlockBalance: Negative partition ID found: " 
                  << part_id << std::endl;
        return Matrix<float>(); // Return empty matrix
      }
      max_part_id = std::max(max_part_id, part_id);
    }
    
    // Use max_part_id + 1 or num_parts_, whichever is larger
    int num_parts = std::max(max_part_id + 1, num_parts_);
    
    // Initialize block_balance with proper dimensions
    Matrix<float> block_balance(
        num_parts, std::vector<float>(hgraph->GetVertexDimensions(), 0.0));
    
    // Update the block_balance with safety checks
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      int part_id = solution[v];
      
      // Check for invalid partition ID
      if (part_id < 0 || part_id >= num_parts) {
        std::cerr << "Error in GetBlockBalance: Invalid partition ID " << part_id 
                  << " for vertex " << v << std::endl;
        continue; // Skip this vertex
      }
      
      // Get vertex weights with safety check
      const std::vector<float>& vertex_weights = hgraph->GetVertexWeights(v);
      
      // Check if dimensions match
      if (vertex_weights.size() != block_balance[part_id].size()) {
        std::cerr << "Error in GetBlockBalance: Dimension mismatch for vertex " << v 
                  << ": vertex_weights.size()=" << vertex_weights.size() 
                  << ", block_balance[" << part_id << "].size()=" 
                  << block_balance[part_id].size() << std::endl;
        continue; // Skip this vertex
      }
      
      // Manually add weights to avoid using operator+
      for (size_t i = 0; i < vertex_weights.size(); ++i) {
        block_balance[part_id][i] += vertex_weights[i];
      }
    }
    
    return block_balance;
  }

  Matrix<int> GetNetDegrees(const HGraphPtr &hgraph,
                            const Partition &solution) const {
    // Validate input parameters
    if (!hgraph) {
      std::cerr << "Error in GetNetDegrees: Null hypergraph pointer" << std::endl;
      return Matrix<int>(); // Return empty matrix
    }
    
    if (solution.empty()) {
      std::cerr << "Error in GetNetDegrees: Empty solution vector" << std::endl;
      return Matrix<int>(); // Return empty matrix
    }
    
    if (hgraph->GetNumVertices() != solution.size()) {
      std::cerr << "Error in GetNetDegrees: Mismatch between hypergraph vertices (" 
                << hgraph->GetNumVertices() << ") and solution size (" 
                << solution.size() << ")" << std::endl;
      return Matrix<int>(); // Return empty matrix
    }
    
    // Find the maximum partition ID to ensure we allocate enough space
    int max_part_id = -1;
    for (int part_id : solution) {
      if (part_id < 0) {
        std::cerr << "Error in GetNetDegrees: Negative partition ID found: " 
                  << part_id << std::endl;
        return Matrix<int>(); // Return empty matrix
      }
      max_part_id = std::max(max_part_id, part_id);
    }
    
    // Use max_part_id + 1 or num_parts_, whichever is larger
    int num_parts = std::max(max_part_id + 1, num_parts_);
    
    // Initialize net_degs with proper dimensions
    Matrix<int> net_degs(hgraph->GetNumHyperedges(),
                         std::vector<int>(num_parts, 0));
    
    // Update net_degs with safety checks
    for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
      for (const int vertex_id : hgraph->Vertices(e)) {
        // Check for valid vertex ID
        if (vertex_id < 0 || vertex_id >= solution.size()) {
          std::cerr << "Error in GetNetDegrees: Invalid vertex ID " << vertex_id 
                    << " for hyperedge " << e << std::endl;
          continue; // Skip this vertex
        }
        
        int part_id = solution[vertex_id];
        
        // Check for valid partition ID
        if (part_id < 0 || part_id >= num_parts) {
          std::cerr << "Error in GetNetDegrees: Invalid partition ID " << part_id 
                    << " for vertex " << vertex_id << std::endl;
          continue; // Skip this vertex
        }
        
        net_degs[e][part_id]++;
      }
    }
    
    return net_degs;
  }

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

  void BuildChiplets(const HGraphPtr &hgraph);
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

  float Pass(const HGraphPtr &hgraph, const Matrix<float> &upper_block_balance,
             const Matrix<float> &lower_block_balance,
             Matrix<float> &block_balance, // the current block balance
             Matrix<int> &net_degs,        // the current net degree
             Partition &solution, std::vector<bool> &visited_vertices_flag);

  HGraphPtr GenerateNetlist(const HGraphPtr hgraph,
                            const std::vector<int> &partition);

  void SetLegacyCost(float legacyCost) { legacy_cost_ = legacyCost; }

  // Get the total floorplan time
  float GetTotalFloorplanTime() const { return total_fplan_time_; }

  float GetCostModelTime() const { return total_cost_model_time_; }

  void SetFloorplanVerbose(bool verbose) { floorplan_verbose_ = verbose; }

  // ---- Floorplan-feasibility-aware refinement -----------------------------
  // When enabled, every FM move (and every KL swap, via IsFloorplanAcceptable)
  // is followed by a short warm-started floorplan probe of the resulting
  // partition, and each pass is rolled back to the prefix with the lowest
  // reach violation and, among those, the highest gain. Starting from a
  // feasible partition the refinement therefore stays feasible (moves that
  // would break feasibility are skipped); starting from an infeasible one it
  // first follows the cost gains and switches to violation-guided moves when
  // those stop reducing the violation. The best arrangement found is kept as
  // a seed (GetFloorplanSeed) for the full floorplanner.
  void SetFloorplanAware(bool aware) { floorplan_aware_ = aware; }
  bool IsFloorplanAware() const { return floorplan_aware_; }
  // Probe statistics for the current candidate (reset by
  // ResetFloorplanProbeState). `audit_*` are only filled when an audit factor
  // is set: every gate rejection is then re-checked with a probe whose budget
  // is `factor` times larger, and a disagreement counts as a false rejection.
  struct FloorplanProbeStats {
    int probes = 0;             // SA probes run
    int gated = 0;              // probes whose result could veto a move/swap
    int rejected = 0;           // vetoes
    int audit_checked = 0;      // vetoes re-checked with the large probe
    int audit_false = 0;        // ... of which the large probe would have accepted
  };
  const FloorplanProbeStats& GetFloorplanProbeStats() const { return fp_stats_; }
  void SetFloorplanProbeAudit(int factor) { fp_audit_factor_ = std::max(0, factor); }
  // Budget of the per-move probe (steps x perturbations, one SA walker).
  void SetFloorplanCheckBudget(int steps, int perturbs) {
    fp_check_steps_ = std::max(1, steps);
    fp_check_perturbs_ = std::max(1, perturbs);
  }
  // Number of GWTW walkers used by Floorplanner(); 0 = min(threads, 4).
  void SetFloorplanWorkers(int workers) { num_fp_workers_ = workers; }
  // Bounded excursions: an FM pass that starts feasible may accept moves
  // whose reach violation is at most `frac` x (sum of weight x reach of the
  // chiplet graph); the pass result is still restricted to feasible
  // prefixes. 0 = strict gate.
  void SetFloorplanExcursion(float frac) { fp_excursion_frac_ = std::max(0.0f, frac); }
  // Cost-aware floorplanning (RunFloorplanner only): feasible walkers are
  // ranked by the cost model rather than by the SA objective, and the
  // chiplet shapes of every feasible walker are refined against the cost
  // model under the reach constraint before the best one is returned.
  void SetFloorplanCostAware(bool on) { fp_cost_aware_ = on; }
  bool IsFloorplanCostAware() const { return fp_cost_aware_; }
  // Statistics of the last Floorplanner() call.
  struct FloorplanRunStats {
    int walkers = 0;          // walkers run
    int feasible = 0;         // ... that ended (or were restored) feasible
    float sa_pick_cost = 0;   // cost-model cost of the walker the SA objective would pick
    float best_cost = 0;      // cost-model cost of the returned floorplan
    int shape_evals = 0;      // cost evaluations spent in the shape refinement
  };
  const FloorplanRunStats& GetFloorplanRunStats() const { return fp_run_stats_; }
  // Arrangement (sequence pair + height/width ratios) returned by the last
  // Floorplanner() call; saved next to the partition so that evaluation mode
  // can start from it (SetFloorplanSeed) and reproduce the feasibility.
  const FloorplanSeed& GetLastFloorplanResult() const { return fp_last_result_; }
  // Total reach violation (sum over nets of weight x excess length, 0 when
  // feasible) of the best floorplan found by a short probe of `partition`.
  float FloorplanViolation(const HGraphPtr &hgraph, const std::vector<int> &partition);
  // Record `partition` as the current baseline for IsFloorplanAcceptable().
  void SetFloorplanBaseline(const HGraphPtr &hgraph, const std::vector<int> &partition);
  float GetFloorplanBaselineViolation() const { return current_violation_; }
  // True when `partition` does not violate the reach constraints more than
  // the baseline does (always true when not floorplan-aware).
  bool IsFloorplanAcceptable(const HGraphPtr &hgraph, const std::vector<int> &partition);
  // Best floorplan arrangement known for the current partition; seeds the
  // next probe and the full floorplanner (which starts every walker from it
  // whenever its size matches the chiplet count, aware or not).
  const FloorplanSeed& GetFloorplanSeed() const { return fp_warm_; }
  void SetFloorplanSeed(const FloorplanSeed& seed) { fp_warm_ = seed; }
  // Forget the warm start and probe counter (call before refining a new,
  // unrelated partition so results do not depend on what the refiner saw
  // before, e.g. on the thread a candidate landed on).
  void ResetFloorplanProbeState() {
    fp_warm_.clear();
    last_probe_.clear();
    fp_probe_count_ = 0;
    current_violation_ = 0.0f;
    last_probe_violation_ = 0.0f;
    fp_stats_ = FloorplanProbeStats();
  }
  // Make the result of the last probe (IsFloorplanAcceptable / 
  // FloorplanViolation) the new baseline and seed, once the probed partition
  // has been adopted.
  void CommitFloorplanProbe() {
    current_violation_ = last_probe_violation_;
    fp_warm_ = last_probe_;
  }
  void SetGWTWIterations(int iter) { gwtw_iter_ = iter; }
  void SetGWTWMinTemp(float temp) { gwtw_min_temp_ = temp; }
  void SetGWTWTopK(int k) { gwtw_top_k_ = k; }
  void SetGWTWTempDerateFactor(float factor) { gwtw_temp_derate_factor_ = factor; }
  void SetGWTWTopKRatio(const std::vector<float>& ratio) { gwtw_top_k_ratio_ = ratio; }

private:
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

  // Find all the boundary vertices. The boundary vertices will not include any
  // fixed vertices
  // Find all the boundary vertices. The boundary vertices will not include any
  // fixed vertices
  std::vector<int>
  FindBoundaryVertices(const HGraphPtr &hgraph, const Matrix<int> &net_degs,
                       const std::vector<bool> &visited_vertices_flag,
                       float random_non_boundary_ratio = 0.05) const;

  std::vector<int>
  FindBoundaryVertices(const HGraphPtr &hgraph, const Matrix<int> &net_degs,
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

  bool floorplanner_ = true;
  bool chiplet_flag_ = false;
  std::vector<BundledNet> bundled_nets_;
  std::vector<Chiplet> chiplets_;
  HGraphPtr chiplet_graph_ = nullptr;
  float reach_ = 2.0;
  float separation_ = 0.1;
  SACorePtr sa_core_ = nullptr;
  // SA specific:
  // define the parameters here
  // Penalty weights of the normalised SA cost. The floorplanner's job here
  // is primarily to find a REACH-FEASIBLE arrangement (the cost model only
  // consumes aspect ratios / positions and the feasibility flag), so reach
  // violations are weighted well above package area: on an 8-chiplet test
  // instance net weight 10 raised the valid-solution rate ~8x for ~3% more
  // package area than weight 1.
  float area_penalty_weight_ = 1.0;
  float package_penalty_weight_ = 1.0;
  float net_penalty_weight_ = 10.0;
  // Move probabilities. SACore::perturb() uses them as cumulative thresholds,
  // so "expand" receives whatever is left (1 - sum of the other four);
  // expand_prob_ itself is informational. Expansion is kept rare: it is a
  // correct but costly move (it grows chiplet area) and measured roughly
  // neutral for feasibility.
  float pos_swap_prob_ = 0.2375;
  float neg_swap_prob_ = 0.2375;
  float double_swap_prob_ = 0.2375;
  float resize_prob_ = 0.2375;
  float expand_prob_ = 0.05;
  // Print a violation summary when the floorplanner ends infeasible
  // (enabled by SetFloorplanVerbose; off by default because the floorplanner
  // runs many times per partition).
  bool floorplan_verbose_ = false;
  bool floorplan_aware_ = false;
  int fp_check_steps_ = 40;
  int fp_check_perturbs_ = 25;
  int num_fp_workers_ = 0;
  float current_violation_ = 0.0f;       // baseline for IsFloorplanAcceptable
  bool fp_repair_mode_ = false;          // violation-guided move selection (set by Refine)
  float last_probe_violation_ = 0.0f;    // result of the last probe
  FloorplanSeed last_probe_;             // best arrangement found by the last probe
  FloorplanSeed fp_warm_;                // best arrangement for the current partition
  int fp_probe_count_ = 0;               // seeds successive probes
  FloorplanProbeStats fp_stats_;
  int fp_audit_factor_ = 0;              // 0 = no audit
  // Re-check a vetoed partition with a `fp_audit_factor_`x probe without
  // disturbing the probe state (seed, warm start) of the real search.
  void AuditFloorplanRejection(const HGraphPtr &hgraph, const std::vector<int> &partition);
  bool fp_cost_aware_ = true;
  float fp_excursion_frac_ = 0.0f;       // bounded-excursion budget (fraction of fp_reach_scale_)
  float fp_reach_scale_ = 0.0f;          // sum of weight x reach of the current chiplet graph
  std::vector<int> fp_cost_partition_;   // partition being floorplanned (set by RunFloorplanner)
  FloorplanRunStats fp_run_stats_;
  FloorplanSeed fp_last_result_;
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
  std::vector<int> partition_ids_;
  float base_cost_ = 0.0;
  mutable float legacy_cost_ = 0.0;
  float costConfidenceInterval_ = -1.0;
  float powerConfidenceInterval_ = -1.0;
  float cost_coefficient_ = 1.0;
  float power_coefficient_ = 0.0;
  std::vector<std::string> tech_array_;
  std::vector<float> aspect_ratios_;
  std::vector<float> x_locations_;
  std::vector<float> y_locations_;
  bool approx_state_ = 0;
  // tally global runtime
  mutable float total_cost_model_time_ = 0.0;
  float total_fplan_time_ = 0.0;
  // 0 stands for gain bucket initialization
  // 1 stands for gain bucket neighbor update
  HGraphPtr soc_;
  mutable bool boundary_flag_ = false;
  
  // GWTW parameters
  int gwtw_iter_ = 2;
  float gwtw_min_temp_ = 1e-12;
  int gwtw_top_k_ = 2;
  float gwtw_temp_derate_factor_ = 1.0;
  std::vector<float> gwtw_top_k_ratio_ = {0.5, 0.5};
  
  // Cost model related members
  LibraryDicts* libraryDicts_ = nullptr;
  std::string io_file_;
  std::string layer_file_;
  std::string wafer_process_file_;
  std::string assembly_process_file_;
  std::string test_file_;
  std::string netlist_file_;
  std::string blocks_file_;
  bool cost_model_initialized_ = false;
  std::vector<block> blocks_; // Block data populated by readBlocks
  float cost_model_weight_ = 1.0f; // Weight for cost model vs connectivity gain
  float random_non_boundary_ratio_ = 0.05f;    // Ratio of random non-boundary vertices to include
};

using ChipletRefinerPtr = std::shared_ptr<ChipletRefiner>;

} // namespace chiplet