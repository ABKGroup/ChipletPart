///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "GeneticTechPartitioner.h"
#include "Console.h" // For console output utilities

namespace chiplet {

// Constructor
GeneticTechPartitioner::GeneticTechPartitioner(
    std::shared_ptr<Hypergraph> hypergraph,
    const std::vector<std::string> &available_tech_nodes, float ub_factor = 1.0,
    unsigned int seed, int num_generations, int population_size,
    float mutation_rate, float crossover_rate, int min_partitions,
    int max_partitions)
    : hypergraph_(hypergraph), available_tech_nodes_(available_tech_nodes),
      ub_factor_(ub_factor), rng_(seed), num_generations_(num_generations),
      population_size_(population_size), mutation_rate_(mutation_rate),
      crossover_rate_(crossover_rate), min_partitions_(min_partitions),
      max_partitions_(max_partitions), chiplet_part_(nullptr) {

  // Ensure min_partitions_ is at least 1
  min_partitions_ = std::max(1, min_partitions_);

  // Ensure max_partitions_ is no more than the number of vertices
  max_partitions_ = std::min(max_partitions_,
                             static_cast<int>(hypergraph_->GetNumVertices()));

  // Ensure max_partitions_ is at least min_partitions_
  max_partitions_ = std::max(min_partitions_, max_partitions_);

  // Initialize best solution with the worst possible cost
  best_solution_ = GeneticSolution();

  Console::Header("Genetic Tech Partitioner Initialized");
  std::vector<std::string> columns = {"Parameter", "Value"};
  std::vector<int> widths = {30, 20};
  Console::TableHeader(columns, widths);

  Console::TableRow(
      {"Hypergraph Vertices", std::to_string(hypergraph_->GetNumVertices())},
      widths);
  Console::TableRow({"Hypergraph Hyperedges",
                     std::to_string(hypergraph_->GetNumHyperedges())},
                    widths);
  Console::TableRow(
      {"Available Technologies", std::to_string(available_tech_nodes_.size())},
      widths);
  Console::TableRow({"Random Seed", std::to_string(seed)}, widths);
  Console::TableRow({"Generations", std::to_string(num_generations_)}, widths);
  Console::TableRow({"Population Size", std::to_string(population_size_)},
                    widths);
  Console::TableRow({"Mutation Rate", std::to_string(mutation_rate_)}, widths);
  Console::TableRow({"Crossover Rate", std::to_string(crossover_rate_)},
                    widths);
  Console::TableRow({"Min Partitions", std::to_string(min_partitions_)},
                    widths);
  Console::TableRow({"Max Partitions", std::to_string(max_partitions_)},
                    widths);
  std::cout << std::endl;
}

// Helper function to validate a solution and ensure consistency
bool GeneticTechPartitioner::ValidateSolution(GeneticSolution &solution,
                                              bool repair_if_invalid) {
  bool is_valid = true;
  // Check if partitions are valid
  if (solution.partition.empty()) {
#pragma omp critical(console_output)
    { Console::Warning("Solution has empty partition vector"); }
    is_valid = false;
  }

  // Check if num_partitions is consistent with actual partitions
  int max_partition_id = -1;
  for (int p : solution.partition) {
    if (p > max_partition_id) {
      max_partition_id = p;
    }
    if (p < 0) {
#pragma omp critical(console_output)
      {
        Console::Warning("Solution has negative partition ID: " +
                         std::to_string(p));
      }
      is_valid = false;
    }
  }

  // Actual number of partitions is max_id + 1
  int actual_num_partitions = max_partition_id + 1;

  if (solution.num_partitions != actual_num_partitions) {
#pragma omp critical(console_output)
    {
      Console::Warning("Solution num_partitions (" +
                       std::to_string(solution.num_partitions) +
                       ") doesn't match actual partitions (" +
                       std::to_string(actual_num_partitions) + "), adjusting");
    }
    // Update num_partitions to match actual partitions
    solution.num_partitions = actual_num_partitions;
  }

  // Check if tech_nodes has the right size
  if (solution.tech_nodes.size() != solution.num_partitions) {
#pragma omp critical(console_output)
    {
      Console::Warning("Solution tech_nodes size (" +
                       std::to_string(solution.tech_nodes.size()) +
                       ") doesn't match num_partitions (" +
                       std::to_string(solution.num_partitions) +
                       "), adjusting");
    }

    // If tech_nodes is larger than num_partitions, trim it
    if (solution.tech_nodes.size() > solution.num_partitions) {
      solution.tech_nodes.resize(solution.num_partitions);
    } else {
      // If tech_nodes is smaller, this needs more complex repair
      is_valid = false;
    }
  }

  // Check for empty tech nodes
  for (size_t i = 0; i < solution.tech_nodes.size(); ++i) {
    if (solution.tech_nodes[i].empty()) {
#pragma omp critical(console_output)
      {
        Console::Warning("Solution has empty tech node at index " +
                         std::to_string(i));
      }
      is_valid = false;
    }
  }

  // Repair if requested and if invalid
  if (!is_valid && repair_if_invalid) {
// Must repair in a critical section to avoid races
#pragma omp critical(solution_repair)
    { RepairSolution(solution); }
    return ValidateSolution(
        solution, false); // Validate again but don't repair to avoid loop
  }

  return is_valid;
}

// Run the genetic algorithm
GeneticSolution GeneticTechPartitioner::Run(
    const std::string &chiplet_io_file, const std::string &chiplet_layer_file,
    const std::string &chiplet_wafer_process_file,
    const std::string &chiplet_assembly_process_file,
    const std::string &chiplet_test_file,
    const std::string &chiplet_netlist_file,
    const std::string &chiplet_blocks_file, float reach, float separation) {

  auto start_time = std::chrono::high_resolution_clock::now();

  Console::Header("Starting Genetic Partitioning with Technology Assignment");

  // Initialize population
  InitializePopulation(
      chiplet_io_file, chiplet_layer_file, chiplet_wafer_process_file,
      chiplet_assembly_process_file, chiplet_test_file, chiplet_netlist_file,
      chiplet_blocks_file, reach, separation);

  // Variables to track progress and convergence
  int no_improvement_counter = 0;
  const int max_no_improvement_generations = 10; // Convergence criterion
  float prev_best_cost = std::numeric_limits<float>::max();

  // Main evolutionary loop
  for (int generation = 0; generation < num_generations_; ++generation) {
    Console::Subheader("Generation " + std::to_string(generation + 1) + " of " +
                       std::to_string(num_generations_));

    // Evaluate fitness for all individuals
    float total_cost = 0.0f;
    float best_generation_cost = std::numeric_limits<float>::max();
    float worst_generation_cost = 0.0f;
    int best_index = -1;

    // Make sure each thread has a valid copy of the tech nodes
    // Ensure each solution's tech_nodes is valid before parallel evaluation
    for (auto &sol : population_) {
      if (sol.tech_nodes.size() != sol.num_partitions) {
#pragma omp critical(console_output)
        {
          Console::Warning(
              "Fixing tech_nodes size mismatch before fitness evaluation: " +
              std::to_string(sol.tech_nodes.size()) + " vs " +
              std::to_string(sol.num_partitions));
        }
        ValidateSolution(sol, true);
      }
    }

#pragma omp parallel for reduction(+:total_cost) schedule(dynamic) num_threads(omp_utils::get_max_threads())
    for (int i = 0; i < population_.size(); ++i) {
      // Skip invalid solutions to prevent segfaults
      if (population_[i].tech_nodes.empty() ||
          population_[i].partition.empty()) {
#pragma omp critical(console_output)
        { Console::Warning("Skipping invalid solution in fitness evaluation"); }
        continue;
      }

      // Make sure each thread sees the technology nodes array size matches
      // num_partitions
      if (population_[i].tech_nodes.size() != population_[i].num_partitions) {
#pragma omp critical(console_output)
        {
          Console::Error("Tech nodes size mismatch in fitness evaluation: " +
                         std::to_string(population_[i].tech_nodes.size()) +
                         " vs " +
                         std::to_string(population_[i].num_partitions));
        }
        continue;
      }

      // Evaluate fitness of each individual
      float cost = 0.0f;
      try {
        cost = EvaluateFitness(population_[i]);
      } catch (const std::exception &e) {
#pragma omp critical(console_output)
        {
          Console::Error("Exception in fitness evaluation: " +
                         std::string(e.what()));
        }
        continue;
      }

      // std::cout << "Evaluating solution " << i + 1 << " with cost: " << cost
      // << std::endl;
      // Update statistics (thread-safe using reduction)
      total_cost += cost;

// Update best solution (needs mutex)
#pragma omp critical
      {
        if (cost < best_generation_cost) {
          best_generation_cost = cost;
          best_index = i;
        }
        if (cost > worst_generation_cost) {
          worst_generation_cost = cost;
        }
      }
    }

    // Update best solution across all generations
    if (best_index >= 0 && best_index < population_.size() &&
        population_[best_index].cost < best_solution_.cost) {
      // Deep copy to avoid reference issues
      best_solution_ = population_[best_index];

      // Extra validation for best solution
      if (best_solution_.tech_nodes.size() != best_solution_.num_partitions) {
        Console::Error("Best solution has tech node size mismatch: " +
                       std::to_string(best_solution_.tech_nodes.size()) +
                       " vs " + std::to_string(best_solution_.num_partitions));

        // Fix the best solution
        ValidateSolution(best_solution_, true);
      }
    }

    // Calculate average cost
    // Only include valid solutions in the average calculation
    int valid_solutions = 0;
    float valid_total_cost = 0.0f;
    for (const auto &sol : population_) {
      if (sol.cost < std::numeric_limits<float>::max() &&
          !std::isinf(sol.cost) && !std::isnan(sol.cost)) {
        valid_total_cost += sol.cost;
        valid_solutions++;
      }
    }
    float avg_cost =
        valid_solutions == 0 ? 0.0f : valid_total_cost / valid_solutions;
    std::cout << "Total cost (valid solutions): " << valid_total_cost
              << std::endl;
    std::cout << "Valid solutions: " << valid_solutions << " out of "
              << population_.size() << std::endl;

    // Print statistics
    PrintGenerationStats(generation, best_generation_cost, avg_cost);

    // Check for convergence
    if (std::abs(best_generation_cost - prev_best_cost) <
        0.001f * prev_best_cost) {
      no_improvement_counter++;
      if (no_improvement_counter >= max_no_improvement_generations) {
        Console::Success("Convergence reached after " +
                         std::to_string(generation + 1) +
                         " generations with best cost: " +
                         std::to_string(best_solution_.cost));
        break;
      }
    } else {
      no_improvement_counter = 0;
      prev_best_cost = best_generation_cost;
    }

    // Create new population
    std::vector<GeneticSolution> new_population;
    new_population.reserve(population_size_);

    // Elitism: Always keep the best solution
    if (ValidateSolution(best_solution_, true)) {
      new_population.push_back(best_solution_);
    } else {
      Console::Error("Failed to validate best solution for elitism");
    }

    // Fill the rest of the population with offspring
    while (new_population.size() < population_size_) {
      // Select parents
      std::vector<int> parent_indices = SelectParents();

      // Ensure parent indices are valid
      if (parent_indices.size() < 2 || parent_indices[0] < 0 ||
          parent_indices[0] >= population_.size() || parent_indices[1] < 0 ||
          parent_indices[1] >= population_.size()) {
        Console::Error("Invalid parent indices for crossover");
        continue;
      }

      // Validate parent solutions
      if (!ValidateSolution(population_[parent_indices[0]], true) ||
          !ValidateSolution(population_[parent_indices[1]], true)) {
        Console::Error("Invalid parent solution for crossover");
        continue;
      }

      // Perform crossover with probability crossover_rate_
      bool do_crossover = std::uniform_real_distribution<float>(0.0f, 1.0f)(
                              rng_) < crossover_rate_;

      if (do_crossover) {
        try {
          GeneticSolution offspring = Crossover(population_[parent_indices[0]],
                                                population_[parent_indices[1]]);

          // Validate offspring before mutation
          if (!ValidateSolution(offspring, true)) {
            Console::Error("Invalid offspring after crossover");
            continue;
          }

          // Perform mutation with probability mutation_rate_
          if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) <
              mutation_rate_) {
#pragma omp critical(mutation)
            { Mutate(offspring); }
          }

// Repair solution to ensure consistency
#pragma omp critical(repair)
          { RepairSolution(offspring); }

          // Final validation before adding to new population
          if (ValidateSolution(offspring, true)) {
            // Add to new population
            new_population.push_back(offspring);
          } else {
            Console::Error("Invalid offspring after repair");
          }
        } catch (const std::exception &e) {
          Console::Error("Exception during crossover or mutation: " +
                         std::string(e.what()));
        }
      } else {
        // Add copies of parents with possible mutation
        for (int idx : parent_indices) {
          if (new_population.size() < population_size_) {
            GeneticSolution offspring = population_[idx];

            // Perform mutation with probability mutation_rate_
            if (std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) <
                mutation_rate_) {
#pragma omp critical(mutation)
              { Mutate(offspring); }

#pragma omp critical(repair)
              { RepairSolution(offspring); }
            }

            // Final validation before adding
            if (ValidateSolution(offspring, true)) {
              new_population.push_back(offspring);
            } else {
              Console::Error("Invalid parent copy after mutation");
            }
          }
        }
      }
    }

    // Replace old population with new
    population_ = std::move(new_population);

    // Validate the entire population after generation
    int invalid_count = 0;
    for (auto &sol : population_) {
      if (!ValidateSolution(sol, true)) {
        invalid_count++;
      }
    }

    if (invalid_count > 0) {
      Console::Warning("Generation " + std::to_string(generation + 1) +
                       " has " + std::to_string(invalid_count) +
                       " invalid solutions after repair");
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                      end_time - start_time)
                      .count();

  Console::Header("Genetic Algorithm Results");
  std::vector<std::string> columns = {"Metric", "Value"};
  std::vector<int> widths = {30, 20};
  Console::TableHeader(columns, widths);

  Console::TableRow({"Best Cost", std::to_string(best_solution_.cost)}, widths);
  Console::TableRow(
      {"Number of Partitions", std::to_string(best_solution_.num_partitions)},
      widths);
  Console::TableRow({"Valid Solution", best_solution_.valid ? "Yes" : "No"},
                    widths);
  Console::TableRow({"Execution Time (ms)", std::to_string(duration)}, widths);
  std::cout << std::endl;

  Console::Subheader("Technology Assignment");
  std::vector<std::string> tech_columns = {"Partition", "Technology"};
  Console::TableHeader(tech_columns, widths);

  for (int i = 0; i < best_solution_.tech_nodes.size(); ++i) {
    Console::TableRow({std::to_string(i), best_solution_.tech_nodes[i]},
                      widths);
  }
  std::cout << std::endl;

  return best_solution_;
}

