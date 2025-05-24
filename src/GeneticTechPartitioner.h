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

#pragma once

#include "ChipletPart.h"
#include "FMRefiner.h"
#include "Hypergraph.h"
#include "omp_utils.h"

#include <vector>
#include <string>
#include <random>
#include <memory>
#include <map>
#include <set>
#include <tuple>
#include <mutex>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>
#include <atomic>
#include <iomanip>

namespace chiplet {

/**
 * @brief Structure to represent a solution in the genetic algorithm
 */
struct GeneticSolution {
    int num_partitions;                   // Number of partitions
    std::vector<int> partition;           // Vertex→partition assignments
    std::vector<std::string> tech_nodes;  // Partition→technology assignments
    float cost;                           // Cached cost value
    bool valid;                           // Is the solution valid?

    GeneticSolution() : num_partitions(0), cost(std::numeric_limits<float>::max()), valid(false) {}
    
    GeneticSolution(int n, const std::vector<int>& p, const std::vector<std::string>& t, float c, bool v) 
        : num_partitions(n), partition(p), tech_nodes(t), cost(c), valid(v) {}
    
    // Copy constructor
    GeneticSolution(const GeneticSolution& other) = default;
    
    // Move constructor
    GeneticSolution(GeneticSolution&& other) = default;
    
    // Copy assignment operator
    GeneticSolution& operator=(const GeneticSolution& other) = default;
    
    // Move assignment operator
    GeneticSolution& operator=(GeneticSolution&& other) = default;
};

/**
 * @brief Class for genetic algorithm that co-optimizes partitioning and technology assignment
 */
class GeneticTechPartitioner {
public:
    /**
     * @brief Constructor
     * @param hypergraph The hypergraph to partition
     * @param ub_factor imbalance factor for balance constraints
     * @param available_tech_nodes The available technology nodes
     * @param seed Random seed for reproducibility
     * @param num_generations Number of generations to run
     * @param population_size Population size for the genetic algorithm
     * @param mutation_rate Mutation rate
     * @param crossover_rate Crossover rate
     * @param min_partitions Minimum number of partitions to consider
     * @param max_partitions Maximum number of partitions to consider
     */
    GeneticTechPartitioner(
        std::shared_ptr<Hypergraph> hypergraph,
        const std::vector<std::string>& available_tech_nodes,
        float ub_factor,
        unsigned int seed = 42,
        int num_generations = 50,
        int population_size = 50,
        float mutation_rate = 0.2,
        float crossover_rate = 0.7,
        int min_partitions = 2,
        int max_partitions = 8
    );

    /**
     * @brief Run the genetic algorithm for co-optimization
     * @param chiplet_io_file IO file path
     * @param chiplet_layer_file Layer file path
     * @param chiplet_wafer_process_file Wafer process file path
     * @param chiplet_assembly_process_file Assembly process file path
     * @param chiplet_test_file Test file path
     * @param chiplet_netlist_file Netlist file path
     * @param chiplet_blocks_file Blocks file path
     * @param reach Reach parameter
     * @param separation Separation parameter
     * @return Best solution found
     */
    GeneticSolution Run(
        const std::string& chiplet_io_file,
        const std::string& chiplet_layer_file,
        const std::string& chiplet_wafer_process_file,
        const std::string& chiplet_assembly_process_file,
        const std::string& chiplet_test_file,
        const std::string& chiplet_netlist_file,
        const std::string& chiplet_blocks_file,
        float reach,
        float separation
    );

    /**
     * @brief Save the results to files
     * @param solution The solution to save
     * @param prefix Optional file prefix
     */
    void SaveResults(const GeneticSolution& solution, const std::string& prefix = "genetic_tech_part");

    // Set the ChipletPart pointer for advanced partitioning methods
    void SetChipletPart(ChipletPart* chiplet_part) {
        chiplet_part_ = chiplet_part;
    }

private:
    // Initialize the population with diverse solutions
    void InitializePopulation(
        const std::string& chiplet_io_file,
        const std::string& chiplet_layer_file,
        const std::string& chiplet_wafer_process_file,
        const std::string& chiplet_assembly_process_file,
        const std::string& chiplet_test_file,
        const std::string& chiplet_netlist_file,
        const std::string& chiplet_blocks_file,
        float reach,
        float separation
    );

    // Evaluate fitness for a solution
    float EvaluateFitness(GeneticSolution& solution);

    // Select parents for reproduction using tournament selection
    std::vector<int> SelectParents();

    // Perform crossover between two parents
    GeneticSolution Crossover(const GeneticSolution& parent1, const GeneticSolution& parent2);

    // Perform mutation on an individual
    void Mutate(GeneticSolution& solution);

    // Create initial partitions using various methods
    std::vector<std::vector<int>> CreateInitialPartitions(int min_parts, int max_parts);

    // Generate a random technology assignment
    std::vector<std::string> GenerateRandomTechAssignment(int num_partitions);

    // Repair a solution to maintain consistency between partition and technology assignments
    void RepairSolution(GeneticSolution& solution);

    // Helper function to validate a solution and ensure consistency
    bool ValidateSolution(GeneticSolution& solution, bool repair_if_invalid = false);

    // Print generation statistics
    void PrintGenerationStats(int generation, float best_cost, float avg_cost);

    // Cache for cost evaluations to avoid redundant calculations
    std::map<std::string, float> cost_cache_;

    // Member variables
    std::shared_ptr<Hypergraph> hypergraph_;
    std::vector<std::string> available_tech_nodes_;
    std::mt19937 rng_;
    ChipletPart* chiplet_part_; // Pointer to ChipletPart for advanced partitioning methods

    Matrix<float> upper_block_balance_;
    Matrix<float> lower_block_balance_;
    
    // Control parameters
    float ub_factor_;
    int num_generations_;
    int population_size_;
    float mutation_rate_;
    float crossover_rate_;
    int min_partitions_;
    int max_partitions_;
    
    // Runtime data
    std::vector<GeneticSolution> population_;
    GeneticSolution best_solution_;
    std::shared_ptr<ChipletRefiner> refiner_;
    int tournament_size_ = 3; // Default tournament size
    
    // Mutex for thread safety
    mutable std::mutex cache_mutex_;
};

} // namespace chiplet 