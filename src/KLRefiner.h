#pragma once

#include "Hypergraph.h"
#include "PriorityQueue.h"
#include "Utilities.h"
#include "floorplan.h"
#include "FMRefiner.h" // Add this to access ChipletRefiner
#include <vector>
#include <memory>
#include <deque>
#include <chrono>
#include <unordered_map>
#include <string>

namespace chiplet {

using Partition = std::vector<int>;
using GainCell = std::shared_ptr<VertexGain>;
using GainPair = std::pair<int, int>; // Pair of vertices to swap
using KLGain = float; // Gain value for KL algorithm

// Class for Kernighan-Lin refinement
class KLRefiner {
public:
  KLRefiner(int num_parts, 
            int refiner_iters,
            int max_swaps,
            bool floorplanner = false);

  // Non-copyable
  KLRefiner(const KLRefiner&) = delete;
  KLRefiner& operator=(const KLRefiner&) = delete;
  ~KLRefiner() = default;

  // Main refine method - follows same pattern as FMRefiner
  void Refine(const HGraphPtr& hgraph,
              const Matrix<float>& upper_block_balance,
              const Matrix<float>& lower_block_balance,
              Partition& solution);

  // Floorplanner integration - reusing existing framework
  std::tuple<std::vector<float>, std::vector<float>, std::vector<float>, bool>
  RunFloorplanner(std::vector<int>& partition, 
                  HGraphPtr hgraph, 
                  int max_steps,
                  int perturbations, 
                  float cooling_acceleration_factor,
                  bool local = false);

  // Setter methods for parameters
  void SetMaxSwaps(int max_swaps) { max_swaps_ = max_swaps; }
  void SetRefinerIters(int refiner_iters) { refiner_iters_ = refiner_iters; }
  void SetFloorplannerParams(int num_workers, int max_steps, int perturbations) {
    num_fp_workers_ = num_workers;
    max_fp_steps_ = max_steps;
    max_fp_perturbations_ = perturbations;
  }
  
  // Set weight scale factor for normalizing gain calculations
  void SetWeightScaleFactor(float scale) { weight_scale_factor_ = scale; }

  // Set a refiner to use for cost evaluation
  void SetCostEvaluator(std::shared_ptr<ChipletRefiner> evaluator) {
    cost_evaluator_ = evaluator;
  }

  // Performance metrics
  float GetTotalRefineTime() const { return total_refine_time_; }
  float GetTotalFloorplanTime() const { return total_fplan_time_; }

private:
  // Core KL algorithm methods
  float KLPass(const HGraphPtr& hgraph,
               const Matrix<float>& upper_block_balance,
               const Matrix<float>& lower_block_balance, 
               Matrix<float>& block_balance,
               Matrix<int>& net_degs,
               Partition& solution);

  // Calculate gain for swapping a pair of vertices
  KLGain CalculateSwapGain(const HGraphPtr& hgraph,
                          int vertex_a, int block_a,
                          int vertex_b, int block_b,
                          const Partition& solution,
                          const Matrix<int>& net_degs);

  // Find the best swap pair
  GainPair FindBestSwapPair(const HGraphPtr& hgraph,
                            const Matrix<float>& block_balance,
                            const Matrix<float>& upper_block_balance,
                            const Matrix<float>& lower_block_balance,
                            const Partition& solution,
                            const Matrix<int>& net_degs,
                            const std::vector<bool>& locked_vertices);

  // Execute a vertex swap
  void ExecuteSwap(const HGraphPtr& hgraph,
                   int vertex_a, int vertex_b,
                   Matrix<float>& block_balance,
                   Matrix<int>& net_degs,
                   Partition& solution);

  // Check if a swap is legal (respects balance constraints)
  bool IsSwapLegal(int vertex_a, int block_a,
                   int vertex_b, int block_b,
                   const HGraphPtr& hgraph,
                   const Matrix<float>& block_balance,
                   const Matrix<float>& upper_block_balance,
                   const Matrix<float>& lower_block_balance);

  // Generate a chiplet-level netlist for floorplanning
  HGraphPtr GenerateNetlist(const HGraphPtr hgraph, const std::vector<int>& partition);

  // Build chiplets for floorplanning
  void BuildChiplets(const HGraphPtr &hgraph);

  // Member variables
  int num_parts_;
  int refiner_iters_;
  int max_swaps_;
  bool floorplanner_;
  float weight_scale_factor_ = 0.01f; // Default scale factor for gain calculations

  // Floorplanner-specific variables
  std::vector<BundledNet> bundled_nets_;
  std::vector<Chiplet> chiplets_;
  HGraphPtr chiplet_graph_ = nullptr;
  std::vector<int> local_pos_seq_;
  std::vector<int> local_neg_seq_;
  std::vector<int> global_pos_seq_;
  std::vector<int> global_neg_seq_;
  float separation_ = 0.1;
  
  // Floorplanner parameters
  int num_fp_workers_ = 2;
  int max_fp_steps_ = 200;
  int max_fp_perturbations_ = 50;
  float cooling_rate_ = 0.95;
  
  // Performance tracking
  float total_refine_time_ = 0.0;
  float total_fplan_time_ = 0.0;

  // Cost evaluator (borrowed from FM refiner)
  std::shared_ptr<ChipletRefiner> cost_evaluator_ = nullptr;
  
  // Cache for gain calculations to avoid redundant calculations
  std::unordered_map<std::string, float> gain_cache_;
};

using KLRefinerPtr = std::shared_ptr<KLRefiner>;

} // namespace chiplet 