// Initialize the population with diverse solutions
void GeneticTechPartitioner::InitializePopulation(
    const std::string &chiplet_io_file, const std::string &chiplet_layer_file,
    const std::string &chiplet_wafer_process_file,
    const std::string &chiplet_assembly_process_file,
    const std::string &chiplet_test_file,
    const std::string &chiplet_netlist_file,
    const std::string &chiplet_blocks_file, float reach, float separation) {

  Console::Info("Initializing population with diverse solutions...");

  // Initialize the refiner for cost evaluation
  std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);
  refiner_ = std::make_shared<ChipletRefiner>(
      max_partitions_, // num_parts (use max to be safe)
      2,               // refiner_iters
      static_cast<int>(hypergraph_->GetNumVertices() * 0.05), // max_move
      reaches,                                                // reaches
      true,                                                  // floorplanning
      chiplet_io_file,                                        // io_file
      chiplet_layer_file,                                     // layer_file
      chiplet_wafer_process_file,    // wafer_process_file
      chiplet_assembly_process_file, // assembly_process_file
      chiplet_test_file,             // test_file
      chiplet_netlist_file,          // netlist_file
      chiplet_blocks_file            // blocks_file
  );

  // Check if cost model is initialized
  if (!refiner_->IsCostModelInitialized()) {
    Console::Error("Cost model initialization failed!");
    return;
  }

  // Generate initial partitions
  std::vector<std::vector<int>> initial_partitions =
      CreateInitialPartitions(min_partitions_, max_partitions_);

  Console::Info("Created " + std::to_string(initial_partitions.size()) +
                " initial partitions with varying numbers of partitions.");

  // Reserve space for population
  population_.reserve(population_size_);

  // Create diverse initial population
  for (int i = 0; i < population_size_; ++i) {
    // If we have initial partitions, use them first
    if (i < initial_partitions.size()) {
      // Find the actual number of partitions in this partition
      int num_parts = *std::max_element(initial_partitions[i].begin(),
                                        initial_partitions[i].end()) +
                      1;

      // Generate tech assignment
      std::vector<std::string> tech_assignment =
          GenerateRandomTechAssignment(num_parts);
      // Create and add the solution
      GeneticSolution solution(num_parts, initial_partitions[i],
                               tech_assignment, 0.0f, true);
      population_.push_back(solution);
    } else {
      // For remaining slots, create random solutions

      // Choose random number of partitions
      int num_parts = std::uniform_int_distribution<int>(min_partitions_,
                                                         max_partitions_)(rng_);

      // Create random partition
      std::vector<int> partition(hypergraph_->GetNumVertices());
      for (int j = 0; j < hypergraph_->GetNumVertices(); ++j) {
        partition[j] =
            std::uniform_int_distribution<int>(0, num_parts - 1)(rng_);
      }

      // Generate tech assignment
      std::vector<std::string> tech_assignment =
          GenerateRandomTechAssignment(num_parts);

      // Create and add the solution
      auto floor_result =
          refiner_->RunFloorplanner(partition, hypergraph_, 100, 100, 0.00001);
      std::vector<float> result_aspect_ratios = std::get<0>(floor_result);
      std::vector<float> result_x_locations = std::get<1>(floor_result);
      std::vector<float> result_y_locations = std::get<2>(floor_result);
      bool success = std::get<3>(floor_result);
      refiner_->SetAspectRatios(result_aspect_ratios);
      refiner_->SetXLocations(result_x_locations);
      refiner_->SetYLocations(result_y_locations);
      refiner_->SetTechArray(tech_assignment);
      refiner_->SetNumParts(num_parts);
      // run the refiner
      Matrix<float> upper_block_balance(
          num_parts,
          std::vector<float>(hypergraph_->GetVertexDimensions(), 0.0));
      Matrix<float> lower_block_balance(
          num_parts,
          std::vector<float>(hypergraph_->GetVertexDimensions(), 0.0));
      std::vector<float> total_weights = hypergraph_->GetTotalVertexWeights();
      // Calculate total weights for this specific partition
      for (int j = 0; j < hypergraph_->GetVertexDimensions(); j++) {
        for (int k = 0; k < num_parts; k++) {
          upper_block_balance[k][j] =
              std::accumulate(total_weights.begin(), total_weights.end(),
                              0.0f) *
              ub_factor_ / num_parts;
        }
      }
      refiner_->Refine(hypergraph_, upper_block_balance, lower_block_balance,
                       partition);
      // adjust the num_parts and tech assignment
      num_parts = *std::max_element(partition.begin(), partition.end()) + 1;
      float cost = refiner_->GetCostFromScratch(partition);
      GeneticSolution solution(num_parts, partition, tech_assignment, cost,
                               success);
      population_.push_back(solution);
    }
  }

  Console::Success("Population initialized with " +
                   std::to_string(population_.size()) + " individuals.");
}

