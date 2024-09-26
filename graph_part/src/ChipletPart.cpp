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
#include "ChipletPart.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "pugixml.hpp"
#include <Python.h>
#include <omp.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <chrono>
#include <codecvt>
#include <filesystem>
#include <future> // For parallel fitness evaluation
#include <iomanip>
#include <iostream>
#include <locale>
#include <metis.h>
#include <numpy/arrayobject.h>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// write the main here (accept arguments from command line)
namespace chiplet {

std::vector<int> ChipletPart::CrossBarExpansion(std::vector<int> &crossbars,
                                                int &num_parts) {
  if (crossbars.size() < num_parts) {
    return std::vector<int>();
  }

  // round robin assignment of crossbars to partitions
  std::vector<std::unordered_set<int>> partitions(num_parts);
  std::vector<std::queue<int>> queues(num_parts);
  std::unordered_map<int, int> node_partition_map;
  std::vector<std::unordered_map<int, int>> edge_count(
      hypergraph_->GetNumVertices());

  // Initialize partitions and queues with crossbars
  for (int i = 0; i < num_parts; i++) {
    int partition = i % num_parts;
    int crossbar = crossbars[i];
    partitions[partition].insert(crossbar);
    queues[partition].push(crossbar);
    node_partition_map[crossbar] = partition;
  }

  auto ShouldAddToPartition =
      [](const std::unordered_map<int, int> &edge_counts,
         const std::unordered_map<int, int> &node_partition_map,
         int node) -> bool {
    if (node_partition_map.find(node) != node_partition_map.end()) {
      return false;
    }

    int maxEdges = 0;
    int maxPartition = -1;

    // Find partition with the maximum edge count to this node
    for (const auto &pair : edge_counts) {
      if (pair.second > maxEdges) {
        maxEdges = pair.second;
        maxPartition = pair.first;
      }
    }

    // Only add the node to this partition if it has the majority of its edges
    // here
    return edge_counts.at(maxPartition) > maxEdges / 2;
  };

  auto AllQueuesEmpty = [](const std::vector<std::queue<int>> &queues) -> bool {
    return all_of(queues.begin(), queues.end(),
                  [](const std::queue<int> &q) { return q.empty(); });
  };

  // BFS expansion with weight-based partitioning
  while (!AllQueuesEmpty(queues)) {
    for (int i = 0; i < num_parts; ++i) {
      if (!queues[i].empty()) {
        int current_node = queues[i].front();
        queues[i].pop();

        // get the neighbors of the current node
        std::vector<int> neighbors = hypergraph_->GetNeighbors(current_node);
        for (int neighbor : neighbors) {
          if (partitions[i].count(neighbor) == 0) {
            edge_count[neighbor][i]++;
            if (ShouldAddToPartition(edge_count[neighbor], node_partition_map,
                                     neighbor)) {
              partitions[i].insert(neighbor);
              queues[i].push(neighbor);
              node_partition_map[neighbor] = i;
            }
          }
        }
      }
    }
  }

  std::vector<int> partition(hypergraph_->GetNumVertices());
  for (int i = 0; i < num_parts; i++) {
    for (int node : partitions[i]) {
      partition[node] = i;
    }
  }

  return partition;
}

std::vector<int> ChipletPart::SpectralPartition(const std::string &file_name) {
  // run the python script get_embedding.py
  std::vector<int> partition;
  Py_Initialize();
  // Initialize NumPy array API
  if (_import_array() < 0) {
    PyErr_Print();
    PyErr_SetString(PyExc_ImportError,
                    "numpy.core.multiarray failed to import");
    // Handle error: return an empty vector or throw an exception
    return std::vector<int>(); // Return an empty vector as an error indicator
  }
  if (!Py_IsInitialized()) {
    std::cerr << "Failed to initialize the Python interpreter." << std::endl;
    return partition;
  }

  PyObject *sysPath = PySys_GetObject("path");
  PyList_Append(sysPath,
                PyUnicode_FromString(path_to_embedding_script_.c_str()));
  PyObject *pName = PyUnicode_FromString("get_embedding");
  PyObject *pModule = PyImport_Import(pName);
  Py_DECREF(pName);

  if (!pModule) {
    PyErr_Print();
    std::cerr << "Failed to load the Python module." << std::endl;
    return partition;
  }

  PyObject *pFunc =
      PyObject_GetAttrString(pModule, "get_clusters_from_embedding");
  if (!pFunc || !PyCallable_Check(pFunc)) {
    PyErr_Print();
    std::cerr << "Cannot find function 'get_clusters_from_embedding'."
              << std::endl;
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
    return partition;
  }

  PyObject *pArgs = PyTuple_New(1);
  PyObject *pValue = PyUnicode_FromString(file_name.c_str());
  PyTuple_SetItem(
      pArgs, 0,
      pValue); // PyTuple_SetItem steals a reference, no need to decref pValue

  PyObject *pResult = PyObject_CallObject(pFunc, pArgs);
  Py_DECREF(pArgs);
  if (!pResult) {
    PyErr_Print();
    std::cerr << "Call to 'get_clusters_from_embedding' failed." << std::endl;
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
    return partition;
  }

  // Ensure pResult is an array and check its type
  if (PyArray_Check(pResult) &&
      PyArray_TYPE((PyArrayObject *)pResult) == NPY_INT) {
    PyArrayObject *arr = reinterpret_cast<PyArrayObject *>(pResult);
    int *data = static_cast<int *>(PyArray_DATA(arr));
    int size = PyArray_SIZE(arr);

    // Copy data to a vector (if necessary)
    partition.assign(data, data + size);
  } else {
    std::cerr << "Returned object is not an int array." << std::endl;
  }

  // Clean up
  Py_DECREF(pResult);
  Py_DECREF(pFunc);
  Py_DECREF(pModule);

  return partition; // Return the vector, even if empty
}

std::vector<int> ChipletPart::KWayCuts(int &num_parts) {
  std::vector<int> partition(hypergraph_->GetNumVertices(), 0);
  std::vector<int> partition_sizes(num_parts, 0);
  int average_size = hypergraph_->GetNumVertices() / num_parts;
  // seed the random number generator
  srand(time(NULL));

  // Random initial partitioning
  for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
    partition[i] = rand() % num_parts;
    partition_sizes[partition[i]]++;
  }

