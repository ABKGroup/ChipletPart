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
//#include "FMRefiner.h"
#include "Hypergraph.h"
#include "Utilities.h"
//#include "floorplan.h"

#ifndef DISABLE_METIS
#include <metis.h>
#endif

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <map>
#include <queue>
#include <random>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_set>

// Add Eigen headers
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

namespace chiplet {

class ChipletPart {

public:
  ChipletPart();  // Default constructor
  
  // Constructor that accepts a seed
  ChipletPart(int seed) : seed_(seed) {
    rng_.seed(seed_);
  }
  
  ~ChipletPart() = default;

  // Set the random seed for reproducible results
  void SetSeed(int seed) { 
    seed_ = seed; 
    rng_.seed(seed_);
    std::cout << "[INFO] Random seed set to " << seed_ << std::endl;
  }
  
  // Get the current random seed
  int GetSeed() const { return seed_; }

  // Original method that reads from a hypergraph file
  void ReadChipletGraph(std::string hypergraph_file,
                        std::string chiplet_io_file);
                        
  // New method that creates a hypergraph from XML files directly
  void ReadChipletGraphFromXML(std::string chiplet_io_file,
                              std::string chiplet_netlist_file,
                              std::string chiplet_blocks_file);

  void TechAssignPartition(
      std::string chiplet_io_file,
      std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file, std::string chiplet_test_file,
      std::string chiplet_netlist_file, std::string chiplet_blocks_file,
      float reach, float separation, std::vector<std::string> techs);
      
  void Partition(std::string chiplet_io_file,
                 std::string chiplet_layer_file,
                 std::string chiplet_wafer_process_file,
                 std::string chiplet_assembly_process_file,
                 std::string chiplet_test_file,
                 std::string chiplet_netlist_file,
                 std::string chiplet_blocks_file, float reach, float separation,
                 std::string tech);

  void EvaluatePartition(
      std::string hypergraph_part,
      std::string chiplet_io_file, std::string chiplet_layer_file,
      std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file, std::string chiplet_test_file,
      std::string chiplet_netlist_file, std::string chiplet_blocks_file,
      float reach, float separation, std::string tech);
      
  void GeneticPart(std::string chiplet_io_file,
                   std::string chiplet_layer_file,
                   std::string chiplet_wafer_process_file,
                   std::string chiplet_assembly_process_file,
                   std::string chiplet_test_file,
                   std::string chiplet_netlist_file,
                   std::string chiplet_blocks_file, float reach,
                   float separation, std::vector<std::string> &tech_nodes);
  
  // Enhanced genetic algorithm for co-optimizing partitioning and technology assignment
  void GeneticTechPart(std::string chiplet_io_file,
                      std::string chiplet_layer_file,
                      std::string chiplet_wafer_process_file,
                      std::string chiplet_assembly_process_file,
                      std::string chiplet_test_file,
                      std::string chiplet_netlist_file,
                      std::string chiplet_blocks_file, 
                      float reach,
                      float separation, 
                      std::vector<std::string> &tech_nodes,
                      int population_size = 50,
                      int num_generations = 50,
                      float mutation_rate = 0.2,
                      float crossover_rate = 0.7,
                      int min_partitions = 2,
                      int max_partitions = 8,
                      std::string output_prefix = "genetic_tech_part");

  // New method for quick evaluation of a technology assignment in the canonical GA
  void QuickTechPartition(
      std::string chiplet_io_file,
      std::string chiplet_layer_file,
      std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file,
      std::string chiplet_test_file,
      std::string chiplet_netlist_file,
      std::string chiplet_blocks_file, 
      float reach,
      float separation, 
      std::vector<std::string> &tech_nodes,
      std::string output_prefix = "quick_tech_part");
      
  // Method to run the canonical GA for technology assignment
  void CanonicalGeneticTechPart(
      std::string chiplet_io_file,
      std::string chiplet_layer_file,
      std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file,
      std::string chiplet_test_file,
      std::string chiplet_netlist_file,
      std::string chiplet_blocks_file, 
      float reach,
      float separation, 
      std::vector<std::string> &tech_nodes,
      int population_size = 50,
      int num_generations = 50,
      float mutation_rate = 0.2,
      float crossover_rate = 0.7,
      int min_partitions = 2,
      int max_partitions = 8,
      std::string output_prefix = "canonical_ga");

  // Advanced partitioning methods made available for external use
  std::vector<int> METISPart(int &num_parts);
  std::vector<int> SpectralPartition();
  std::vector<int> KWayCuts(int &num_parts);
  std::vector<int> KWayCutsParallel(int &num_parts);

  /// Get the number of vertices in the hypergraph
  int GetNumVertices() const {
    if (hypergraph_) {
      return hypergraph_->GetNumVertices();
    }
    return 0;
  }

  // New method for evaluating technology partitioning for the genetic algorithm
  std::tuple<float, std::vector<int>> EvaluateTechPartition(
      std::string chiplet_io_file,
      std::string chiplet_layer_file,
      std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file,
      std::string chiplet_test_file,
      std::string chiplet_netlist_file,
      std::string chiplet_blocks_file, 
      float reach,
      float separation, 
      const std::vector<std::string>& tech_assignment);