// Evaluate fitness for a solution
float GeneticTechPartitioner::EvaluateFitness(GeneticSolution &solution) {
  // If we already have the fitness value, return it
  if (solution.cost != 0.0 &&
      solution.cost < std::numeric_limits<float>::max()) {
    // std::cout << "[Fitness] Solution cost: " << solution.cost << std::endl;
    return solution.cost;
  }

  // CRITICAL FIX: Find the actual number of partitions in the solution
  // This ensures we're working with the true partition count
  int actual_num_partitions = 0;
  if (!solution.partition.empty()) {
    for (int p : solution.partition) {
      actual_num_partitions = std::max(actual_num_partitions, p + 1);
    }

    // Update num_partitions if it doesn't match the actual count
    if (solution.num_partitions != actual_num_partitions) {
#pragma omp critical(console_output)
      {
        Console::Warning("EvaluateFitness: Solution num_partitions (" +
                         std::to_string(solution.num_partitions) +
                         ") doesn't match actual partitions (" +
                         std::to_string(actual_num_partitions) + "), fixing");
      }
      solution.num_partitions = actual_num_partitions;
    }
  }

  // Verify solution has valid data before attempting to evaluate
  if (solution.partition.empty()) {
#pragma omp critical(console_output)
    {
      Console::Error(
          "Cannot evaluate fitness of solution with empty partition");
    }
    // std::cout << "[Fitness] Solution has empty partition" << std::endl;
    solution.valid = false;
    return std::numeric_limits<float>::max();
  }

  // Special handling for tech_nodes - never reject a solution just because
  // tech_nodes is wrong Instead, always fix it

  // If tech_nodes is completely empty, create a new one
  if (solution.tech_nodes.empty()) {
#pragma omp critical(console_output)
    {
      Console::Warning(
          "EvaluateFitness: Solution has empty tech_nodes, creating new ones");
    }

    // Generate new tech_nodes
    solution.tech_nodes.resize(solution.num_partitions);
    for (int i = 0; i < solution.num_partitions; ++i) {
      if (!available_tech_nodes_.empty()) {
        int tech_idx = std::uniform_int_distribution<int>(
            0, available_tech_nodes_.size() - 1)(rng_);
        solution.tech_nodes[i] = available_tech_nodes_[tech_idx];
      } else {
        solution.tech_nodes[i] = "7nm"; // Default fallback
      }
    }
    // std::cout << "[Fitness] Created new tech nodes array with size " <<
    // solution.tech_nodes.size() << std::endl;
  }

  // Ensure tech_nodes size matches num_partitions - always fix, never reject
  if (solution.tech_nodes.size() != solution.num_partitions) {
#pragma omp critical(console_output)
    {
      Console::Warning("EvaluateFitness: Tech nodes size (" +
                       std::to_string(solution.tech_nodes.size()) +
                       ") doesn't match num_partitions (" +
                       std::to_string(solution.num_partitions) +
                       "), fixing it");
    }

    // Fix tech_nodes size before proceeding
    if (solution.tech_nodes.size() > solution.num_partitions) {
      // Trim the tech_nodes
      solution.tech_nodes.resize(solution.num_partitions);
      // std::cout << "[Fitness] Trimmed tech nodes to size " <<
      // solution.tech_nodes.size() << std::endl;
    } else {
      // Expand tech_nodes
      std::vector<std::string> old_tech = solution.tech_nodes;
      solution.tech_nodes.resize(solution.num_partitions);

      // Fill new tech nodes
      for (int i = old_tech.size(); i < solution.num_partitions; ++i) {
        if (!available_tech_nodes_.empty()) {
#pragma omp critical(rng_access)
          {
            int tech_idx = std::uniform_int_distribution<int>(
                0, available_tech_nodes_.size() - 1)(rng_);

            if (tech_idx >= 0 && tech_idx < available_tech_nodes_.size()) {
              solution.tech_nodes[i] = available_tech_nodes_[tech_idx];
            } else {
              solution.tech_nodes[i] = "7nm"; // Default fallback
            }
          }
        } else {
          solution.tech_nodes[i] = "7nm"; // Default fallback
        }
      }
      // std::cout << "[Fitness] Expanded tech nodes to size " <<
      // solution.tech_nodes.size() << std::endl;
    }
  }

  // Check for empty tech nodes and fix them
  for (size_t i = 0; i < solution.tech_nodes.size(); ++i) {
    if (solution.tech_nodes[i].empty()) {
#pragma omp critical(console_output)
      {
        Console::Warning("EvaluateFitness: Empty tech node at index " +
                         std::to_string(i) + ", fixing");
      }

      // Assign a valid tech node
      if (!available_tech_nodes_.empty()) {
#pragma omp critical(rng_access)
        {
          int tech_idx = std::uniform_int_distribution<int>(
              0, available_tech_nodes_.size() - 1)(rng_);
          solution.tech_nodes[i] = available_tech_nodes_[tech_idx];
        }
      } else {
        solution.tech_nodes[i] = "7nm"; // Default fallback
      }
    }
  }

  // Create a hash key for caching
  std::string key = std::to_string(solution.num_partitions) + ":";
  for (int p : solution.partition) {
    key += std::to_string(p) + ",";
  }
  key += ":";
  for (const auto &tech : solution.tech_nodes) {
    key += tech + ",";
  }

  // Check cache - must be thread-safe
  float cached_cost = std::numeric_limits<float>::max();
  bool cache_hit = false;

  {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    if (cost_cache_.find(key) != cost_cache_.end()) {
      cached_cost = cost_cache_[key];
      // Only use cache if it's a valid cost (not maximum float)
      if (cached_cost < std::numeric_limits<float>::max() - 1.0) {
        cache_hit = true;
      }
    }
  }

  if (cache_hit) {
    solution.cost = cached_cost;
    return solution.cost;
  }

// Thread safety for accessing the refiner
#pragma omp critical(refiner_access)
  {
    try {
      // Validate tech nodes one more time before setting in refiner
      bool all_tech_valid = true;
      for (size_t i = 0; i < solution.tech_nodes.size(); ++i) {
        if (solution.tech_nodes[i].empty()) {
          all_tech_valid = false;
          solution.tech_nodes[i] = "7nm"; // Emergency fix
        }
      }

      if (!all_tech_valid) {
        Console::Warning("Had to fix empty tech nodes right before evaluation");
      }

      // Set technology array in refiner - CRITICAL!
      refiner_->SetTechArray(solution.tech_nodes);

      // Setup balance constraints
      Matrix<float> upper_block_balance;
      Matrix<float> lower_block_balance;

      std::vector<float> zero_vector(hypergraph_->GetVertexDimensions(), 0.0f);
      // removing balance constraints
      for (int i = 0; i < solution.num_partitions; i++) {
        upper_block_balance.emplace_back(hypergraph_->GetTotalVertexWeights());
        lower_block_balance.emplace_back(zero_vector);
      }

      // TEMPORARILY DISABLE FLOORPLANNER
      // auto floor_result = refiner_->RunFloorplanner(solution.partition,
      // hypergraph_, 100, 100, 0.00001); std::vector<float>
      // result_aspect_ratios = std::get<0>(floor_result); std::vector<float>
      // result_x_locations = std::get<1>(floor_result); std::vector<float>
      // result_y_locations = std::get<2>(floor_result); bool success =
      // std::get<3>(floor_result);
      // refiner_->SetAspectRatios(result_aspect_ratios);
      // refiner_->SetXLocations(result_x_locations);
      // refiner_->SetYLocations(result_y_locations);

      // CRITICAL: Set tech array again right before cost calculation
      refiner_->SetTechArray(solution.tech_nodes);

      // ADD DEBUG LOGGING
      // std::cout << "[Fitness Debug] Evaluating solution with " <<
      // solution.num_partitions
      //          << " partitions, tech array size: " <<
      //          solution.tech_nodes.size() << std::endl;

      // Evaluate the cost with robust error handling
      float cost = 0.0f;
      bool success = false;
      try {
        // Safety check for consistent partition sizes
        if (solution.partition.size() != hypergraph_->GetNumVertices()) {
          Console::Error("Partition size (" +
                         std::to_string(solution.partition.size()) +
                         ") doesn't match hypergraph vertices (" +
                         std::to_string(hypergraph_->GetNumVertices()) + ")");
          // Fix by resizing if possible
          if (solution.partition.size() < hypergraph_->GetNumVertices()) {
            solution.partition.resize(hypergraph_->GetNumVertices(), 0);
          }
        }

        // One more tech array set for safety
        auto floor_result = refiner_->RunFloorplanner(
            solution.partition, hypergraph_, 100, 100, 0.00001);
        std::vector<float> result_aspect_ratios = std::get<0>(floor_result);
        std::vector<float> result_x_locations = std::get<1>(floor_result);
        std::vector<float> result_y_locations = std::get<2>(floor_result);
        success = std::get<3>(floor_result);
        refiner_->SetAspectRatios(result_aspect_ratios);
        refiner_->SetXLocations(result_x_locations);
        refiner_->SetYLocations(result_y_locations);
        refiner_->SetTechArray(solution.tech_nodes);
        refiner_->SetNumParts(solution.num_partitions);
        // run the refiner
        Matrix<float> upper_block_balance(
            solution.num_partitions,
            std::vector<float>(hypergraph_->GetVertexDimensions(), 0.0));
        Matrix<float> lower_block_balance(
            solution.num_partitions,
            std::vector<float>(hypergraph_->GetVertexDimensions(), 0.0));
        std::vector<float> total_weights = hypergraph_->GetTotalVertexWeights();
        // Calculate total weights for this specific partition
        for (int j = 0; j < hypergraph_->GetVertexDimensions(); j++) {
          for (int k = 0; k < solution.num_partitions; k++) {
            upper_block_balance[k][j] =
                std::accumulate(total_weights.begin(), total_weights.end(),
                                0.0f) *
                ub_factor_ / solution.num_partitions;
          }
        }
        refiner_->Refine(hypergraph_, upper_block_balance, lower_block_balance,
                         solution.partition);
        // adjust the num_parts and tech assignment
        solution.num_partitions = *std::max_element(solution.partition.begin(),
                                                    solution.partition.end()) +
                                  1;
        // Get the cost
        cost = refiner_->GetCostFromScratch(solution.partition);

        // Sanity check on cost
        if (std::isinf(cost) || std::isnan(cost)) {
          // std::cout << "[Fitness Debug] Cost is invalid (inf/nan), setting to
          // max float" << std::endl;
          cost = std::numeric_limits<float>::max();
        }
      } catch (const std::exception &e) {
        // std::cout << "[Fitness Debug] Exception in GetCostFromScratch: " <<
        // e.what() << std::endl;
        cost = std::numeric_limits<float>::max();
        success = false;
      }

      // Update solution
      solution.valid = success;
      
      // CRITICAL CHANGE: Ensure invalid solutions have maximum cost
      // This will prevent them from being selected as the best solution
      if (!success) {
#pragma omp critical(console_output)
        {
          Console::Warning("EvaluateFitness: Invalid solution, enforcing maximum cost penalty");
        }
        cost = std::numeric_limits<float>::max();
      }
      
      solution.cost = cost;

      // Only cache the cost if it's valid
      if (cost < std::numeric_limits<float>::max() - 1.0) {
        // Update cache (thread-safe)
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cost_cache_[key] = cost;
      }
    } catch (const std::exception &e) {
#pragma omp critical(console_output)
      {
        Console::Error("Exception in refiner: " + std::string(e.what()));
        std::cout << "Tech nodes dump: ";
        for (const auto &tech : solution.tech_nodes) {
          std::cout << "'" << tech << "' ";
        }
        std::cout << std::endl;
      }
      // std::cout << "[Fitness] Exception in refiner: " << e.what() <<
      // std::endl;
      solution.cost = std::numeric_limits<float>::max();
    }
  }

  return solution.cost;
}