  // Attempt to balance the partitions
  bool balanced = false;
  while (!balanced) {
    balanced = true;
    for (int i = 0; i < hypergraph_->GetNumVertices(); ++i) {
      int current_partition = partition[i];
      if (partition_sizes[current_partition] > average_size) {
        for (int j = 0; j < num_parts; ++j) {
          if (partition_sizes[j] < average_size) {
            partition[i] = j;
            partition_sizes[current_partition]--;
            partition_sizes[j]++;
            balanced = false;
            break;
          }
        }
      }
    }
  }

  return partition;
}

std::vector<int> ChipletPart::METISPart(int &num_parts) {
  // create the METIS file
  std::string graph_file = "metis_graph.txt";
  std::ofstream metis_file(graph_file);
  metis_file << hypergraph_->GetNumVertices() << " "
             << hypergraph_->GetNumHyperedges() << std::endl;

  for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
    for (int j = 0; j < hypergraph_->GetNeighbors(i).size(); j++) {
      metis_file << hypergraph_->GetNeighbors(i)[j] + 1 << " ";
    }
    metis_file << std::endl;
  }

  metis_file.close();

  // run gpmetis to run generate the partition
  std::string command = "./gpmetis " + graph_file + " " +
                        std::to_string(num_parts) + " > /dev/null 2>&1";
  int status = system(command.c_str());

  if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
    std::cerr << "Error: gpmetis failed with status " << WEXITSTATUS(status)
              << std::endl;
    return std::vector<int>();
  } else {
    // read the partition file
    std::string partition_file =
        "metis_graph.txt.part." + std::to_string(num_parts);
    std::ifstream partition_input(partition_file);
    if (!partition_input.is_open()) {
      std::cerr << "Error: Cannot open partition file " << partition_file
                << std::endl;
      return std::vector<int>();
    }
    std::vector<int> partition(hypergraph_->GetNumVertices(), 0);
    for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
      partition_input >> partition[i];
    }
    return partition;
  }
}

void ChipletPart::ReadChipletGraph(std::string hypergraph_file,
                                   std::string chiplet_io_file) {
  // read the chiplet_io_file_
  if (!chiplet_io_file.empty()) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(chiplet_io_file.c_str());
    if (!result) {
      std::cerr << "Error: " << result.description() << std::endl;
      return;
    }
    pugi::xml_node ios = doc.child("ios");
    for (pugi::xml_node io = ios.child("io"); io; io = io.next_sibling("io")) {
      // Read the 'type' attribute
      std::string type = io.attribute("type").as_string();

      // Read the 'reach' attribute
      double reach = io.attribute("reach").as_double();

      // Add to hash map
      io_map_[type] = reach;
    }
  }

  // read hypergraph file
  std::ifstream hypergraph_file_input(hypergraph_file);
  if (!hypergraph_file_input.is_open()) {
    std::cerr << "Error: Cannot open hypergraph file " << hypergraph_file
              << std::endl;
    return;
  }
  // Check the number of vertices, number of hyperedges, weight flag
  std::string cur_line;
  std::getline(hypergraph_file_input, cur_line);
  std::istringstream cur_line_buf(cur_line);
  std::vector<int> stats{std::istream_iterator<int>(cur_line_buf),
                         std::istream_iterator<int>()};
  num_hyperedges_ = stats[0];
  num_vertices_ = stats[1];
  bool hyperedge_weight_flag = false;
  bool vertex_weight_flag = false;
  if (stats.size() == 3) {
    if ((stats[2] % 10) == 1) {
      hyperedge_weight_flag = true;
    }
    if (stats[2] >= 10) {
      vertex_weight_flag = true;
    }
  }

  // clear the related vectors
  hyperedges_.clear();
  hyperedge_weights_.clear();
  vertex_weights_.clear();
  hyperedges_.reserve(num_hyperedges_);
  hyperedge_weights_.reserve(num_hyperedges_);
  vertex_weights_.reserve(num_vertices_);
  std::unordered_map<long long, std::vector<int>> distinct_hyperedges;

  // Read hyperedge information
  for (int i = 0; i < num_hyperedges_; i++) {
    std::getline(hypergraph_file_input, cur_line);
    if (hyperedge_weight_flag == true) {
      std::istringstream cur_line_buf(cur_line);
      std::vector<float> hvec{std::istream_iterator<float>(cur_line_buf),
                              std::istream_iterator<float>()};
      // each line is wt reach {vertex list}
      // the first element is the weight, the second element is the reach
      // the remaining elements are the vertex list
      std::vector<float> hwt = {hvec[0]};
      float reach = hvec[1];
      float io_size = hvec[2];
      io_sizes_.push_back(io_size);
      reach_.push_back(reach);
      std::vector<int> hyperedge(hvec.begin() + 3, hvec.end());
      long long hash = 0;
      for (auto &value : hyperedge) {
        value--; // the vertex id starts from 1 in the hypergraph file
        hash += value * value;
      }

      /*if (distinct_hyperedges.find(hash) != distinct_hyperedges.end()) {
        continue;
      } else {
        distinct_hyperedges[hash] = hyperedge;
      }*/

      hyperedge_weights_.push_back(hwt);
      hyperedges_.push_back(hyperedge);
    } else {
      std::istringstream cur_line_buf(cur_line);
      std::vector<int> hyperedge{std::istream_iterator<int>(cur_line_buf),
                                 std::istream_iterator<int>()};
      for (auto &value : hyperedge) {
        value--; // the vertex id starts from 1 in the hypergraph file
      }
      std::vector<float> hwts(hyperedge_dimensions_,
                              1.0); // each dimension has the same weight
      hyperedge_weights_.push_back(hwts);
      hyperedges_.push_back(hyperedge);
    }
  }

  // Read weight for vertices
  for (int i = 0; i < num_vertices_; i++) {
    if (vertex_weight_flag == true) {
      std::getline(hypergraph_file_input, cur_line);
      std::istringstream cur_line_buf(cur_line);
      std::vector<float> vwts{std::istream_iterator<float>(cur_line_buf),
                              std::istream_iterator<float>()};
      vertex_weights_.push_back(vwts);
    } else {
      std::vector<float> vwts(vertex_dimensions_, 1.0);
      vertex_weights_.push_back(vwts);
    }
  }

  hypergraph_ = std::make_shared<Hypergraph>(
      vertex_dimensions_, hyperedge_dimensions_, hyperedges_, vertex_weights_,
      hyperedge_weights_, reach_, io_sizes_);
  std::cout << "[INFO] Number of IP blocks in chiplet graph: " << num_vertices_
            << std::endl;
  std::cout << "[INFO] Number of nets in chiplet graph: " << hyperedges_.size()
            << std::endl;
}