  // Enumerate all canonical technology assignments and find the best one
  std::tuple<float, std::vector<int>, std::vector<std::string>> EnumerateTechAssignments(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, 
    std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, 
    std::string chiplet_test_file,
    std::string chiplet_netlist_file, 
    std::string chiplet_blocks_file,
    float reach, 
    float separation, 
    const std::vector<std::string>& available_tech_nodes,
    int max_partitions,
    bool detailed_output = false);

private:
  // Helper method that converts XML files to hypergraph representation
  void ConvertXMLToHypergraph(const std::string& netlist_file,
                             const std::string& block_def_file);
                             
  void CreateMatingPool(const std::vector<std::vector<std::string>> &population,
                        const std::vector<float> &fitness,
                        std::vector<std::vector<std::string>> &mating_pool);
  std::tuple<float, std::vector<int>> InitQuickPart(
      std::string chiplet_io_file,
      std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file, std::string chiplet_test_file,
      std::string chiplet_netlist_file, std::string chiplet_blocks_file,
      float reach, float separation, std::vector<std::string> &tech_nodes);
      
  std::tuple<float, std::vector<int>> QuickPart(
      std::string chiplet_io_file,
      std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file, std::string chiplet_test_file,
      std::string chiplet_netlist_file, std::string chiplet_blocks_file,
      float reach, float separation, std::vector<std::string> &tech_nodes);
      
  std::vector<int> FindCrossbars(float &quantile);
  std::vector<int> CrossBarExpansion(std::vector<int> &crossbars,
                                     int &num_parts);
  std::vector<int> kMeansClustering(const Eigen::MatrixXd& embedding, int k);
  // Parallel version of kMeansClustering for large datasets
  std::vector<int> kMeansClusteringParallel(const Eigen::MatrixXd& embedding, int k);
  // Helper methods for CrossBarExpansion
  void processNeighbors(int vertex, int partition,
                      std::vector<std::unordered_set<int>>& partitions,
                      std::vector<std::queue<int>>& queues,
                      std::unordered_map<int, int>& vertex_to_partition,
                      std::vector<std::unordered_map<int, int>>& edge_counts);
  
  // Parallel version of processNeighbors for large graphs
  void processNeighborsParallel(int vertex, int partition,
                             std::vector<std::unordered_set<int>>& partitions,
                             std::vector<std::queue<int>>& queues,
                             std::unordered_map<int, int>& vertex_to_partition,
                             std::vector<std::unordered_map<int, int>>& edge_counts);
                      
  bool shouldAddToPartition(int vertex, 
                          const std::unordered_map<int, int>& edge_counts,
                          const std::unordered_map<int, int>& vertex_to_partition);
                          
  void assignRemainingVertices(std::vector<int>& partition, 
                             const std::unordered_map<int, int>& vertex_to_partition,
                             int num_parts);
                             
  // Parallel version of assignRemainingVertices for large graphs
  void assignRemainingVerticesParallel(std::vector<int>& partition, 
                                    const std::unordered_map<int, int>& vertex_to_partition,
                                    int num_parts);
  float ub_factor_ = 1.0; // balance constraint
  int num_parts_ = 8;     // number of partitions
  int refine_iters_ = 2;  // number of refinement iterations
  int max_moves_ = 50;
  // for technology aware partitioning
  int tech_parts_;
  // random seed
  int seed_ = 0;
  // Hypergraph information
  // basic information
  std::vector<std::vector<int>> hyperedges_;
  int num_vertices_ = 0;
  int num_hyperedges_ = 0;
  int vertex_dimensions_ = 1;    // specified in the hypergraph
  int hyperedge_dimensions_ = 1; // specified in the hypergraph
  std::vector<std::vector<float>> vertex_weights_;
  std::vector<std::vector<float>> hyperedge_weights_;
  // When we create the hypergraph, we ignore all the hyperedges with vertices
  // more than global_net_threshold_
  HGraphPtr hypergraph_ =
      nullptr; // the hypergraph after removing large hyperedges
  // Final solution
  std::vector<int> solution_; // store the part_id for each vertex
  // Create a hash map to store io type and reach value
  std::unordered_map<std::string, float> io_map_;
  std::vector<float> reach_;
  std::vector<float> io_sizes_;
  // Map to store vertex names (for debugging and output)
  std::unordered_map<int, std::string> vertex_index_to_name_;
  std::unordered_map<std::string, int> vertex_name_to_index_;
  // partition specific
  int num_init_parts_ = 50;
  std::vector<int> chiplets_set_ = {1, 2, 3, 4, 5, 6, 7, 8};
  //ChipletRefinerPtr refiner_ = nullptr;

  // genetic algorithm specific
  int population_size_ = 10;   // increase this for more exploration
  int num_generations_ = 20;    // increase this for more exploration
  int hard_stop_ = num_generations_ / 2;
  int gen_threshold_ = 8;      // this will consider the results after the 2nd
                               // generation
                               // increase this for more exploration
  float mutation_rate_ = 0.025; // mutation rate
  int num_individuals_ = 6;    // number of individuals to be selected for
                               // mating pool
  std::mt19937 rng_;           // random number generator
  // std::mt19937 rng_;
};

using ChipletPartPtr = std::shared_ptr<ChipletPart>;

} // namespace chiplet