// Select parents for reproduction using tournament selection
std::vector<int> GeneticTechPartitioner::SelectParents() {
  const int tournament_size = 3; // Number of individuals in each tournament
  std::vector<int> selected_indices;

  // Make sure we have valid population
  if (population_.empty()) {
#pragma omp critical(console_output)
    { Console::Error("Cannot select parents from empty population"); }
    return selected_indices; // Return empty vector
  }

  // Thread-local copy of RNG for thread safety
  std::mt19937 local_rng;

#pragma omp critical(rng_access)
  {
    local_rng = rng_; // Make a thread-local copy of the RNG
  }

  for (int i = 0; i < 2; ++i) { // Select two parents
    // Randomly select tournament_size individuals
    std::vector<int> tournament;

    // Thread-safe selection of tournament participants
    for (int j = 0; j < tournament_size; ++j) {
      int idx = std::uniform_int_distribution<int>(0, population_.size() -
                                                          1)(local_rng);

      // Validate index
      if (idx < 0 || idx >= population_.size()) {
#pragma omp critical(console_output)
        {
          Console::Warning("Invalid index " + std::to_string(idx) +
                           " in tournament selection, using 0 instead");
        }
        idx = 0;
      }

      tournament.push_back(idx);
    }

    // Find the best individual in the tournament
    int best_idx = tournament[0];
    bool valid_found = false;

    for (int j = 0; j < tournament.size(); ++j) {
      int idx = tournament[j];

      // Skip invalid indices
      if (idx < 0 || idx >= population_.size()) {
        continue;
      }

      // Skip solutions with invalid cost
      if (population_[idx].cost >= std::numeric_limits<float>::max() ||
          std::isnan(population_[idx].cost)) {
        continue;
      }

      if (!valid_found || population_[idx].cost < population_[best_idx].cost) {
        best_idx = idx;
        valid_found = true;
      }
    }

    // Double-check that we have a valid index
    if (best_idx < 0 || best_idx >= population_.size()) {
#pragma omp critical(console_output)
      {
        Console::Error("Invalid best index " + std::to_string(best_idx) +
                       " in tournament selection, using 0 instead");
      }
      best_idx = 0;
    }

    // Validate the selected parent
    if (!ValidateSolution(population_[best_idx], true)) {
#pragma omp critical(console_output)
      {
        Console::Warning("Selected parent " + std::to_string(best_idx) +
                         " is invalid, attempting repair");
      }
    }

    selected_indices.push_back(best_idx);
  }

  return selected_indices;
}

