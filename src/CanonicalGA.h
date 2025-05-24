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
#include "Console.h"
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
#include <functional>
#include <unordered_map>

namespace chiplet {

/**
 * @brief Structure to represent a solution in the canonical genetic algorithm
 */
struct CanonicalSolution {
    std::vector<std::string> tech_nodes;     // Technology assignments for chiplets
    std::vector<int> partition;              // Resulting partition assignments
    float cost;                              // Fitness/cost value
    
    CanonicalSolution() : cost(std::numeric_limits<float>::max()) {}
    
    CanonicalSolution(const std::vector<std::string>& t, const std::vector<int>& p, float c) 
        : partition(p), cost(c) {
        // Make safe copies of the strings
        try {
            tech_nodes.reserve(t.size());
            
            for (size_t i = 0; i < t.size(); i++) {
                try {
                    const std::string& tech = t[i];
                    // Use a different approach to avoid c_str()
                    std::string tech_copy;
                    tech_copy.assign(tech.begin(), tech.end());
                    tech_nodes.push_back(tech_copy);
                } catch (const std::exception& e) {
                    // Try a simpler approach as fallback
                    tech_nodes.push_back("default");
                }
            }
        } catch (const std::exception& e) {
            // Handle exceptions silently
        }
    }
    
    // Copy constructor with safe string handling
    CanonicalSolution(const CanonicalSolution& other) : partition(other.partition), cost(other.cost) {
        // Make safe copies of the strings
        try {
            tech_nodes.reserve(other.tech_nodes.size());
            for (const auto& tech : other.tech_nodes) {
                // Use a different approach to avoid c_str()
                std::string tech_copy;
                tech_copy.assign(tech.begin(), tech.end());
                tech_nodes.push_back(tech_copy);
            }
        } catch (const std::exception& e) {
            // Handle exceptions silently
        }
    }
    
    // Move constructor
    CanonicalSolution(CanonicalSolution&& other) noexcept 
        : tech_nodes(std::move(other.tech_nodes)), 
          partition(std::move(other.partition)), 
          cost(other.cost) {}
    
    // Copy assignment operator with safe string handling
    CanonicalSolution& operator=(const CanonicalSolution& other) {
        if (this != &other) {
            partition = other.partition;
            cost = other.cost;
            
            // Make safe copies of the strings
            try {
                tech_nodes.clear();
                tech_nodes.reserve(other.tech_nodes.size());
                for (const auto& tech : other.tech_nodes) {
                    // Use a different approach to avoid c_str()
                    std::string tech_copy;
                    tech_copy.assign(tech.begin(), tech.end());
                    tech_nodes.push_back(tech_copy);
                }
            } catch (const std::exception& e) {
                // Handle exceptions silently
            }
        }
        return *this;
    }
    
    // Move assignment operator
    CanonicalSolution& operator=(CanonicalSolution&& other) noexcept {
        if (this != &other) {
            tech_nodes = std::move(other.tech_nodes);
            partition = std::move(other.partition);
            cost = other.cost;
        }
        return *this;
    }
};

/**
 * @brief Class for canonical genetic algorithm that optimizes technology assignment
 */
class CanonicalGA {
public:
    /**
     * @brief Constructor
     * @param available_tech_nodes The available technology nodes
     * @param seed Random seed for reproducibility
     * @param num_generations Number of generations to run
     * @param population_size Population size for the genetic algorithm
     * @param mutation_rate Mutation rate
     * @param crossover_rate Crossover rate
     * @param min_partitions Minimum number of chiplets to consider
     * @param max_partitions Maximum number of chiplets to consider
     */
    CanonicalGA(
        const std::vector<std::string>& available_tech_nodes,
        unsigned int seed = 42,
        int num_generations = 50,
        int population_size = 50,
        float mutation_rate = 0.2,
        float crossover_rate = 0.7,
        int min_partitions = 2,
        int max_partitions = 8
    );
    
    /**
     * @brief Run the genetic algorithm
     * @param chiplet_part Pointer to ChipletPart for evaluation
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
    CanonicalSolution Run(
        ChipletPart* chiplet_part,
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
    void SaveResults(const CanonicalSolution& solution, const std::string& prefix = "canonical_ga");

    // Helper function to get the ID of a tech node
    int GetTechNodeId(const std::string& tech_node);

private:
    // Canonicalize technology assignment to avoid duplicate evaluations
    std::vector<std::string> CanonicalizeAssignment(const std::vector<std::string>& tech_nodes);
    
    // Generate a hash string for caching
    std::string ComputeAssignmentHash(const std::vector<std::string>& tech_nodes);
    
    // Initialize population with diverse tech assignments
    void InitializePopulation();
    
    // Evaluate fitness for a solution
    float EvaluateFitness(
        std::vector<std::string>& tech_assignment,
        std::vector<int>& resulting_partition,
        ChipletPart* chiplet_part,
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
    
    // Select parents using tournament selection
    std::vector<int> SelectParents();
    
    // Perform crossover between two parents
    std::vector<std::string> Crossover(const std::vector<std::string>& parent1, 
                                       const std::vector<std::string>& parent2);
    
    // Perform mutation on an individual
    void Mutate(std::vector<std::string>& tech_assignment);
    
    // Ensure a solution is unique in the population
    bool IsUnique(const std::vector<std::string>& tech_assignment, 
                 const std::vector<std::vector<std::string>>& population);
    
    // Generate initial random tech assignments
    std::vector<std::string> GenerateRandomAssignment(int num_chiplets);
    
    // Print generation statistics
    void PrintGenerationStats(int generation, float best_cost, float avg_cost);
    
    // Member variables
    std::vector<std::string> available_tech_nodes_;
    std::mt19937 rng_;
    
    // Control parameters
    int num_generations_;
    int population_size_;
    float mutation_rate_;
    float crossover_rate_;
    int min_partitions_;
    int max_partitions_;
    int tournament_size_ = 3;
    
    // Runtime data
    std::vector<std::vector<std::string>> population_;
    std::vector<std::vector<int>> partitions_;
    std::vector<float> fitness_values_;
    CanonicalSolution best_solution_;
    
    // Cache for fitness evaluations to avoid redundant calculations
    std::unordered_map<std::string, std::pair<float, std::vector<int>>> fitness_cache_;
    
    // Mutex for thread safety
    mutable std::mutex cache_mutex_;
};

} // namespace chiplet 