void ChipletPart::TechAssignPartition(
    std::string hypergraph_file, std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::vector<std::string> techs) {
  // seed the rng_ with 0
  rng_.seed(seed_);
  auto start_time = std::chrono::high_resolution_clock::now();
  std::cout << "[INFO] Reading the chiplet graph " << hypergraph_file
            << std::endl;
  std::cout << "[INFO] Reading the chiplet IO file " << chiplet_io_file
            << std::endl;
  std::cout << "[INFO] Reading the chiplet layer file " << chiplet_layer_file
            << std::endl;
  std::cout << "[INFO] Reading the chiplet wafer process file "
            << chiplet_wafer_process_file << std::endl;
  std::cout << "[INFO] Reading the chiplet assembly process file "
            << chiplet_assembly_process_file << std::endl;
  std::cout << "[INFO] Reading the chiplet test file " << chiplet_test_file
            << std::endl;
  std::cout << "[INFO] Reading the chiplet netlist file "
            << chiplet_netlist_file << std::endl;
  std::cout << "[INFO] Reading the chiplet blocks file " << chiplet_blocks_file
            << std::endl;
  std::cout << "[INFO] Reach: " << reach << std::endl;
  std::cout << "[INFO] Separation: " << separation << std::endl;
  std::cout << "[INFO] Techs: ";
  for (auto &tech : techs) {
    std::cout << tech << " ";
  }
  std::cout << std::endl;

  // for each tech node run the partitioner first
  // 45nm, 10nm, 7nm
  std::vector<std::string> optimal_tech_arr;
  std::vector<int> optimal_parts_for_each_tech;
  const std::string separator(60, '-'); // Creates a separator line
  /*for (auto &tech : techs) {
    std::cout << separator << std::endl;
    std::cout << std::setw((separator.size() + 10) / 2) << "TECH: " << tech
              << std::endl;
    std::cout << separator << std::endl;
    Partition(hypergraph_file, chiplet_io_file, chiplet_layer_file,
              chiplet_wafer_process_file, chiplet_assembly_process_file,
              chiplet_test_file, chiplet_netlist_file, chiplet_blocks_file,
              reach, separation, tech);
    optimal_parts_for_each_tech.push_back(tech_parts_);
  }

  // replicate techs based on the number of partitions to estimate number of
  // partitions
  std::vector<std::string> techs_replicated;
  for (int i = 0; i < optimal_parts_for_each_tech.size(); i++) {
    for (int j = 0; j < optimal_parts_for_each_tech[i]; j++) {
      techs_replicated.push_back(techs[i]);
    }
  }

  int estimated_max_parts = techs_replicated.size();*/
  // std::vector<std::string> techs_replicated = techs;
  int estimated_max_parts = techs.size();

  // the heavy lifting begins here
  std::cout << separator << std::endl;
  std::cout << std::setw((separator.size() + 10) / 2)
            << "TECH ASSIGNMENT BEGINS: " << std::endl;
  std::cout << separator << std::endl;
  GeneticPart(hypergraph_file, chiplet_io_file, chiplet_layer_file,
              chiplet_wafer_process_file, chiplet_assembly_process_file,
              chiplet_test_file, chiplet_netlist_file, chiplet_blocks_file,
              reach, separation, techs);

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_time = end_time - start_time;
  std::cout << "[INFO] Total time taken for tech-aware ChipletPart: "
            << elapsed_time.count() << "s" << std::endl;
}

void ChipletPart::CreateMatingPool(
    const std::vector<std::vector<std::string>> &population,
    const std::vector<float> &fitness,
    std::vector<std::vector<std::string>> &mating_pool) {

  mating_pool.clear(); // Clear existing entries in mating pool
  std::vector<int> indices(population.size());
  for (size_t i = 0; i < indices.size(); ++i) {
    indices[i] = i;
  }

  // Sorting indices based on fitness values (ascending order)
  std::sort(indices.begin(), indices.end(), [&](const int &a, const int &b) {
    return fitness[a] < fitness[b];
  });

  // Select the best individuals to fill the mating pool
  mating_pool.reserve(population_size_);
  int factor =
      std::max(1, population_size_ / static_cast<int>(population.size()));

  for (int k = 0; k < factor; ++k) {
    for (int i : indices) {
      if (mating_pool.size() < population_size_) {
        mating_pool.push_back(population[i]);
      }
    }
  }
}