// Perform crossover between two parents
GeneticSolution
GeneticTechPartitioner::Crossover(const GeneticSolution &parent1,
                                  const GeneticSolution &parent2) {

  // Validate parents before crossover
  if (parent1.partition.empty() || parent2.partition.empty() ||
      parent1.tech_nodes.empty() || parent2.tech_nodes.empty()) {
#pragma omp critical(console_output)
    { Console::Error("Cannot perform crossover with invalid parents"); }
    // Return copy of first parent as fallback
    return parent1;
  }

  // Verify tech_nodes sizes match num_partitions
  if (parent1.tech_nodes.size() != parent1.num_partitions ||
      parent2.tech_nodes.size() != parent2.num_partitions) {
#pragma omp critical(console_output)
    { Console::Error("Parent tech_nodes size mismatch in crossover"); }
    // Return copy of first parent as fallback
    return parent1;
  }

  // Thread-local copy of RNG for thread safety
  std::mt19937 local_rng;

#pragma omp critical(rng_access)
  {
    local_rng = rng_; // Make a thread-local copy of the RNG
  }

  // Randomly decide which crossover method to use
  int method = std::uniform_int_distribution<int>(0, 2)(local_rng);

  if (method == 0) {
    // Method 1: Partition crossover (one-point)
    int crossover_point = std::uniform_int_distribution<int>(
        0, hypergraph_->GetNumVertices() - 1)(local_rng);

    // Validate crossover point
    if (crossover_point < 0 ||
        crossover_point >= hypergraph_->GetNumVertices()) {
#pragma omp critical(console_output)
      {
        Console::Error("Invalid crossover point: " +
                       std::to_string(crossover_point));
      }
      return parent1;
    }

    // Validate parent partitions
    if (parent1.partition.size() != hypergraph_->GetNumVertices() ||
        parent2.partition.size() != hypergraph_->GetNumVertices()) {
#pragma omp critical(console_output)
      { Console::Error("Parent partition size mismatch in crossover"); }
      return parent1;
    }

    std::vector<int> child_partition = parent1.partition;
    for (int i = crossover_point; i < hypergraph_->GetNumVertices(); ++i) {
      child_partition[i] = parent2.partition[i];
    }

    // Use tech assignment from the parent with better cost
    std::vector<std::string> child_tech;

    if (parent1.cost < parent2.cost && parent1.tech_nodes.size() > 0) {
      child_tech = parent1.tech_nodes;
    } else if (parent2.tech_nodes.size() > 0) {
      child_tech = parent2.tech_nodes;
    } else {
      // Fallback if both parents have invalid tech assignments
      child_tech.push_back("7nm");
    }

    // Find the actual number of partitions
    int num_parts = 0;
    for (int pid : child_partition) {
      num_parts = std::max(num_parts, pid + 1);
    }

    // Ensure num_parts is at least 1
    num_parts = std::max(1, num_parts);

    // Ensure child_tech has the right size
    if (child_tech.size() != num_parts) {
#pragma omp critical(console_output)
      {
        Console::Warning("Adjusting child tech size from " +
                         std::to_string(child_tech.size()) + " to " +
                         std::to_string(num_parts));
      }

      // Preserve existing tech
      std::vector<std::string> old_tech = child_tech;
      child_tech.resize(num_parts);

      // Fill in missing tech nodes
      for (int i = 0; i < num_parts; ++i) {
        if (i < old_tech.size()) {
          child_tech[i] = old_tech[i];
        } else if (!available_tech_nodes_.empty()) {
          int tech_idx = std::uniform_int_distribution<int>(
              0, available_tech_nodes_.size() - 1)(local_rng);

          if (tech_idx >= 0 && tech_idx < available_tech_nodes_.size()) {
            child_tech[i] = available_tech_nodes_[tech_idx];
          } else {
            child_tech[i] = "7nm"; // Default as fallback
          }
        }
      }
    }

    // Create child solution
    GeneticSolution child(num_parts, child_partition, child_tech, 0.0f, true);
    return child;
  } else if (method == 1) {
    // Method 2: Tech crossover (uniform)
    // Use partition from the better parent
    std::vector<int> child_partition;
    if (parent1.cost < parent2.cost && !parent1.partition.empty()) {
      child_partition = parent1.partition;
    } else if (!parent2.partition.empty()) {
      child_partition = parent2.partition;
    } else {
      // Fallback for invalid parents
      child_partition.resize(hypergraph_->GetNumVertices(), 0);
    }

    // Find the actual number of partitions
    int num_parts = 0;
    for (int pid : child_partition) {
      num_parts = std::max(num_parts, pid + 1);
    }

    // Ensure num_parts is at least 1
    num_parts = std::max(1, num_parts);

    // Create a mixed tech assignment
    std::vector<std::string> child_tech(num_parts);
    for (int i = 0; i < num_parts; ++i) {
      bool use_parent1 =
          std::uniform_real_distribution<float>(0.0f, 1.0f)(local_rng) < 0.5f;

      if (use_parent1 && i < parent1.tech_nodes.size() &&
          !parent1.tech_nodes[i].empty()) {
        child_tech[i] = parent1.tech_nodes[i];
      } else if (i < parent2.tech_nodes.size() &&
                 !parent2.tech_nodes[i].empty()) {
        child_tech[i] = parent2.tech_nodes[i];
      } else if (!available_tech_nodes_.empty()) {
        // Use random tech from available
        int tech_idx = std::uniform_int_distribution<int>(
            0, available_tech_nodes_.size() - 1)(local_rng);

        if (tech_idx >= 0 && tech_idx < available_tech_nodes_.size()) {
          child_tech[i] = available_tech_nodes_[tech_idx];
        } else {
          child_tech[i] = "7nm"; // Default as fallback
        }
      } else {
        child_tech[i] = "7nm"; // Default tech node as fallback
      }
    }

    // Create child solution
    GeneticSolution child(num_parts, child_partition, child_tech, 0.0f, true);
    return child;
  } else {
    // Method 3: Hybrid crossover (partition count from one parent, tech
    // assignment strategy from other) Randomly choose which parent to take the
    // partition count from
    bool use_parent1_partition_count =
        std::uniform_real_distribution<float>(0.0f, 1.0f)(local_rng) < 0.5f;

    const GeneticSolution &partition_parent =
        use_parent1_partition_count ? parent1 : parent2;

    // Check if partition_parent has valid num_partitions
    int partition_count = partition_parent.num_partitions;
    if (partition_count <= 0) {
#pragma omp critical(console_output)
      {
        Console::Warning("Invalid partition count in hybrid crossover: " +
                         std::to_string(partition_count) + ", using 2 instead");
      }
      partition_count = 2;
    }

    // Create a new partition with the chosen parent's partition count
    std::vector<int> child_partition(hypergraph_->GetNumVertices());
    for (int i = 0; i < hypergraph_->GetNumVertices(); ++i) {
      child_partition[i] =
          std::uniform_int_distribution<int>(0, partition_count - 1)(local_rng);
    }

    // Mix tech assignments from both parents
    std::vector<std::string> child_tech(partition_count);
    for (int i = 0; i < partition_count; ++i) {
      bool use_parent1_tech =
          std::uniform_real_distribution<float>(0.0f, 1.0f)(local_rng) < 0.5f;

      if (use_parent1_tech && i < parent1.tech_nodes.size() &&
          !parent1.tech_nodes[i].empty()) {
        child_tech[i] = parent1.tech_nodes[i % parent1.tech_nodes.size()];
      } else if (i < parent2.tech_nodes.size() &&
                 !parent2.tech_nodes[i].empty()) {
        child_tech[i] = parent2.tech_nodes[i % parent2.tech_nodes.size()];
      } else if (!available_tech_nodes_.empty()) {
        // Use random tech
        int tech_idx = std::uniform_int_distribution<int>(
            0, available_tech_nodes_.size() - 1)(local_rng);

        if (tech_idx >= 0 && tech_idx < available_tech_nodes_.size()) {
          child_tech[i] = available_tech_nodes_[tech_idx];
        } else {
          child_tech[i] = "7nm"; // Default as fallback
        }
      } else {
        child_tech[i] = "7nm"; // Default tech node as fallback
      }
    }

    // Create child solution
    GeneticSolution child(partition_count, child_partition, child_tech, 0.0f,
                          true);
    return child;
  }
}

