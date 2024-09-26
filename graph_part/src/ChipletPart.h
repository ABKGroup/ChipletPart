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
#include "FMRefiner.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "floorplan.h"
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <string>
#include <tuple>
#include <vector>

namespace chiplet {

class ChipletPart {

public:
  ChipletPart() = default;
  ~ChipletPart() = default;

  void ReadChipletGraph(std::string hypergraph_file,
                        std::string chiplet_io_file);
  void TechAssignPartition(
      std::string hypergraph_file, std::string chiplet_io_file,
      std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file, std::string chiplet_test_file,
      std::string chiplet_netlist_file, std::string chiplet_blocks_file,
      float reach, float separation, std::vector<std::string> techs);
  void Partition(std::string hypergraph_file, std::string chiplet_io_file,
                 std::string chiplet_layer_file,
                 std::string chiplet_wafer_process_file,
                 std::string chiplet_assembly_process_file,
                 std::string chiplet_test_file,
                 std::string chiplet_netlist_file,
                 std::string chiplet_blocks_file, float reach, float separation,
                 std::string tech);

  void EvaluatePartition(
      std::string hypergraph_file, std::string hypergraph_part,
      std::string chiplet_io_file, std::string chiplet_layer_file,
      std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file, std::string chiplet_test_file,
      std::string chiplet_netlist_file, std::string chiplet_blocks_file,
      float reach, float separation, std::string tech);
  void GeneticPart(std::string hypergraph_file, std::string chiplet_io_file,
                   std::string chiplet_layer_file,
                   std::string chiplet_wafer_process_file,
                   std::string chiplet_assembly_process_file,
                   std::string chiplet_test_file,
                   std::string chiplet_netlist_file,
                   std::string chiplet_blocks_file, float reach,
                   float separation, std::vector<std::string> &tech_nodes);

private:
  void CreateMatingPool(const std::vector<std::vector<std::string>> &population,
                        const std::vector<float> &fitness,
                        std::vector<std::vector<std::string>> &mating_pool);
  std::tuple<float, std::vector<int>> InitQuickPart(
      std::string hypergraph_file, std::string chiplet_io_file,
      std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file, std::string chiplet_test_file,
      std::string chiplet_netlist_file, std::string chiplet_blocks_file,
      float reach, float separation, std::vector<std::string> &tech_nodes);
  std::tuple<float, std::vector<int>> QuickPart(
      std::string hypergraph_file, std::string chiplet_io_file,
      std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
      std::string chiplet_assembly_process_file, std::string chiplet_test_file,
      std::string chiplet_netlist_file, std::string chiplet_blocks_file,
      float reach, float separation, std::vector<std::string> &tech_nodes);
  std::vector<int> FindCrossbars(float &quantile);
  void InitPython();
  std::vector<int> METISPart(int &num_parts);
  std::vector<int> KWayCuts(int &num_parts);
  std::vector<int> SpectralPartition(const std::string &file_name);
  std::vector<int> CrossBarExpansion(std::vector<int> &crossbars,
                                     int &num_parts);
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
  std::string path_to_embedding_script_ =
      "/home/fetzfs_projects/TritonPart/bodhi/branch_chiplet/";

  // partition specific
  int num_init_parts_ = 50;
  std::vector<int> chiplets_set_ = {1, 2, 3, 4, 8, 16};
  ChipletRefinerPtr refiner_ = nullptr;

  // genetic algorithm specific
  int population_size_ = 20;   // increase this for more exploration
  int num_generations_ = 10;    // increase this for more exploration
  int gen_threshold_ = 2;      // this will consider the results after the 2nd
                               // generation
                               // increase this for more exploration
  float mutation_rate_ = 0.05; // 5 % mutation rate
  int num_individuals_ = 6;    // number of individuals to be selected for
                               // mating pool
  std::mt19937 rng_;           // random number generator
  // std::mt19937 rng_;
};

using ChipletPartPtr = std::shared_ptr<ChipletPart>;

} // namespace chiplet