void ChipletPart::GeneticPart(
    std::string hypergraph_file, std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::vector<std::string> &tech_nodes) {

  const std::string separator(60, '=');
  std::cout << separator << std::endl
            << std::setw((separator.size() + 10) / 2)
            << "RUNNING GENETIC ALGORITHM!" << std::endl
            << separator << std::endl;

  // Genetic algorithm core functions
  auto CrossOver = [this](const std::vector<std::string> &parent1,
                          const std::vector<std::string> &parent2) {
    std::vector<std::string> child(parent1.size());
    int c = std::uniform_int_distribution<int>(0, parent1.size() - 1)(rng_);
    std::copy(parent1.begin(), parent1.begin() + c, child.begin());
    std::copy(parent2.begin() + c, parent2.end(), child.begin() + c);
    return child;
  };

  auto Mutate = [this](std::vector<std::string> &individual,
                       const std::vector<std::string> &tech_nodes) {
    int part_idx = std::uniform_int_distribution<int>(0, num_parts_ - 1)(rng_);
    int tech_idx =
        std::uniform_int_distribution<int>(0, tech_nodes.size() - 1)(rng_);
    individual[part_idx] = tech_nodes[tech_idx];
  };

  // Initialization
  std::vector<int> alpha_partition, best_partition;
  std::vector<std::string> alpha_assignment, best_individual;
  float alpha_cost = std::numeric_limits<float>::max(), best_cost = alpha_cost;
  std::vector<std::vector<std::string>> population;
  std::vector<float> fitness(num_individuals_, alpha_cost);

  // Seed the random generator
  rng_.seed(std::random_device{}());

  // Generate initial population
  population.reserve(num_individuals_);
  for (int i = 0; i < num_individuals_; ++i) {
    std::vector<std::string> individual(num_parts_);
    for (auto &part : individual) {
      part = tech_nodes[rand() % tech_nodes.size()];
    }
    population.push_back(std::move(individual));
  }

  // Main loop: Genetic Algorithm
  for (int generation = 0; generation < num_generations_; ++generation) {
    std::cout << separator << "\n[INFO] Starting [generation " << generation
              << "] with population " << population.size() << std::endl
              << separator << std::endl;

    // 1. Evaluate fitness (Parallelized)
    std::vector<std::future<std::tuple<float, std::vector<int>>>> futures;
    futures.reserve(population.size());

    for (int j = 0; j < population.size(); ++j) {
      futures.push_back(std::async(std::launch::async, [&, j]() {
        return QuickPart(hypergraph_file, chiplet_io_file, chiplet_layer_file,
                         chiplet_wafer_process_file,
                         chiplet_assembly_process_file, chiplet_test_file,
                         chiplet_netlist_file, chiplet_blocks_file, reach,
                         separation, population[j]);
      }));
    }

    for (int j = 0; j < population.size(); ++j) {
      auto [cost, partition] = futures[j].get();
      fitness[j] = cost;

      if (cost < best_cost) {
        best_cost = cost;
        best_individual = population[j];
        best_partition = std::move(partition);
        if (best_cost < alpha_cost) {
          alpha_cost = best_cost;
          alpha_assignment = best_individual;
          alpha_partition = best_partition;
        }
      }
    }

    // 2. Selection
    std::vector<std::vector<std::string>> mating_pool;
    CreateMatingPool(population, fitness, mating_pool);

    // 3. Crossover
    std::vector<std::vector<std::string>> new_population;
    new_population.reserve(population_size_);
    for (int j = 0; j < mating_pool.size() / 2; ++j) {
      auto child1 = CrossOver(mating_pool[2 * j], mating_pool[2 * j + 1]);
      auto child2 = CrossOver(mating_pool[2 * j + 1], mating_pool[2 * j]);
      new_population.push_back(std::move(child1));
      new_population.push_back(std::move(child2));
    }

    // 4. Mutation
    for (auto &individual : new_population) {
      if (std::uniform_real_distribution<float>(0.0, 1.0)(rng_) <
          mutation_rate_) {
        Mutate(individual, tech_nodes);
      }
    }

    // 5. Replace the old population
    population = std::move(new_population);

    // 6. Early stopping
    if (generation > gen_threshold_ &&
        std::abs(best_cost - alpha_cost) < 1e-6) {
      break;
    }

    std::cout << "[INFO] Best generational cost: " << best_cost << std::endl;
  }

  // Output results
  int num_parts =
      *std::max_element(best_partition.begin(), best_partition.end());
  std::ofstream partition_output("tech_assignment.chipletpart.parts." +
                                 std::to_string(num_parts));
  for (const auto &part : best_partition) {
    partition_output << part << std::endl;
  }
  partition_output.close();

  std::ofstream tech_assignment_file("tech_assignment.chipletpart.techs." +
                                     std::to_string(num_parts_));
  for (const auto &tech : best_individual) {
    tech_assignment_file << tech << std::endl;
  }
  tech_assignment_file.close();

  std::cout << "[INFO] Best cost: " << alpha_cost << " for "
            << (std::max_element(alpha_partition.begin(),
                                 alpha_partition.end()) -
                alpha_partition.begin()) +
                   1
            << " partitions\n[INFO] Best partition and tech assignment written "
               "to files.\n";
}