// Modify the Mutate function to improve bounds checking
void GeneticTechPartitioner::Mutate(GeneticSolution &solution) {
  // Validate solution before mutation
  if (!ValidateSolution(solution, true)) {
#pragma omp critical(console_output)
    {
      Console::Warning(
          "Starting mutation with invalid solution - attempting repair");
    }
  }

#pragma omp critical(console_output)
  {
    // std::cout << "[DEBUG] Starting mutation on solution with " <<
    // solution.num_partitions
    //          << " partitions and " << solution.tech_nodes.size() << " tech
    //          nodes" << std::endl;
  }

  // Ensure tech_nodes has correct size before proceeding
  if (solution.tech_nodes.size() != solution.num_partitions) {
#pragma omp critical(console_output)
    {
      Console::Warning("Tech nodes size does not match num_partitions, fixing");
    }

// This block modifies the solution, so it needs critical section
#pragma omp critical(solution_modify)
    {
      if (solution.tech_nodes.size() > solution.num_partitions) {
        // If too many tech nodes, trim the vector
        solution.tech_nodes.resize(solution.num_partitions);
      } else {
        // If too few tech nodes, add new ones
        std::vector<std::string> old_tech = solution.tech_nodes;
        solution.tech_nodes.resize(solution.num_partitions);

        // Fill new tech nodes with reasonable values
        for (int i = old_tech.size(); i < solution.num_partitions; ++i) {
          if (!available_tech_nodes_.empty()) {
            int tech_idx = std::uniform_int_distribution<int>(
                0, available_tech_nodes_.size() - 1)(rng_);

            if (tech_idx >= 0 && tech_idx < available_tech_nodes_.size()) {
              solution.tech_nodes[i] = available_tech_nodes_[tech_idx];
            } else {
              solution.tech_nodes[i] = "7nm"; // Default fallback
            }
          } else {
            solution.tech_nodes[i] = "7nm"; // Default fallback
          }
        }
      }
    }
  }

// Dump tech nodes for debugging
#pragma omp critical(console_output)
  {
    // std::cout << "[DEBUG] Tech nodes before mutation: ";
    // for (size_t i = 0; i < solution.tech_nodes.size(); ++i) {
    //  std::cout << "'" << solution.tech_nodes[i] << "' ";
    //}
    // std::cout << std::endl;
  }

  // Thread-local RNG to avoid contention
  std::mt19937 local_rng = rng_;

  // Randomly decide which mutation method to use
  int method;
  {
// Get a consistent method decision across threads
#pragma omp critical(rng_access)
    { method = std::uniform_int_distribution<int>(0, 2)(rng_); }
  }

#pragma omp critical(console_output)
  {
    // std::cout << "[DEBUG] Using mutation method: " << method << std::endl;
  }

  if (method == 0) {
    // Method 1: Partition mutation - move vertices between partitions
    int num_mutations =
        std::max(1, static_cast<int>(hypergraph_->GetNumVertices() * 0.05));

#pragma omp critical(console_output)
    {
      // std::cout << "[DEBUG] Partition mutation: will move " << num_mutations
      // << " vertices" << std::endl;
    }

    for (int i = 0; i < num_mutations; ++i) {
      // Ensure vertex index is valid
      int vertex;
      int new_partition;
      bool valid_indices = true;

// Thread-safe RNG access
#pragma omp critical(rng_access)
      {
        vertex = std::uniform_int_distribution<int>(
            0, hypergraph_->GetNumVertices() - 1)(rng_);

        // Ensure partition index is valid
        if (solution.num_partitions <= 0) {
#pragma omp critical(console_output)
          {
            Console::Error("Invalid number of partitions: " +
                           std::to_string(solution.num_partitions));
          }
          valid_indices = false;
        }

        if (valid_indices) {
          new_partition = std::uniform_int_distribution<int>(
              0, solution.num_partitions - 1)(rng_);
        }
      }

      // Check for valid indices
      if (valid_indices &&
          (vertex < 0 || vertex >= solution.partition.size())) {
#pragma omp critical(console_output)
        { Console::Error("Invalid vertex index: " + std::to_string(vertex)); }
        valid_indices = false;
      }

      if (valid_indices) {
#pragma omp critical(console_output)
        {
          // std::cout << "[DEBUG] Moving vertex " << vertex << " from partition
          // "
          //          << solution.partition[vertex] << " to " << new_partition
          //          << std::endl;
        }

// This modifies the solution, so it needs a critical section
#pragma omp critical(solution_modify)
        { solution.partition[vertex] = new_partition; }
      }
    }
  } else if (method == 1) {
    // Method 2: Tech mutation - change technology for some partitions
    int num_tech_mutations = std::max(1, solution.num_partitions / 3);

#pragma omp critical(console_output)
    {
      // std::cout << "[DEBUG] Tech mutation: will change " <<
      // num_tech_mutations << " partition technologies" << std::endl;
    }

    if (available_tech_nodes_.empty()) {
#pragma omp critical(console_output)
      {
        // std::cout << "[DEBUG] ERROR: available_tech_nodes_ is empty" <<
        // std::endl;
      }
      return;
    }

    for (int i = 0; i < num_tech_mutations; ++i) {
      // Thread-safe RNG for partition selection
      int partition;
      int new_tech_idx;
      bool valid_indices = true;

#pragma omp critical(rng_access)
      {
        // Ensure partition index is valid
        if (solution.num_partitions <= 0) {
#pragma omp critical(console_output)
          {
            Console::Error("Invalid number of partitions: " +
                           std::to_string(solution.num_partitions));
          }
          valid_indices = false;
        }

        if (valid_indices) {
          partition = std::uniform_int_distribution<int>(
              0, solution.num_partitions - 1)(rng_);

          // Ensure tech_idx is valid
          if (available_tech_nodes_.empty()) {
#pragma omp critical(console_output)
            { Console::Error("No available tech nodes"); }
            valid_indices = false;
          }
        }

        if (valid_indices) {
          new_tech_idx = std::uniform_int_distribution<int>(
              0, available_tech_nodes_.size() - 1)(rng_);
        }
      }

      if (valid_indices &&
          (partition < 0 || partition >= solution.tech_nodes.size())) {
#pragma omp critical(console_output)
        {
          // std::cout << "[DEBUG] ERROR: Invalid partition index: " <<
          // partition
          //          << " (size: " << solution.tech_nodes.size() << ")" <<
          //          std::endl;
        }
        valid_indices = false;
      }

      if (valid_indices &&
          (new_tech_idx < 0 || new_tech_idx >= available_tech_nodes_.size())) {
#pragma omp critical(console_output)
        {
          // std::cout << "[DEBUG] ERROR: Invalid tech index: " << new_tech_idx
          //          << " (size: " << available_tech_nodes_.size() << ")" <<
          //          std::endl;
        }
        valid_indices = false;
      }

      if (valid_indices) {
#pragma omp critical(console_output)
        {
          std::string old_tech = solution.tech_nodes[partition];
          // std::cout << "[DEBUG] Changing tech for partition " << partition <<
          // " from '"
          //          << old_tech << "' to '" <<
          //          available_tech_nodes_[new_tech_idx] << "'" << std::endl;
        }

// This modifies the solution, so it needs a critical section
#pragma omp critical(solution_modify)
        {
          solution.tech_nodes[partition] = available_tech_nodes_[new_tech_idx];
        }
      }
    }
  } else {
// Method 3: Structure mutation - split or merge partitions
#pragma omp critical(console_output)
    {
      // std::cout << "[DEBUG] Structure mutation" << std::endl;
    }

    // Thread-safe RNG access for determining merge or split
    bool should_merge;
#pragma omp critical(rng_access)
    {
      should_merge =
          solution.num_partitions > min_partitions_ &&
          std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) < 0.5f;
    }

    if (should_merge) {
      // Safety check before proceeding
      if (solution.num_partitions < 2) {
#pragma omp critical(console_output)
        { Console::Warning("Cannot merge partitions: too few partitions"); }
        return; // Can't merge with less than 2 partitions
      }

      // Thread-safe RNG access for partition selection
      int partition1, partition2;
#pragma omp critical(rng_access)
      {
        // Merge two partitions
        partition1 = std::uniform_int_distribution<int>(
            0, solution.num_partitions - 1)(rng_);

        do {
          partition2 = std::uniform_int_distribution<int>(
              0, solution.num_partitions - 1)(rng_);
        } while (partition2 == partition1);
      }

#pragma omp critical(console_output)
      {
        // std::cout << "[DEBUG] Merging partitions " << partition1 << " and "
        // << partition2 << std::endl;
      }

      // Validate partition indices
      if (partition1 < 0 || partition1 >= solution.tech_nodes.size() ||
          partition2 < 0 || partition2 >= solution.tech_nodes.size()) {
#pragma omp critical(console_output)
        {
          // std::cout << "[DEBUG] ERROR: Invalid partition indices for merge: "
          // << partition1
          //          << ", " << partition2 << " (size: " <<
          //          solution.tech_nodes.size() << ")" << std::endl;
        }
        return;
      }

      // Ensure partition vector is valid
      if (solution.partition.empty()) {
#pragma omp critical(console_output)
        { Console::Error("Empty partition vector"); }
        return;
      }

// This entire operation modifies the solution, so it needs a critical section
#pragma omp critical(solution_modify)
      {
        // Reassign vertices from partition2 to partition1
        for (int i = 0; i < hypergraph_->GetNumVertices(); ++i) {
          if (i >= solution.partition.size()) {
#pragma omp critical(console_output)
            {
              Console::Error("Vertex index out of bounds: " +
                             std::to_string(i));
            }
          } else if (solution.partition[i] == partition2) {
            solution.partition[i] = partition1;
          } else if (solution.partition[i] > partition2) {
            // Shift down partitions above partition2
            solution.partition[i]--;
          }
        }

#pragma omp critical(console_output)
        {
          // Update tech assignments - remove partition2
          // std::cout << "[DEBUG] Removing tech node at index " << partition2
          //          << " (value: '" << solution.tech_nodes[partition2] << "')"
          //          << std::endl;
        }

        // Safe removal
        if (partition2 >= 0 && partition2 < solution.tech_nodes.size()) {
          solution.tech_nodes.erase(solution.tech_nodes.begin() + partition2);
        } else {
#pragma omp critical(console_output)
          { Console::Error("Cannot remove tech node: index out of bounds"); }
        }

        // Update partition count
        solution.num_partitions--;

        // Ensure tech_nodes size equals num_partitions
        if (solution.tech_nodes.size() != solution.num_partitions) {
#pragma omp critical(console_output)
          {
            Console::Warning("After merge: tech_nodes size (" +
                             std::to_string(solution.tech_nodes.size()) +
                             ") doesn't match num_partitions (" +
                             std::to_string(solution.num_partitions) +
                             "), adjusting");
          }
          solution.tech_nodes.resize(solution.num_partitions);
        }
      }

#pragma omp critical(console_output)
      {
        // std::cout << "[DEBUG] After merge: " << solution.num_partitions << "
        // partitions" << std::endl;
      }
    } else if (solution.num_partitions < max_partitions_) {
      // Split a partition
      if (solution.num_partitions <= 0) {
#pragma omp critical(console_output)
        {
          Console::Error("Invalid number of partitions for split: " +
                         std::to_string(solution.num_partitions));
        }
        return;
      }

      // Thread-safe RNG access for partition selection
      int partition_to_split;
#pragma omp critical(rng_access)
      {
        partition_to_split = std::uniform_int_distribution<int>(
            0, solution.num_partitions - 1)(rng_);
      }

#pragma omp critical(console_output)
      {
        // std::cout << "[DEBUG] Attempting to split partition " <<
        // partition_to_split << std::endl;
      }

      // Validate partition index
      if (partition_to_split < 0 ||
          partition_to_split >= solution.tech_nodes.size()) {
#pragma omp critical(console_output)
        {
          // std::cout << "[DEBUG] ERROR: Invalid partition_to_split: " <<
          // partition_to_split
          //          << " (size: " << solution.tech_nodes.size() << ")" <<
          //          std::endl;
        }
        return;
      }

      // Count vertices in the partition
      int count = 0;
      for (int i = 0; i < hypergraph_->GetNumVertices(); ++i) {
        if (i < solution.partition.size() &&
            solution.partition[i] == partition_to_split) {
          count++;
        }
      }

#pragma omp critical(console_output)
      {
        // std::cout << "[DEBUG] Partition " << partition_to_split << " has " <<
        // count << " vertices" << std::endl;
      }

      // Only split if the partition has enough vertices
      if (count > 1) {
// This entire operation modifies the solution, so it needs a critical section
#pragma omp critical(solution_modify)
        {
          // Create a new partition ID
          int new_partition = solution.num_partitions;

          // Reassign about half the vertices to the new partition
          int to_reassign = count / 2;

#pragma omp critical(console_output)
          {
            // std::cout << "[DEBUG] Will reassign " << to_reassign << "
            // vertices to new partition " << new_partition << std::endl;
          }

          // Thread-safe RNG for reassignment decisions
          std::vector<float> random_values(hypergraph_->GetNumVertices());
#pragma omp critical(rng_access)
          {
            for (int i = 0; i < hypergraph_->GetNumVertices(); ++i) {
              random_values[i] =
                  std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_);
            }
          }

          for (int i = 0; i < hypergraph_->GetNumVertices() && to_reassign > 0;
               ++i) {
            if (i < solution.partition.size() &&
                solution.partition[i] == partition_to_split) {
              if (random_values[i] < 0.5f) {
                solution.partition[i] = new_partition;
                to_reassign--;
              }
            }
          }

#pragma omp critical(console_output)
          {
            // Add a new tech assignment
            // std::cout << "[DEBUG] Adding new tech assignment for partition "
            // << new_partition << std::endl;
          }

          std::string new_tech;
          float inheritance_decision;

// Thread-safe RNG for tech assignment
#pragma omp critical(rng_access)
          {
            inheritance_decision =
                std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_);
          }

          if (inheritance_decision < 0.7f) {
            // 70% chance to inherit tech from parent partition
            std::string inherited_tech;
            if (partition_to_split >= 0 &&
                partition_to_split < solution.tech_nodes.size()) {
              inherited_tech = solution.tech_nodes[partition_to_split];
            }

#pragma omp critical(console_output)
            {
              // std::cout << "[DEBUG] Inheriting tech '" << inherited_tech <<
              // "' from partition " << partition_to_split << std::endl;
            }

            if (inherited_tech.empty()) {
#pragma omp critical(console_output)
              {
                // std::cout << "[DEBUG] WARNING: Inherited tech is empty, using
                // a default" << std::endl;
              }

              if (!available_tech_nodes_.empty()) {
                inherited_tech =
                    available_tech_nodes_[0]; // Use first available as default
              } else {
                inherited_tech = "7nm"; // Hard default
              }
            }

            new_tech = inherited_tech;
          } else {
            // 30% chance to use a random tech
            if (available_tech_nodes_.empty()) {
#pragma omp critical(console_output)
              {
                // std::cout << "[DEBUG] ERROR: available_tech_nodes_ is empty"
                // << std::endl;
              }
              new_tech = "7nm"; // Hard default
            } else {
              int tech_idx;

// Thread-safe RNG for tech index
#pragma omp critical(rng_access)
              {
                tech_idx = std::uniform_int_distribution<int>(
                    0, available_tech_nodes_.size() - 1)(rng_);
              }

              if (tech_idx < 0 || tech_idx >= available_tech_nodes_.size()) {
#pragma omp critical(console_output)
                {
                  // std::cout << "[DEBUG] ERROR: Invalid tech index: " <<
                  // tech_idx
                  //          << " (size: " << available_tech_nodes_.size() <<
                  //          ")" << std::endl;
                }
                new_tech = "7nm"; // Hard default
              } else {
                new_tech = available_tech_nodes_[tech_idx];
              }
            }

#pragma omp critical(console_output)
            {
              // std::cout << "[DEBUG] Using random tech '" << new_tech << "'"
              // << std::endl;
            }

            if (new_tech.empty()) {
#pragma omp critical(console_output)
              {
                // std::cout << "[DEBUG] WARNING: Selected tech is empty, using
                // a default" << std::endl;
              }
              new_tech = "7nm"; // Hard-coded default as last resort
            }
          }

          solution.tech_nodes.push_back(new_tech);

          // Update partition count
          solution.num_partitions++;

          // Ensure tech_nodes size equals num_partitions
          if (solution.tech_nodes.size() != solution.num_partitions) {
#pragma omp critical(console_output)
            {
              Console::Warning("After split: tech_nodes size (" +
                               std::to_string(solution.tech_nodes.size()) +
                               ") doesn't match num_partitions (" +
                               std::to_string(solution.num_partitions) +
                               "), adjusting");
            }
            solution.tech_nodes.resize(solution.num_partitions);

            // If we truncated and removed the tech we just added, add it back
            if (solution.tech_nodes.size() == solution.num_partitions - 1) {
              solution.tech_nodes.push_back(new_tech);
            }
          }
        }

#pragma omp critical(console_output)
        {
          // std::cout << "[DEBUG] After split: " << solution.num_partitions <<
          // " partitions" << std::endl;
        }
      }
    }
  }