std::tuple<float, std::vector<int>> ChipletPart::InitQuickPart(
    std::string hypergraph_file, std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::vector<std::string> &tech_nodes) {
  Matrix<float> upper_block_balance;
  Matrix<float> lower_block_balance;
  std::vector<float> zero_vector(vertex_dimensions_, 0.0);
  // removing balance constraints
  for (int i = 0; i < num_parts_; i++) {
    upper_block_balance.emplace_back(hypergraph_->GetTotalVertexWeights());
    lower_block_balance.emplace_back(zero_vector);
  }

  num_parts_ = tech_nodes.size();
  // Do num_parts-way cuts
  std::vector<int> kway_partition = METISPart(num_parts_);
  // remove all part.* files
  std::string command = "rm -f .part.*";
  int status = system(command.c_str());

  std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);
  auto chiplet_refiner = std::make_shared<ChipletRefiner>(
      num_parts_, refine_iters_, max_moves_, reaches);
  chiplet_refiner->SetReach(reach);
  chiplet_refiner->SetSeparation(separation);
  // Initialize the cost model here
  chiplet_refiner->InitCostModel(
      chiplet_io_file, chiplet_layer_file, chiplet_wafer_process_file,
      chiplet_assembly_process_file, chiplet_test_file, chiplet_netlist_file,
      chiplet_blocks_file);
  chiplet_refiner->SetTechArray(tech_nodes);
  chiplet_refiner->SetNumParts(num_parts_);
  auto f_tuple = chiplet_refiner->RunFloorplanner(kway_partition, hypergraph_,
                                                  2000, 500, 1.0);
  std::vector<float> aspect_ratios = std::get<0>(f_tuple);
  std::vector<float> x_locations = std::get<1>(f_tuple);
  std::vector<float> y_locations = std::get<2>(f_tuple);
  chiplet_refiner->SetAspectRatios(aspect_ratios);
  chiplet_refiner->SetXLocations(x_locations);
  chiplet_refiner->SetYLocations(y_locations);
  chiplet_refiner->InitSlopes(num_parts_);
  float cost = chiplet_refiner->GetCostFromScratch(kway_partition);
  chiplet_refiner->SetLegacyCost(cost);
  chiplet_refiner->SetRefineIters(1);
  chiplet_refiner->SetMove(10);
  chiplet_refiner->Refine(hypergraph_, upper_block_balance, lower_block_balance,
                          kway_partition);
  int max_part =
      *std::max_element(kway_partition.begin(), kway_partition.end());
  cost = chiplet_refiner->GetCostFromScratch(kway_partition);
  std::cout << "[GENETIC-RUNS] [Parts " << max_part + 1 << "] [Chiplet cost "
            << cost << "] ";
  std::cout << "[FP feasible ";
  f_tuple = chiplet_refiner->RunFloorplanner(kway_partition, hypergraph_, 2000,
                                             500, 1.0);
  if (std::get<3>(f_tuple)) {
    std::cout << "YES]" << std::endl;
  } else {
    std::cout << "NO]" << std::endl;
  }

  return std::make_tuple(cost, kway_partition);
}

std::tuple<float, std::vector<int>> ChipletPart::QuickPart(
    std::string hypergraph_file, std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::vector<std::string> &tech_nodes) {
  Matrix<float> upper_block_balance;
  Matrix<float> lower_block_balance;
  std::vector<float> zero_vector(vertex_dimensions_, 0.0);
  // removing balance constraints
  for (int i = 0; i < num_parts_; i++) {
    upper_block_balance.emplace_back(hypergraph_->GetTotalVertexWeights());
    lower_block_balance.emplace_back(zero_vector);
  }

  std::vector<std::vector<int>> init_partitions;
  num_parts_ = tech_nodes.size();

  // Step 1: Run spectral partition
  /*std::vector<int> init_spec_partition = SpectralPartition(hypergraph_file);
  // check if init_spec_partition is empty or if any entry is -1
  bool fail_flag = false;
  if (init_spec_partition.empty() ||
      std::find(init_spec_partition.begin(), init_spec_partition.end(), -1) !=
          init_spec_partition.end()) {
    std::cerr << "[INFO] Error: Spectral partition failed! -- Ignoring this!"
              << std::endl;
    fail_flag = true;
  }

  if (fail_flag == false) {
    std::cout << "[INIT-PART] Spectral partition obtained!" << std::endl;
    init_partitions.push_back(init_spec_partition);
  }

  // Step 2: Run CrossBarExpansion
  float quantile = 0.99;
  std::vector<int> crossbars = FindCrossbars(quantile);
  std::cout << "[INFO] High-degree nodes found are: " << crossbars.size()
            << std::endl;
  for (int i = 1; i < tech_nodes.size(); i++) {
    if (i == 1) {
      std::vector<int> partition(hypergraph_->GetNumVertices(), 0);
      init_partitions.push_back(partition);
    } else {
      int num_parts = tech_nodes.size();
      std::vector<int> crossbar_partition =
          CrossBarExpansion(crossbars, num_parts);
      if (crossbar_partition.empty()) {
        continue;
      }
      init_partitions.push_back(crossbar_partition);
    }
  }*/

  // Step 3: Run KWayCuts
  for (int i = 1; i <= tech_nodes.size(); i++) {
    if (i == 1) {
      std::vector<int> partition(hypergraph_->GetNumVertices(), 0);
      init_partitions.push_back(partition);
      continue;
    }
    // std::vector<int> kway_partition = KWayCuts(i);
    // init_partitions.push_back(kway_partition);
    std::vector<int> metis_partition = METISPart(i);
    init_partitions.push_back(metis_partition);
  }
  // remove all part.* files
  std::string command = "rm -f .part.*";
  int status = system(command.c_str());
  int refine_iters = 1;
  int max_moves = 20;
  std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);
  auto chiplet_refiner = std::make_shared<ChipletRefiner>(
      tech_nodes.size(), refine_iters, max_moves, reaches);
  chiplet_refiner->SetReach(reach);
  chiplet_refiner->SetSeparation(separation);
  // Initialize the cost model here
  chiplet_refiner->InitCostModel(
      chiplet_io_file, chiplet_layer_file, chiplet_wafer_process_file,
      chiplet_assembly_process_file, chiplet_test_file, chiplet_netlist_file,
      chiplet_blocks_file);
  size_t i = 0;
  std::vector<int> best_partition;
  float best_cost = std::numeric_limits<float>::max();

  for (size_t idx = 0; idx < init_partitions.size(); ++idx) {
    auto &partition = init_partitions[idx];
    // for (auto &partition : init_partitions) {
    int max_part = *std::max_element(partition.begin(), partition.end());
    if (max_part + 1 > tech_nodes.size()) {
      continue;
    }
    std::vector<std::string> tech_array(max_part + 1);
    // get the tech nodes for the partition
    for (size_t i = 0; i < partition.size(); i++) {
      tech_array[partition[i]] = tech_nodes[partition[i]];
    }
    chiplet_refiner->SetTechArray(tech_array);
    chiplet_refiner->SetNumParts(max_part + 1);
    if (max_part == 0) {
      std::vector<float> aspect_ratios = {1.0};
      std::vector<float> x_locations = {0.0};
      std::vector<float> y_locations = {0.0};
      chiplet_refiner->SetAspectRatios(aspect_ratios);
      chiplet_refiner->SetXLocations(x_locations);
      chiplet_refiner->SetYLocations(y_locations);
      chiplet_refiner->InitSlopes(max_part + 1);
      float cost = chiplet_refiner->GetCostFromScratch(partition);
      std::cout << "[COST-FPL-INFO] [Partition " << i++ << "] [Parts "
                << max_part + 1 << "] [Chiplet cost " << cost << "] ";
      std::cout << "[FP feasible YES]" << std::endl;
      exit(EXIT_SUCCESS);
      best_cost = cost;
      best_partition = partition;
    } else {
      auto f_tuple = chiplet_refiner->RunFloorplanner(partition, hypergraph_,
                                                      2000, 500, 1.0);
      std::vector<float> aspect_ratios = std::get<0>(f_tuple);
      std::vector<float> x_locations = std::get<1>(f_tuple);
      std::vector<float> y_locations = std::get<2>(f_tuple);
      chiplet_refiner->SetAspectRatios(aspect_ratios);
      chiplet_refiner->SetXLocations(x_locations);
      chiplet_refiner->SetYLocations(y_locations);
      chiplet_refiner->InitSlopes(max_part + 1);
      float cost = chiplet_refiner->GetCostFromScratch(partition);
      chiplet_refiner->SetLegacyCost(cost);
      // exit(EXIT_SUCCESS);
      // do the floorplan-aware refinement here
      chiplet_refiner->Refine(hypergraph_, upper_block_balance,
                              lower_block_balance, partition);
      max_part = *std::max_element(partition.begin(), partition.end());
      cost = chiplet_refiner->GetCostFromScratch(partition);
      std::cout << "[COST-FPL-INFO] [Partition " << i++ << "] [Parts "
                << max_part + 1 << "] [Chiplet cost " << cost << "] ";
      // exit(EXIT_SUCCESS);
      std::cout << "[FP feasible ";
      f_tuple = chiplet_refiner->RunFloorplanner(partition, hypergraph_, 2000,
                                                 500, 1.0);
      if (std::get<3>(f_tuple)) {
        std::cout << "YES]" << std::endl;
        if (cost < best_cost) {
          best_cost = cost;
          best_partition = partition;
        }
      } else {
        std::cout << "NO]" << std::endl;
      }
    }
  }

  std::cout << "[INFO] Best cost " << best_cost << " for "
            << *std::max_element(best_partition.begin(), best_partition.end()) +
                   1
            << " partitions" << std::endl;

  return std::make_tuple(best_cost, best_partition);
}