// Make a final check to ensure tech_nodes size matches num_partitions
#pragma omp critical(solution_modify)
  {
    if (solution.tech_nodes.size() != solution.num_partitions) {
#pragma omp critical(console_output)
      {
        Console::Warning("After mutation: tech_nodes size (" +
                         std::to_string(solution.tech_nodes.size()) +
                         ") doesn't match num_partitions (" +
                         std::to_string(solution.num_partitions) +
                         "), adjusting");
      }

      // Fix the tech_nodes size
      if (solution.tech_nodes.size() > solution.num_partitions) {
        // Trim excess tech nodes
        solution.tech_nodes.resize(solution.num_partitions);
      } else {
        // Add missing tech nodes
        std::vector<std::string> old_tech = solution.tech_nodes;
        solution.tech_nodes.resize(solution.num_partitions);

        // Fill in missing tech nodes
        for (int i = old_tech.size(); i < solution.num_partitions; ++i) {
// Must use thread-safe RNG access
#pragma omp critical(rng_access)
          {
            if (!available_tech_nodes_.empty()) {
              int tech_idx = std::uniform_int_distribution<int>(
                  0, available_tech_nodes_.size() - 1)(rng_);

              if (tech_idx >= 0 && tech_idx < available_tech_nodes_.size()) {
                solution.tech_nodes[i] = available_tech_nodes_[tech_idx];
              } else {
                solution.tech_nodes[i] = "7nm"; // Default fallback
              }
            } else {
              solution.tech_nodes[i] = "7nm"; // Default fallback
            }
          }
        }
      }
    }
  }

// Dump tech nodes after mutation for debugging
#pragma omp critical(console_output)
  {
    // std::cout << "[DEBUG] Tech nodes after mutation: ";
    // for (size_t i = 0; i < solution.tech_nodes.size(); ++i) {
    //  std::cout << "'" << solution.tech_nodes[i] << "' ";
    //}
    // std::cout << std::endl;
  }

// Reset cost to force re-evaluation
#pragma omp critical(solution_modify)
  { solution.cost = std::numeric_limits<float>::max(); }

  // Final validation
  ValidateSolution(solution, true);
}

// Create initial partitions using various methods
std::vector<std::vector<int>>
GeneticTechPartitioner::CreateInitialPartitions(int min_parts, int max_parts) {
  std::vector<std::vector<int>> partitions;

  // Create random partitions for different numbers of parts
  std::uniform_int_distribution<int> part_dist(0, max_parts - 1);

  // Add random partitions with different numbers of parts
  for (int num_parts = min_parts; num_parts <= max_parts; ++num_parts) {
    // Create 3 different random partitions for each number of parts
    for (int i = 0; i < 3; ++i) {
      std::vector<int> random_partition(hypergraph_->GetNumVertices());
      for (int j = 0; j < hypergraph_->GetNumVertices(); ++j) {
        random_partition[j] =
            std::uniform_int_distribution<int>(0, num_parts - 1)(rng_);
      }
      partitions.push_back(random_partition);

      // Also create a partitioning that tries to balance the number of vertices
      // in each part
      if (i == 0) {
        std::vector<int> balanced_partition(hypergraph_->GetNumVertices());
        for (int j = 0; j < hypergraph_->GetNumVertices(); ++j) {
          balanced_partition[j] = j % num_parts;
        }
        partitions.push_back(balanced_partition);
      }
    }
  }
  
  // Use METIS and spectral partitioning if chiplet_part_ is available
  if (chiplet_part_ != nullptr) {
    Console::Info("Using advanced partitioning methods (METIS and Spectral) for initial population");
    
    // Add METIS-based partitions
    for (int num_parts = min_parts; num_parts <= max_parts; ++num_parts) {
      int metis_parts = num_parts; // Make a copy because METISPart modifies the value
      std::vector<int> metis_partition = chiplet_part_->METISPart(metis_parts);
      
      if (!metis_partition.empty()) {
        Console::Info("Added METIS-based partition with " + std::to_string(metis_parts) + " parts");
        partitions.push_back(metis_partition);
      }
    }
    
    // Add spectral partitioning
    std::vector<int> spectral_partition = chiplet_part_->SpectralPartition();
    if (!spectral_partition.empty() && 
        std::find(spectral_partition.begin(), spectral_partition.end(), -1) == spectral_partition.end()) {
      Console::Info("Added spectral-based partition");
      partitions.push_back(spectral_partition);
    }
  } else {
    Console::Warning("ChipletPart reference not set, advanced partitioning methods not available");
  }

  Console::Info("Created " + std::to_string(partitions.size()) +
                " initial partitions with varying numbers of partitions.");
  return partitions;
}

// Generate a random technology assignment
std::vector<std::string>
GeneticTechPartitioner::GenerateRandomTechAssignment(int num_partitions) {
  // std::cout << "[DEBUG-GEN] Generating random tech assignment for " <<
  // num_partitions
  //          << " partitions from " << available_tech_nodes_.size() << "
  //          available techs" << std::endl;

  if (available_tech_nodes_.empty()) {
    // std::cout << "[DEBUG-GEN] ERROR: available_tech_nodes_ is empty!" <<
    // std::endl;
    // Return a vector of default techs as fallback
    return std::vector<std::string>(num_partitions, "7nm");
  }

  std::vector<std::string> assignment;
  assignment.reserve(num_partitions);

  for (int i = 0; i < num_partitions; ++i) {
    int tech_idx = std::uniform_int_distribution<int>(
        0, available_tech_nodes_.size() - 1)(rng_);

    std::string selected_tech = available_tech_nodes_[tech_idx];

    // std::cout << "[DEBUG-GEN] Selected tech '" << selected_tech << "' for
    // partition " << i << std::endl;

    if (selected_tech.empty()) {
      // std::cout << "[DEBUG-GEN] WARNING: Empty tech selected, using default"
      // << std::endl;
      selected_tech = "7nm"; // Default fallback
    }

    assignment.push_back(selected_tech);
  }

  return assignment;
}