void ChipletPart::Partition(
    std::string hypergraph_file, std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::string tech) {

  rng_.seed(seed_);

  // Log function (optional: consider reducing verbosity)
  auto log_info = [](const std::string &msg) {
    std::cout << "[INFO] " << msg << std::endl;
  };

  log_info("Reading the chiplet graph " + hypergraph_file);
  log_info("Reading the chiplet IO file " + chiplet_io_file);
  log_info("Reading the chiplet layer file " + chiplet_layer_file);
  log_info("Reading the chiplet wafer process file " +
           chiplet_wafer_process_file);
  log_info("Reading the chiplet assembly process file " +
           chiplet_assembly_process_file);
  log_info("Reading the chiplet test file " + chiplet_test_file);
  log_info("Reading the chiplet netlist file " + chiplet_netlist_file);
  log_info("Reading the chiplet blocks file " + chiplet_blocks_file);
  log_info("Reach: " + std::to_string(reach));
  log_info("Separation: " + std::to_string(separation));
  log_info("Tech: " + tech);

  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();

  // Pre-allocate vectors to avoid resizing
  std::vector<std::vector<int>> init_partitions;
  init_partitions.reserve(10); // assuming we need to store around 10 partitions

  // Prepare upper and lower block balance
  std::vector<float> total_weights = hypergraph_->GetTotalVertexWeights();
  Matrix<float> upper_block_balance(num_parts_, total_weights);
  Matrix<float> lower_block_balance(
      num_parts_, std::vector<float>(vertex_dimensions_, 0.0));

  // Step 1: Run spectral partition
  std::vector<int> init_spec_partition = SpectralPartition(hypergraph_file);
  if (!init_spec_partition.empty() &&
      std::find(init_spec_partition.begin(), init_spec_partition.end(), -1) ==
          init_spec_partition.end()) {
    log_info("Spectral partition obtained!");
    init_partitions.push_back(
        std::move(init_spec_partition)); // Use move to avoid copying
  } else {
    std::cerr << "[ERROR] Spectral partition failed! Ignoring this!"
              << std::endl;
  }

  // Step 2: Run CrossBarExpansion
  float quantile = 0.99;
  std::vector<int> crossbars = FindCrossbars(quantile);
  log_info("High-degree nodes found: " + std::to_string(crossbars.size()));

  for (int i : chiplets_set_) {
    if (i == 1) {
      init_partitions.push_back(std::vector<int>(hypergraph_->GetNumVertices(),
                                                 0)); // create empty partition
    } else {
      std::vector<int> crossbar_partition =
          CrossBarExpansion(crossbars, num_parts_);
      if (!crossbar_partition.empty()) {
        init_partitions.push_back(std::move(crossbar_partition));
      }
    }
  }

  // Step 3: Run KWayCuts
  for (int i : chiplets_set_) {
    if (i != 1) {
      init_partitions.push_back(KWayCuts(i));
      init_partitions.push_back(METISPart(i));
    }
  }

  // Remove .part.* files safely
  if (std::filesystem::exists(".part.*")) {
    std::filesystem::remove_all(
        ".part.*"); // C++17 feature, faster than system call
  }

  log_info("Random and METIS partitions obtained!");
  log_info("Total number of initial partitions: " +
           std::to_string(init_partitions.size()));

  // Step 4: Run FMRefiner
  auto chiplet_refiner = std::make_shared<ChipletRefiner>(
      num_parts_, refine_iters_, max_moves_,
      std::vector<int>(hypergraph_->GetNumHyperedges(), reach));
  chiplet_refiner->SetReach(reach);
  chiplet_refiner->SetSeparation(separation);
  chiplet_refiner->InitCostModel(
      chiplet_io_file, chiplet_layer_file, chiplet_wafer_process_file,
      chiplet_assembly_process_file, chiplet_test_file, chiplet_netlist_file,
      chiplet_blocks_file);

  // Refactoring loops and minimizing redundant code
  size_t best_partition_idx = 0;
  float best_cost = std::numeric_limits<float>::max();

  for (size_t idx = 0; idx < init_partitions.size(); ++idx) {
    std::vector<int> &partition = init_partitions[idx];
    int max_part = *std::max_element(partition.begin(), partition.end());
    std::vector<std::string> tech_array(max_part + 1, tech);

    chiplet_refiner->SetTechArray(tech_array);
    chiplet_refiner->SetNumParts(max_part + 1);

    if (max_part == 0) {
      std::vector<float> aspect_ratios = {1.0};
      std::vector<float> x_locations = {0.0};
      std::vector<float> y_locations = {0.0};
      chiplet_refiner->SetAspectRatios(aspect_ratios);
      chiplet_refiner->SetXLocations(x_locations);
      chiplet_refiner->SetYLocations(y_locations);
      chiplet_refiner->InitSlopes(max_part + 1);
      float cost = chiplet_refiner->GetCostFromScratch(partition);
      std::cout << "[COST-FPL-INFO] [Partition " << idx << "] [Parts "
                << max_part + 1 << "] [Chiplet cost " << cost << "] ";
      std::cout << "[FP feasible YES]" << std::endl;
      // exit(EXIT_SUCCESS);
      best_cost = cost;
      best_partition_idx = idx;
    } else {
      auto f_tuple = chiplet_refiner->RunFloorplanner(partition, hypergraph_,
                                                      2000, 500, 1.0);
      std::vector<float> aspect_ratios = std::get<0>(f_tuple);
      std::vector<float> x_locations = std::get<1>(f_tuple);
      std::vector<float> y_locations = std::get<2>(f_tuple);
      chiplet_refiner->SetAspectRatios(aspect_ratios);
      chiplet_refiner->SetXLocations(x_locations);
      chiplet_refiner->SetYLocations(y_locations);
      chiplet_refiner->InitSlopes(max_part + 1);
      float cost = chiplet_refiner->GetCostFromScratch(partition);
      chiplet_refiner->SetLegacyCost(cost);
      // exit(EXIT_SUCCESS);
      // do the floorplan-aware refinement here
      chiplet_refiner->Refine(hypergraph_, upper_block_balance,
                              lower_block_balance, partition);
      max_part = *std::max_element(partition.begin(), partition.end());
      cost = chiplet_refiner->GetCostFromScratch(partition, true);
      std::cout << "[COST-FPL-INFO] [Partition " << idx << "] [Parts "
                << max_part + 1 << "] [Chiplet cost " << cost << "] ";
      std::cout << "[FP feasible ";
      f_tuple = chiplet_refiner->RunFloorplanner(partition, hypergraph_, 2000,
                                                 500, 1.0);
      if (std::get<3>(f_tuple)) {
        std::cout << "YES]" << std::endl;
        if (cost < best_cost) {
          best_cost = cost;
          best_partition_idx = idx;
        }
      } else {
        std::cout << "NO]" << std::endl;
      }
    }
  }

  log_info("Best partition cost: " + std::to_string(best_cost));

  // Write best partition to file
  std::ofstream best_partition_file(hypergraph_file + ".chiplet.part");
  for (const auto &part : init_partitions[best_partition_idx]) {
    best_partition_file << part << std::endl;
  }

  tech_parts_ = *std::max_element(init_partitions[best_partition_idx].begin(),
                                  init_partitions[best_partition_idx].end()) +
                1;

  auto end_time_stamp_global = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_time_global =
      end_time_stamp_global - start_time_stamp_global;

  log_info("Total time taken: " + std::to_string(elapsed_time_global.count()) +
           " seconds");
  log_info("Partitioning completed!");

  float total_fplan_time = chiplet_refiner->GetTotalFloorplanTime();
  log_info("Total floorplanning time: " + std::to_string(total_fplan_time) +
           " seconds");

  float total_cost_model_time = chiplet_refiner->GetCostModelTime();
  log_info("Total cost model time: " + std::to_string(total_cost_model_time) +
           " seconds");
}

void ChipletPart::EvaluatePartition(
    std::string hypergraph_file, std::string hypergraph_part,
    std::string chiplet_io_file, std::string chiplet_layer_file,
    std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::string tech) {
  Py_Initialize();
  PyObject *sysPath = PySys_GetObject("path");
  PyList_Append(sysPath,
                PyUnicode_FromString(path_to_embedding_script_.c_str()));

  std::cout << "[INFO] Reading the chiplet graph " << hypergraph_file
            << std::endl;
  std::cout << "[INFO] Reading the partition file " << hypergraph_part
            << std::endl;
  std::cout << "[INFO] Reading the chiplet IO file " << chiplet_io_file
            << std::endl;
  std::cout << "[INFO] Reading the chiplet layer file " << chiplet_layer_file
            << std::endl;
  std::cout << "[INFO] Reading the chiplet wafer process file "
            << chiplet_wafer_process_file << std::endl;
  std::cout << "[INFO] Reading the chiplet assembly process file "
            << chiplet_assembly_process_file << std::endl;
  std::cout << "[INFO] Reading the chiplet test file " << chiplet_test_file
            << std::endl;
  std::cout << "[INFO] Reading the chiplet netlist file "
            << chiplet_netlist_file << std::endl;
  std::cout << "[INFO] Reading the chiplet blocks file " << chiplet_blocks_file
            << std::endl;
  std::cout << "[INFO] Reach: " << reach << std::endl;
  std::cout << "[INFO] Separation: " << separation << std::endl;
  std::cout << "[INFO] Tech: " << tech << std::endl;

  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();
  Matrix<float> upper_block_balance;
  Matrix<float> lower_block_balance;
  std::vector<float> zero_vector(vertex_dimensions_, 0.0);
  // removing balance constraints
  for (int i = 0; i < num_parts_; i++) {
    upper_block_balance.emplace_back(hypergraph_->GetTotalVertexWeights());
    lower_block_balance.emplace_back(zero_vector);
  }

  std::vector<int> partition;

  // read hypergraph partitioning file
  std::ifstream hypergraph_part_input(hypergraph_part);
  int num_parts = 0;
  if (!hypergraph_part_input.is_open()) {
    std::cerr << "Error: Cannot open hypergraph partition file "
              << hypergraph_part << std::endl;
    return;
  } else {
    partition = std::vector<int>(hypergraph_->GetNumVertices(), 0);
    for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
      hypergraph_part_input >> partition[i];
      num_parts = std::max(num_parts, partition[i]);
    }
  }

  num_parts_ = num_parts + 1;
  std::cout << "[INFO] Number of partitions: " << num_parts_ << std::endl;
  std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);
  auto chiplet_refiner = std::make_shared<ChipletRefiner>(
      num_parts_, refine_iters_, max_moves_, reaches);
  chiplet_refiner->SetReach(reach);
  chiplet_refiner->SetSeparation(separation);
  // Initialize the cost model here
  chiplet_refiner->InitCostModel(
      chiplet_io_file, chiplet_layer_file, chiplet_wafer_process_file,
      chiplet_assembly_process_file, chiplet_test_file, chiplet_netlist_file,
      chiplet_blocks_file);
  std::vector<std::string> tech_array(num_parts_, tech);
  chiplet_refiner->SetTechArray(tech_array);
  chiplet_refiner->SetNumParts(num_parts_);

  auto f_tuple =
      chiplet_refiner->RunFloorplanner(partition, hypergraph_, 2000, 500, 1.0);
  std::vector<float> aspect_ratios = std::get<0>(f_tuple);
  std::vector<float> x_locations = std::get<1>(f_tuple);
  std::vector<float> y_locations = std::get<2>(f_tuple);
  chiplet_refiner->SetAspectRatios(aspect_ratios);
  chiplet_refiner->SetXLocations(x_locations);
  chiplet_refiner->SetYLocations(y_locations);
  // print the aspect ratios
  std::cout << "[INFO] Aspect ratios: ";
  for (auto &ar : aspect_ratios) {
    std::cout << ar << " ";
  }
  std::cout << std::endl;
  chiplet_refiner->InitSlopes(num_parts_);
  std::cout << "Floorplan is feasible: " << std::get<3>(f_tuple) << std::endl;
  float cost = chiplet_refiner->GetCostFromScratch(partition);
  std::cout << "[COST-FPL-INFO] [Partition has cost " << cost
            << "] [FP feasible " << std::get<3>(f_tuple) << "]" << std::endl;
}

std::vector<int> ChipletPart::FindCrossbars(float &quantile) {
  std::vector<int> degrees(hypergraph_->GetNumVertices(), 0);
  for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
    degrees[i] = hypergraph_->GetNeighbors(i).size();
  }

  std::vector<int> sorted_degrees = degrees;
  std::sort(sorted_degrees.begin(), sorted_degrees.end());

  int threshold_index = static_cast<int>(quantile * sorted_degrees.size());
  int degree_threshold = sorted_degrees[threshold_index];

  std::vector<int> crossbars;
  for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
    if (degrees[i] >= degree_threshold) {
      crossbars.push_back(i);
    }
  }

  return crossbars;
}

} // namespace chiplet