// Improve RepairSolution function for thread safety
void GeneticTechPartitioner::RepairSolution(GeneticSolution &solution) {
#pragma omp critical(console_output)
  {
    Console::Info("Repairing solution with " +
                  std::to_string(solution.num_partitions) + " partitions and " +
                  std::to_string(solution.tech_nodes.size()) + " tech nodes");
  }

  // Ensure partition vector is valid
  if (solution.partition.empty()) {
#pragma omp critical(console_output)
    { Console::Error("Empty partition vector!"); }
    // Create default partition with all vertices in partition 0
    solution.partition.resize(hypergraph_->GetNumVertices(), 0);
    solution.num_partitions = 1;
  }

  // Ensure partition IDs are consecutive starting from 0
  std::map<int, int> id_map;
  std::vector<std::string> old_tech = solution.tech_nodes;
  int next_id = 0;

  // First pass: collect unique partition IDs and create mapping
  for (int pid : solution.partition) {
    if (pid < 0) {
#pragma omp critical(console_output)
      {
        Console::Warning("Negative partition ID " + std::to_string(pid) +
                         " found, will be remapped");
      }
      // Will be corrected in second pass
    } else if (id_map.find(pid) == id_map.end()) {
      id_map[pid] = next_id++;
    }
  }

  // Handle case where all partitions are negative (invalid)
  if (next_id == 0) {
#pragma omp critical(console_output)
    {
      Console::Warning(
          "All partition IDs are invalid, resetting to single partition");
    }
    for (int &pid : solution.partition) {
      pid = 0;
    }
    next_id = 1;
  }

  // Create a new tech_nodes array based on the remapping
  std::vector<std::string> new_tech_nodes(next_id);

  // Initialize with default tech nodes
  for (int i = 0; i < next_id; i++) {
    if (!available_tech_nodes_.empty()) {
#pragma omp critical(rng_access)
      {
        int tech_idx = std::uniform_int_distribution<int>(
            0, available_tech_nodes_.size() - 1)(rng_);
        new_tech_nodes[i] = available_tech_nodes_[tech_idx];
      }
    } else {
      new_tech_nodes[i] = "7nm"; // Default fallback
    }
  }

  // Copy over tech nodes from the old array to the new one using the id map
  for (const auto &[old_id, new_id] : id_map) {
    if (old_id >= 0 && old_id < solution.tech_nodes.size() &&
        !solution.tech_nodes[old_id].empty()) {
      new_tech_nodes[new_id] = solution.tech_nodes[old_id];
    }
  }

  // Second pass: remap partition IDs
  for (int &pid : solution.partition) {
    if (pid < 0 || id_map.find(pid) == id_map.end()) {
      // Invalid ID, assign to partition 0
      pid = 0;
    } else {
      // Remap to consecutive ID
      pid = id_map[pid];
    }
  }

  // Update number of partitions and tech_nodes
  solution.num_partitions = next_id;
  solution.tech_nodes = new_tech_nodes;

#pragma omp critical(console_output)
  {
    Console::Info("Repaired solution now has " +
                  std::to_string(solution.num_partitions) + " partitions and " +
                  std::to_string(solution.tech_nodes.size()) + " tech nodes");
  }

  // Ensure num_partitions is within bounds
  if (solution.num_partitions < min_partitions_) {
#pragma omp critical(console_output)
    {
      Console::Warning(
          "num_partitions " + std::to_string(solution.num_partitions) +
          " below minimum " + std::to_string(min_partitions_) + ", adjusting");
    }

    // Distribute vertices to additional partitions to meet minimum
    while (solution.num_partitions < min_partitions_) {
      int new_partition_id = solution.num_partitions;

      // Assign some vertices from partition 0 to the new partition
      int count = 0;
      for (int i = 0; i < solution.partition.size() && count < 5; ++i) {
        if (solution.partition[i] == 0) {
          solution.partition[i] = new_partition_id;
          count++;
        }
      }

      // Add a new tech node for the new partition
      if (!available_tech_nodes_.empty()) {
#pragma omp critical(rng_access)
        {
          int tech_idx = std::uniform_int_distribution<int>(
              0, available_tech_nodes_.size() - 1)(rng_);
          solution.tech_nodes.push_back(available_tech_nodes_[tech_idx]);
        }
      } else {
        solution.tech_nodes.push_back("7nm"); // Default fallback
      }

      solution.num_partitions++;
    }
  }

  if (solution.num_partitions > max_partitions_) {
#pragma omp critical(console_output)
    {
      Console::Warning("num_partitions " +
                       std::to_string(solution.num_partitions) +
                       " exceeds maximum " + std::to_string(max_partitions_) +
                       ", adjusting");
    }

    // Create a mapping for partitions to collapse
    std::map<int, int> collapse_map;
    for (int i = 0; i < solution.num_partitions; i++) {
      if (i < max_partitions_) {
        collapse_map[i] = i; // Keep these partitions as is
      } else {
        // Assign excess partitions to partition 0 (or distribute them if
        // preferred)
        collapse_map[i] = 0;
      }
    }

    // Apply the collapse mapping to the partition vector
    for (int &pid : solution.partition) {
      pid = collapse_map[pid];
    }

    // Resize tech_nodes to match the new number of partitions
    solution.tech_nodes.resize(max_partitions_);
    solution.num_partitions = max_partitions_;
  }

  // Final check to ensure tech_nodes size matches num_partitions
  if (solution.tech_nodes.size() != solution.num_partitions) {
#pragma omp critical(console_output)
    {
      Console::Warning("Final tech_nodes size (" +
                       std::to_string(solution.tech_nodes.size()) +
                       ") doesn't match num_partitions (" +
                       std::to_string(solution.num_partitions) + "), fixing");
    }

    solution.tech_nodes.resize(solution.num_partitions);

    // Fill any missing tech nodes
    for (int i = 0; i < solution.num_partitions; i++) {
      if (i >= solution.tech_nodes.size() || solution.tech_nodes[i].empty()) {
        if (!available_tech_nodes_.empty()) {
#pragma omp critical(rng_access)
          {
            int tech_idx = std::uniform_int_distribution<int>(
                0, available_tech_nodes_.size() - 1)(rng_);

            if (i < solution.tech_nodes.size()) {
              solution.tech_nodes[i] = available_tech_nodes_[tech_idx];
            } else {
              solution.tech_nodes.push_back(available_tech_nodes_[tech_idx]);
            }
          }
        } else {
          if (i < solution.tech_nodes.size()) {
            solution.tech_nodes[i] = "7nm"; // Default fallback
          } else {
            solution.tech_nodes.push_back("7nm");
          }
        }
      }
    }
  }

  // Validate all tech nodes are non-empty
  for (int i = 0; i < solution.tech_nodes.size(); ++i) {
    if (solution.tech_nodes[i].empty()) {
#pragma omp critical(console_output)
      {
        Console::Warning("Empty tech node at index " + std::to_string(i) +
                         ", assigning default");
      }
      solution.tech_nodes[i] = "7nm"; // Default tech node as fallback
    }
  }

  // Reset cost to force re-evaluation
  solution.cost = std::numeric_limits<float>::max();

#pragma omp critical(console_output)
  {
    std::stringstream tech_debug;
    tech_debug << "Final tech nodes: ";
    for (size_t i = 0; i < solution.tech_nodes.size(); ++i) {
      tech_debug << "'" << solution.tech_nodes[i] << "' ";
    }
    Console::Info(tech_debug.str());
  }
}

// Print generation statistics
void GeneticTechPartitioner::PrintGenerationStats(int generation,
                                                  float best_cost,
                                                  float avg_cost) {

  std::vector<std::string> columns = {"Statistic", "Value"};
  std::vector<int> widths = {30, 20};
  Console::TableHeader(columns, widths);

  Console::TableRow({"Generation", std::to_string(generation + 1)}, widths);
  Console::TableRow({"Best Cost This Generation", std::to_string(best_cost)},
                    widths);
  Console::TableRow({"Average Cost", std::to_string(avg_cost)}, widths);
  Console::TableRow({"Best Overall Cost", std::to_string(best_solution_.cost)},
                    widths);
  Console::TableRow({"Best Solution Partitions",
                     std::to_string(best_solution_.num_partitions)},
                    widths);
  std::cout << std::endl;
}

// Save results to files
void GeneticTechPartitioner::SaveResults(const GeneticSolution &solution,
                                         const std::string &prefix) {

  // Save partition assignments
  std::string partition_file =
      prefix + ".parts." + std::to_string(solution.num_partitions);
  std::ofstream part_out(partition_file);

  if (part_out.is_open()) {
    for (int pid : solution.partition) {
      part_out << pid << std::endl;
    }
    part_out.close();
    Console::Success("Saved partition assignments to " + partition_file);
  } else {
    Console::Error("Failed to save partition assignments to " + partition_file);
  }

  // Save technology assignments
  std::string tech_file =
      prefix + ".techs." + std::to_string(solution.num_partitions);
  std::ofstream tech_out(tech_file);

  if (tech_out.is_open()) {
    for (const auto &tech : solution.tech_nodes) {
      tech_out << tech << std::endl;
    }
    tech_out.close();
    Console::Success("Saved technology assignments to " + tech_file);
  } else {
    Console::Error("Failed to save technology assignments to " + tech_file);
  }

  // Save summary information
  std::string summary_file = prefix + ".summary.txt";
  std::ofstream summary_out(summary_file);

  if (summary_out.is_open()) {
    summary_out << "Number of Partitions: " << solution.num_partitions
                << std::endl;
    summary_out << "Cost: " << solution.cost << std::endl;
    summary_out << "Valid: " << (solution.valid ? "Yes" : "No") << std::endl;
    summary_out << std::endl;

    summary_out << "Technology Assignment:" << std::endl;
    for (int i = 0; i < solution.tech_nodes.size(); ++i) {
      summary_out << "Partition " << i << ": " << solution.tech_nodes[i]
                  << std::endl;
    }

    summary_out << std::endl;
    summary_out << "Partition Statistics:" << std::endl;

    // Count vertices in each partition
    std::vector<int> part_sizes(solution.num_partitions, 0);
    for (int pid : solution.partition) {
      part_sizes[pid]++;
    }

    for (int i = 0; i < solution.num_partitions; ++i) {
      float percentage = 100.0f * part_sizes[i] / solution.partition.size();
      summary_out << "Partition " << i << ": " << part_sizes[i] << " vertices ("
                  << std::fixed << std::setprecision(2) << percentage << "%)"
                  << std::endl;
    }

    summary_out.close();
    Console::Success("Saved summary information to " + summary_file);
  } else {
    Console::Error("Failed to save summary information to " + summary_file);
  }
}

} // namespace chiplet