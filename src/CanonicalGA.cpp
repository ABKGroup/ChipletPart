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

#include "CanonicalGA.h"
#include <thread>

namespace chiplet {

// Constructor
CanonicalGA::CanonicalGA(
    const std::vector<std::string>& available_tech_nodes,
    unsigned int seed,
    int num_generations,
    int population_size,
    float mutation_rate,
    float crossover_rate,
    int min_partitions,
    int max_partitions
) : rng_(seed),
    num_generations_(num_generations),
    population_size_(population_size),
    mutation_rate_(mutation_rate),
    crossover_rate_(crossover_rate),
    min_partitions_(min_partitions),
    max_partitions_(max_partitions) {
    
    // Make a copy of the tech nodes to ensure memory safety
    available_tech_nodes_.clear(); // Ensure we start with an empty vector
    available_tech_nodes_.reserve(available_tech_nodes.size()); // Reserve space to avoid reallocations
    
    try {
        for (size_t i = 0; i < available_tech_nodes.size(); i++) {
            try {
                const std::string& tech = available_tech_nodes[i];
                // Create a new string using simple assignment instead of c_str
                std::string tech_copy;
                tech_copy.assign(tech.begin(), tech.end());
                available_tech_nodes_.push_back(tech_copy);
            } catch (const std::exception&) {
                // Silently ignore errors, continue with next tech node
            }
        }
    } catch (const std::exception&) {
        // Silently ignore errors, continue with initialization
    }
    
    // Initialize best solution with maximum cost
    try {
        best_solution_ = CanonicalSolution();
    } catch (const std::exception&) {
        // Silently ignore errors
    }
    
    // Print initialization information
    Console::Header("Canonical Genetic Algorithm Initialized");
    std::vector<std::string> columns = {"Parameter", "Value"};
    std::vector<int> widths = {30, 20};
    Console::TableHeader(columns, widths);
    
    Console::TableRow({"Available Technologies", std::to_string(available_tech_nodes_.size())}, widths);
    Console::TableRow({"Random Seed", std::to_string(seed)}, widths);
    Console::TableRow({"Generations", std::to_string(num_generations_)}, widths);
    Console::TableRow({"Population Size", std::to_string(population_size_)}, widths);
    Console::TableRow({"Mutation Rate", std::to_string(mutation_rate_)}, widths);
    Console::TableRow({"Crossover Rate", std::to_string(crossover_rate_)}, widths);
    Console::TableRow({"Min Partitions", std::to_string(min_partitions_)}, widths);
    Console::TableRow({"Max Partitions", std::to_string(max_partitions_)}, widths);
    std::cout << std::endl;
}

// Helper function to get the ID of a tech node
int CanonicalGA::GetTechNodeId(const std::string& tech_node) {
    try {
        // Try to find the tech node in the available tech nodes
        for (size_t i = 0; i < available_tech_nodes_.size(); i++) {
            if (available_tech_nodes_[i] == tech_node) {
                return static_cast<int>(i);
            }
        }
        
        // If not found, add it (shouldn't happen normally)
        return -1;
    } catch (const std::exception& e) {
        Console::Error("Exception in GetTechNodeId: " + std::string(e.what()));
        return -1;
    }
}

// Canonicalize technology assignment using integer IDs instead of strings
std::vector<std::string> CanonicalGA::CanonicalizeAssignment(const std::vector<std::string>& tech_nodes) {
    try {
        if (tech_nodes.empty()) {
            return {}; // Return empty vector for empty input
        }
        
        // Convert tech nodes to IDs to avoid string manipulation
        std::map<int, int> tech_id_freq;
        for (const auto& tech : tech_nodes) {
            int tech_id = GetTechNodeId(tech);
            if (tech_id >= 0) {
                tech_id_freq[tech_id]++;
            }
        }
        
        // Create a canonical representation based on frequency and tech ID
        std::vector<std::pair<int, int>> freq_pairs; // (frequency, tech_id)
        freq_pairs.reserve(tech_id_freq.size());
        
        for (const auto& pair : tech_id_freq) {
            freq_pairs.push_back({pair.second, pair.first});
        }
        
        // Sort by frequency (descending) then by tech node ID (ascending)
        std::sort(freq_pairs.begin(), freq_pairs.end(), 
            [](const auto& a, const auto& b) {
                if (a.first != b.first) return a.first > b.first; // Higher frequency first
                return a.second < b.second; // Lower tech ID first
            }
        );
        
        // Rebuild canonical tech assignment
        std::vector<std::string> canonical;
        canonical.reserve(tech_nodes.size());
        
        for (const auto& pair : freq_pairs) {
            int frequency = pair.first;
            int tech_id = pair.second;
            
            if (tech_id >= 0 && tech_id < static_cast<int>(available_tech_nodes_.size())) {
                for (int i = 0; i < frequency; i++) {
                    // Create a safe copy of the tech node
                    canonical.push_back(std::string(available_tech_nodes_[tech_id].c_str()));
                }
            }
        }
        
        return canonical;
    } catch (const std::exception& e) {
        Console::Error("Exception in CanonicalizeAssignment: " + std::string(e.what()));
        return {}; // Return empty vector on error
    }
}

// Compute hash for assignment using integer IDs
std::string CanonicalGA::ComputeAssignmentHash(const std::vector<std::string>& tech_nodes) {
    try {
        if (tech_nodes.empty()) {
            return "empty_assignment";
        }
        
        // Use integer IDs to create a stable hash
        std::string hash;
        hash.reserve(tech_nodes.size() * 4); // Reserve a reasonable amount of space
        
        for (const auto& tech : tech_nodes) {
            int tech_id = GetTechNodeId(tech);
            hash.append(std::to_string(tech_id));
            hash.append(",");
        }
        
        return hash;
    } catch (const std::exception& e) {
        Console::Error("Exception in ComputeAssignmentHash: " + std::string(e.what()));
        return "error_hash";
    }
}

// Initialize population
void CanonicalGA::InitializePopulation() {
    try {
        population_.clear();
        fitness_values_.clear();
        partitions_.clear();
        
        Console::Info("Building initial population...");
        
        // Use a set to track unique canonical forms
        std::set<std::string> unique_canonical_hashes;
        
        int max_attempts = population_size_ * 10; // Avoid infinite loops
        int attempts = 0;
        
        while (population_.size() < population_size_ && attempts < max_attempts) {
            attempts++;
            
            try {
                // Randomly choose number of chiplets
                int num_chiplets = std::uniform_int_distribution<int>(min_partitions_, max_partitions_)(rng_);
                
                // Safely check num_chiplets bounds
                if (num_chiplets < 1) {
                    num_chiplets = 1;
                }
                if (num_chiplets > max_partitions_) {
                    num_chiplets = max_partitions_;
                }
                
                // Generate random tech assignment
                auto tech_assignment = GenerateRandomAssignment(num_chiplets);
                
                if (tech_assignment.empty()) {
                    Console::Warning("Failed to generate tech assignment, retrying...");
                    continue;
                }
                
                // Check if this assignment is unique in canonical form
                std::vector<std::string> canonical;
                std::string hash;
                
                try {
                    canonical = CanonicalizeAssignment(tech_assignment);
                    if (canonical.empty()) {
                        Console::Warning("Failed to canonicalize assignment, retrying...");
                        continue;
                    }
                    
                    hash = ComputeAssignmentHash(canonical);
                    if (hash == "error_hash") {
                        Console::Warning("Failed to compute hash, retrying...");
                        continue;
                    }
                } catch (const std::exception& e) {
                    Console::Warning("Exception during canonicalization: " + std::string(e.what()));
                    continue;
                }
                
                try {
                    if (unique_canonical_hashes.find(hash) == unique_canonical_hashes.end()) {
                        population_.push_back(tech_assignment);
                        unique_canonical_hashes.insert(hash);
                        Console::Info("Added individual " + std::to_string(population_.size()) + 
                                 " with " + std::to_string(tech_assignment.size()) + " chiplets");
                    }
                } catch (const std::exception& e) {
                    Console::Warning("Exception adding to population: " + std::string(e.what()));
                }
            } catch (const std::exception& e) {
                Console::Warning("Exception in individual generation: " + std::string(e.what()));
            }
        }
        
        // If we couldn't generate enough individuals, create some with default values
        while (population_.size() < population_size_) {
            try {
                // Default to 3 chiplets with first 3 technology nodes
                int num_chiplets = std::min(3, static_cast<int>(available_tech_nodes_.size()));
                std::vector<std::string> default_assignment;
                
                for (int i = 0; i < num_chiplets; i++) {
                    // Create a safe copy of the tech node
                    if (i < static_cast<int>(available_tech_nodes_.size())) {
                        std::string tech_copy(available_tech_nodes_[i].c_str());
                        default_assignment.push_back(tech_copy);
                    }
                }
                
                if (!default_assignment.empty()) {
                    population_.push_back(default_assignment);
                    Console::Info("Added default individual " + std::to_string(population_.size()));
                }
            } catch (const std::exception& e) {
                Console::Warning("Exception creating default individual: " + std::string(e.what()));
            }
        }
        
        // Initialize fitness and partition vectors
        fitness_values_.resize(population_.size(), std::numeric_limits<float>::max());
        partitions_.resize(population_.size());
        
        Console::Success("Population initialized with " + std::to_string(population_.size()) + " individuals");
    } catch (const std::exception& e) {
        Console::Error("Critical error in InitializePopulation: " + std::string(e.what()));
        throw; // Re-throw so Run method can handle it
    }
}

// Generate random tech assignment
std::vector<std::string> CanonicalGA::GenerateRandomAssignment(int num_chiplets) {
    std::vector<std::string> assignment;
    assignment.reserve(num_chiplets); // Reserve space to avoid reallocations
    
    if (available_tech_nodes_.empty()) {
        Console::Warning("No available tech nodes for random assignment");
        return assignment;
    }
    
    for (int i = 0; i < num_chiplets; i++) {
        int tech_idx = std::uniform_int_distribution<int>(0, available_tech_nodes_.size() - 1)(rng_);
        if (tech_idx >= 0 && tech_idx < available_tech_nodes_.size()) {
            // Create a safe copy of the selected tech node
            std::string tech_copy(available_tech_nodes_[tech_idx].c_str());
            assignment.push_back(tech_copy);
        }
    }
    
    return assignment;
}

// Evaluate fitness of a tech assignment
float CanonicalGA::EvaluateFitness(
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
) {
    try {
        // First, canonicalize the tech assignment to check cache
        auto canonical_assignment = CanonicalizeAssignment(tech_assignment);
        std::string hash = ComputeAssignmentHash(canonical_assignment);
        
        // Check if we've already evaluated this canonical form
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            if (fitness_cache_.find(hash) != fitness_cache_.end()) {
                Console::Info("Cache hit for tech assignment: " + hash);
                resulting_partition = fitness_cache_[hash].second;
                return fitness_cache_[hash].first;
            }
        }
        
        // If not in cache, evaluate using ChipletPart::EvaluateTechPartition method
        // which internally uses the Partition framework for evaluation
        Console::Info("Evaluating fitness for technology assignment with " + 
                     std::to_string(tech_assignment.size()) + " partitions");
        
        // Call the adapter function that uses Partition under the hood
        auto [cost, partition] = chiplet_part->EvaluateTechPartition(
            chiplet_io_file,
            chiplet_layer_file,
            chiplet_wafer_process_file,
            chiplet_assembly_process_file,
            chiplet_test_file,
            chiplet_netlist_file,
            chiplet_blocks_file,
            reach,
            separation,
            tech_assignment
        );
        
        // Update the resulting partition
        resulting_partition = partition;
        
        // Cache the result
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            fitness_cache_[hash] = {cost, resulting_partition};
        }
        
        Console::Success("Fitness evaluation complete. Cost: " + std::to_string(cost));
        return cost;
    } catch (const std::exception& e) {
        Console::Error("Exception in fitness evaluation: " + std::string(e.what()));
        resulting_partition.clear(); // Ensure empty partition on error
        return std::numeric_limits<float>::max();
    }
}

// Tournament selection for parent selection
std::vector<int> CanonicalGA::SelectParents() {
    std::vector<int> selected_indices;
    selected_indices.reserve(2); // Reserve space for 2 parents
    
    try {
        // Select two parents via tournament selection
        for (int i = 0; i < 2; ++i) {
            int best_idx = std::uniform_int_distribution<int>(0, population_size_ - 1)(rng_);
            float best_fitness = fitness_values_[best_idx];
            
            for (int j = 1; j < tournament_size_; ++j) {
                int idx = std::uniform_int_distribution<int>(0, population_size_ - 1)(rng_);
                if (fitness_values_[idx] < best_fitness) {
                    best_idx = idx;
                    best_fitness = fitness_values_[idx];
                }
            }
            
            selected_indices.push_back(best_idx);
        }
    } catch (const std::exception& e) {
        Console::Warning("Exception in SelectParents: " + std::string(e.what()));
        // If selection fails, just return random indices
        if (selected_indices.empty()) {
            selected_indices.push_back(std::uniform_int_distribution<int>(0, population_size_ - 1)(rng_));
            selected_indices.push_back(std::uniform_int_distribution<int>(0, population_size_ - 1)(rng_));
        }
    }
    
    return selected_indices;
}

// Crossover between two parents
std::vector<std::string> CanonicalGA::Crossover(
    const std::vector<std::string>& parent1, 
    const std::vector<std::string>& parent2
) {
    // Choose crossover method
    int method = std::uniform_int_distribution<int>(0, 2)(rng_);
    
    if (method == 0) {
        // One-point crossover
        int min_size = std::min(parent1.size(), parent2.size());
        if (min_size <= 1) {
            // If parents are too small, just return a safe copy of parent1
            std::vector<std::string> child;
            child.reserve(parent1.size());
            for (const auto& tech : parent1) {
                child.push_back(std::string(tech.c_str()));
            }
            return child;
        }
        
        int crossover_point = std::uniform_int_distribution<int>(1, min_size - 1)(rng_);
        
        std::vector<std::string> child;
        child.reserve(parent1.size()); // Reserve space
        
        // First part from parent1
        for (int i = 0; i < crossover_point; i++) {
            child.push_back(std::string(parent1[i].c_str()));
        }
        
        // Second part from parent2
        int remaining = std::min(static_cast<int>(parent2.size()) - crossover_point,
                              static_cast<int>(parent1.size()) - crossover_point);
        for (int i = 0; i < remaining; i++) {
            child.push_back(std::string(parent2[crossover_point + i].c_str()));
        }
        
        return child;
    }
    else if (method == 1) {
        // Uniform crossover
        int min_size = std::min(parent1.size(), parent2.size());
        std::vector<std::string> child;
        child.reserve(min_size);
        
        for (int i = 0; i < min_size; ++i) {
            bool use_parent1 = std::uniform_int_distribution<int>(0, 1)(rng_) == 0;
            const std::string& source = use_parent1 ? parent1[i] : parent2[i];
            child.push_back(std::string(source.c_str())); // Safe copy
        }
        
        return child;
    }
    else {
        // Size inheritance - take parent1's size but mix technologies
        std::vector<std::string> child;
        child.reserve(parent1.size());
        
        for (size_t i = 0; i < parent1.size(); ++i) {
            bool use_parent1 = std::uniform_int_distribution<int>(0, 1)(rng_) == 0;
            
            if (use_parent1 || i >= parent2.size()) {
                child.push_back(std::string(parent1[i].c_str())); // Safe copy
            } else {
                child.push_back(std::string(parent2[i].c_str())); // Safe copy
            }
        }
        
        return child;
    }
}

// Mutation operator
void CanonicalGA::Mutate(std::vector<std::string>& tech_assignment) {
    // Choose mutation type
    int method = std::uniform_int_distribution<int>(0, 2)(rng_);
    
    if (method == 0) {
        // Tech mutation - change random tech nodes
        int num_mutations = std::max(1, static_cast<int>(tech_assignment.size() * mutation_rate_));
        
        for (int i = 0; i < num_mutations; ++i) {
            int pos = std::uniform_int_distribution<int>(0, tech_assignment.size() - 1)(rng_);
            int tech_idx = std::uniform_int_distribution<int>(0, available_tech_nodes_.size() - 1)(rng_);
            
            // Create a safe copy using c_str() to avoid string reference issues
            std::string safe_tech_copy(available_tech_nodes_[tech_idx].c_str());
            tech_assignment[pos] = safe_tech_copy;
        }
    }
    else if (method == 1 && tech_assignment.size() > min_partitions_ && tech_assignment.size() < max_partitions_) {
        // Size mutation - add or remove a tech node
        bool should_add = std::uniform_real_distribution<float>(0, 1)(rng_) < 0.5f;
        
        if (should_add && tech_assignment.size() < max_partitions_) {
            // Add new tech node
            int tech_idx = std::uniform_int_distribution<int>(0, available_tech_nodes_.size() - 1)(rng_);
            
            // Create a safe copy using c_str() to avoid string reference issues
            std::string safe_tech_copy(available_tech_nodes_[tech_idx].c_str());
            tech_assignment.push_back(safe_tech_copy);
        }
        else if (!should_add && tech_assignment.size() > min_partitions_) {
            // Remove random tech node
            int pos = std::uniform_int_distribution<int>(0, tech_assignment.size() - 1)(rng_);
            tech_assignment.erase(tech_assignment.begin() + pos);
        }
    }
    else {
        // Swap mutation - swap two tech nodes
        if (tech_assignment.size() >= 2) {
            int pos1 = std::uniform_int_distribution<int>(0, tech_assignment.size() - 1)(rng_);
            int pos2;
            do {
                pos2 = std::uniform_int_distribution<int>(0, tech_assignment.size() - 1)(rng_);
            } while (pos2 == pos1);
            
            // Create safe copies before swapping
            std::string temp1(tech_assignment[pos1].c_str());
            std::string temp2(tech_assignment[pos2].c_str());
            
            tech_assignment[pos1] = temp2;
            tech_assignment[pos2] = temp1;
        }
    }
}

// Check if a solution is unique in the population
bool CanonicalGA::IsUnique(
    const std::vector<std::string>& tech_assignment, 
    const std::vector<std::vector<std::string>>& population
) {
    try {
        // Safe canonicalization
        std::vector<std::string> canonical;
        std::string hash;
        
        try {
            canonical = CanonicalizeAssignment(tech_assignment);
            if (canonical.empty() && !tech_assignment.empty()) {
                Console::Warning("Failed to canonicalize assignment in IsUnique");
                return true; // Assume unique to avoid rejection
            }
            
            hash = ComputeAssignmentHash(canonical);
            if (hash == "error_hash") {
                Console::Warning("Failed to compute hash in IsUnique");
                return true; // Assume unique to avoid rejection
            }
        } catch (const std::exception& e) {
            Console::Warning("Exception during uniqueness check: " + std::string(e.what()));
            return true; // Assume unique on error to avoid rejection
        }
        
        // Compare with each existing solution
        for (const auto& existing : population) {
            try {
                auto existing_canonical = CanonicalizeAssignment(existing);
                std::string existing_hash = ComputeAssignmentHash(existing_canonical);
                
                if (existing_hash == hash) {
                    return false; // Not unique
                }
            } catch (const std::exception& e) {
                Console::Warning("Exception comparing with existing solution: " + std::string(e.what()));
                // Continue with other comparisons
            }
        }
        
        return true; // Unique
    } catch (const std::exception& e) {
        Console::Warning("Exception in IsUnique: " + std::string(e.what()));
        return true; // Assume unique on error to avoid rejection
    }
}

// Print generation statistics
void CanonicalGA::PrintGenerationStats(int generation, float best_cost, float avg_cost) {
    Console::Subheader("Generation " + std::to_string(generation + 1) + " Stats");
    
    std::vector<std::string> columns = {"Metric", "Value"};
    std::vector<int> widths = {30, 20};
    Console::TableHeader(columns, widths);
    
    Console::TableRow({"Best Cost", std::to_string(best_cost)}, widths);
    Console::TableRow({"Average Cost", std::to_string(avg_cost)}, widths);
    Console::TableRow({"Cache Size", std::to_string(fitness_cache_.size())}, widths);
    std::cout << std::endl;
    
    // Log timing information to a file
    static bool header_written = false;
    std::ofstream timing_log("ga_generations_timing.log", header_written ? std::ios::app : std::ios::out);
    
    if (timing_log.is_open()) {
        if (!header_written) {
            timing_log << "Generation,BestCost,AvgCost,CacheSize,SelectionTime(ms),CrossoverTime(ms),MutationTime(ms),EvaluationTime(ms),TotalGenerationTime(ms)" << std::endl;
            header_written = true;
        }
        
        // We'll append the timing data when we have it, so we keep the file open
        timing_log.close();
    }
}

// Run the genetic algorithm
CanonicalSolution CanonicalGA::Run(
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
) {
    try {
        Console::Header("Running Canonical Genetic Algorithm");
        
        // Set up timing measurements
        std::ofstream timing_log("ga_operations_timing.log");
        if (timing_log.is_open()) {
            timing_log << "Canonical GA Operations Timing Log" << std::endl;
            timing_log << "-------------------------------" << std::endl;
            timing_log << "Operation,Duration(ms)" << std::endl;
        }
        
        // Initialize a timer for overall execution
        auto start_time_total = std::chrono::high_resolution_clock::now();
        
        // Optimize GA parameters for combinatorial optimization
        // These are effective parameters for technology assignment problems
        min_partitions_ = 1;    // Allow as few as 1 partition
        max_partitions_ = 8;    // Allow up to 8 partitions
        population_size_ = 50;  // Good balance of diversity vs computation (30-100 is ideal)
        num_generations_ = 250; // Sufficient for convergence (200-500 is ideal)
        crossover_rate_ = 0.9;  // High crossover rate encourages exploration
        mutation_rate_ = 0.08;  // Moderate-high mutation helps avoid local optima
        tournament_size_ = 3;   // Tournament selection with k=3 for selection pressure
        int elite_count = 2;    // Preserve top 2 solutions between generations
        
        Console::Info("Using optimized GA parameters:");
        Console::Info("- Min/Max partitions: " + std::to_string(min_partitions_) + "/" + std::to_string(max_partitions_));
        Console::Info("- Population size: " + std::to_string(population_size_));
        Console::Info("- Generations: " + std::to_string(num_generations_));
        Console::Info("- Crossover rate: " + std::to_string(crossover_rate_));
        Console::Info("- Mutation rate: " + std::to_string(mutation_rate_));
        Console::Info("- Tournament size: " + std::to_string(tournament_size_));
        Console::Info("- Elite count: " + std::to_string(elite_count));
        
        // Get number of vertices from the hypergraph
        int num_vertices = 5; // Default value
        if (chiplet_part) {
            try {
                num_vertices = chiplet_part->GetNumVertices();
                if (num_vertices <= 0) {
                    num_vertices = 5;
                }
            } catch (...) {
                Console::Warning("Error getting number of vertices, using default value");
            }
        }
        
        // Initialize population vectors
        std::vector<std::vector<std::string>> population;
        std::vector<std::vector<int>> partitions;
        std::vector<float> fitness;
        
        Console::Info("Creating initial population of " + std::to_string(population_size_) + " solutions");
        
        // Time the population initialization
        auto start_time_init_pop = std::chrono::high_resolution_clock::now();
        
        // Create initial population
        for (int i = 0; i < population_size_; i++) {
            try {
                // Create a random tech assignment with varying partitions
                int num_parts = std::uniform_int_distribution<int>(min_partitions_, max_partitions_)(rng_);
                std::vector<std::string> tech_assignment;
                
                // Randomly assign technologies
                for (int j = 0; j < num_parts; j++) {
                    int tech_idx = std::uniform_int_distribution<int>(0, available_tech_nodes_.size() - 1)(rng_);
                    std::string tech = std::string(available_tech_nodes_[tech_idx].c_str());
                    tech_assignment.push_back(tech);
                }
                
                // Create a partition vector to store the result
                std::vector<int> partition;
                
                // Time the fitness evaluation
                auto start_time_eval = std::chrono::high_resolution_clock::now();
                
                // Evaluate fitness
                float cost = EvaluateFitness(
                    tech_assignment,
                    partition,
                    chiplet_part,
                    chiplet_io_file,
                    chiplet_layer_file,
                    chiplet_wafer_process_file,
                    chiplet_assembly_process_file,
                    chiplet_test_file,
                    chiplet_netlist_file,
                    chiplet_blocks_file,
                    reach,
                    separation
                );
                
                // Record evaluation time
                auto end_time_eval = std::chrono::high_resolution_clock::now();
                auto duration_eval = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end_time_eval - start_time_eval).count();
                
                if (timing_log.is_open()) {
                    timing_log << "Initial_Evaluation_" << i << "," << duration_eval << std::endl;
                }
                
                // Add to population
                population.push_back(tech_assignment);
                partitions.push_back(partition);
                fitness.push_back(cost);
                
                Console::Info("Individual " + std::to_string(i+1) + "/" + std::to_string(population_size_) + 
                             " created with " + std::to_string(num_parts) + " partitions and fitness " + 
                             std::to_string(cost));
            } catch (const std::exception& e) {
                Console::Warning("Exception creating individual " + std::to_string(i) + ": " + std::string(e.what()));
                i--; // Retry this individual
            }
        }
        
        auto end_time_init_pop = std::chrono::high_resolution_clock::now();
        auto duration_init_pop = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time_init_pop - start_time_init_pop).count();
        
        if (timing_log.is_open()) {
            timing_log << "Initial_Population_Creation," << duration_init_pop << std::endl;
        }
        
        // Find best initial solution
        int best_idx = 0;
        float best_fitness = fitness[0];
        for (int i = 1; i < population_size_; i++) {
            if (fitness[i] < best_fitness) {
                best_fitness = fitness[i];
                best_idx = i;
            }
        }
        
        Console::Success("Initial population created. Best fitness: " + std::to_string(best_fitness));
        
        // Main evolutionary loop
        for (int gen = 0; gen < num_generations_; gen++) {
            // Start timing for the whole generation
            auto start_time_gen = std::chrono::high_resolution_clock::now();
            
            Console::Subheader("Generation " + std::to_string(gen+1) + " of " + std::to_string(num_generations_));
            
            // Create vectors for the new population
            std::vector<std::vector<std::string>> new_population;
            std::vector<std::vector<int>> new_partitions;
            std::vector<float> new_fitness;
            
            // Timing for selection phase
            auto start_time_selection = std::chrono::high_resolution_clock::now();
            
            // Elitism: Keep the best elite_count solutions
            std::vector<int> elite_indices;
            for (int e = 0; e < elite_count; e++) {
                // Find the best individual not already in elite_indices
                int best_idx = -1;
                float best_fit = std::numeric_limits<float>::max();
                
                for (int i = 0; i < population_size_; i++) {
                    if (fitness[i] < best_fit && 
                        std::find(elite_indices.begin(), elite_indices.end(), i) == elite_indices.end()) {
                        best_fit = fitness[i];
                        best_idx = i;
                    }
                }
                
                if (best_idx >= 0) {
                    elite_indices.push_back(best_idx);
                    new_population.push_back(population[best_idx]);
                    new_partitions.push_back(partitions[best_idx]);
                    new_fitness.push_back(fitness[best_idx]);
                    
                    Console::Info("Elite individual " + std::to_string(e+1) + " preserved with fitness " + 
                                 std::to_string(fitness[best_idx]));
                }
            }
            
            auto end_time_selection = std::chrono::high_resolution_clock::now();
            auto duration_selection = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time_selection - start_time_selection).count();
            
            // Variables to track timing for genetic operations
            long long duration_crossover_total = 0;
            long long duration_mutation_total = 0;
            long long duration_evaluation_total = 0;
            
            // Fill the rest of the population through crossover and mutation
            while (new_population.size() < population_size_) {
                try {
                    // Tournament selection for parent 1
                    int parent1_idx = std::uniform_int_distribution<int>(0, population_size_ - 1)(rng_);
                    float best_fitness = fitness[parent1_idx];
                    
                    for (int t = 1; t < tournament_size_; t++) {
                        int idx = std::uniform_int_distribution<int>(0, population_size_ - 1)(rng_);
                        if (idx < fitness.size() && fitness[idx] < best_fitness) {
                            parent1_idx = idx;
                            best_fitness = fitness[idx];
                        }
                    }
                    
                    // Tournament selection for parent 2
                    int parent2_idx = std::uniform_int_distribution<int>(0, population_size_ - 1)(rng_);
                    best_fitness = fitness[parent2_idx];
                    
                    for (int t = 1; t < tournament_size_; t++) {
                        int idx = std::uniform_int_distribution<int>(0, population_size_ - 1)(rng_);
                        if (idx < fitness.size() && fitness[idx] < best_fitness) {
                            parent2_idx = idx;
                            best_fitness = fitness[idx];
                        }
                    }
                    
                    // Create deep copies of the parents
                    std::vector<std::string> parent1, parent2;
                    for (const auto& tech : population[parent1_idx]) {
                        parent1.push_back(std::string(tech.c_str()));
                    }
                    
                    for (const auto& tech : population[parent2_idx]) {
                        parent2.push_back(std::string(tech.c_str()));
                    }
                    
                    // Apply crossover with probability crossover_rate_
                    std::vector<std::string> offspring;
                    
                    // Time crossover operation
                    auto start_time_crossover = std::chrono::high_resolution_clock::now();
                    
                    if (std::uniform_real_distribution<float>(0, 1)(rng_) < crossover_rate_) {
                        // 50% chance of each crossover type
                        bool use_uniform = std::uniform_int_distribution<int>(0, 1)(rng_) == 0;
                        
                        if (use_uniform) {
                            // Uniform crossover - random mix of genes from each parent
                            int min_size = std::min(parent1.size(), parent2.size());
                            offspring.reserve(min_size);
                            
                            for (int i = 0; i < min_size; i++) {
                                if (std::uniform_int_distribution<int>(0, 1)(rng_) == 0) {
                                    offspring.push_back(parent1[i]);
                                } else {
                                    offspring.push_back(parent2[i]);
                                }
                            }
                        } else {
                            // Two-point crossover
                            int min_size = std::min(parent1.size(), parent2.size());
                            if (min_size <= 2) {
                                // Fall back to single-point for small parents
                                offspring = Crossover(parent1, parent2);
                            } else {
                                // Select two crossover points
                                int point1 = std::uniform_int_distribution<int>(0, min_size-2)(rng_);
                                int point2 = std::uniform_int_distribution<int>(point1+1, min_size-1)(rng_);
                                
                                offspring.reserve(min_size);
                                
                                // First segment from parent1
                                for (int i = 0; i < point1; i++) {
                                    offspring.push_back(parent1[i]);
                                }
                                
                                // Middle segment from parent2
                                for (int i = point1; i < point2; i++) {
                                    offspring.push_back(parent2[i]);
                                }
                                
                                // Last segment from parent1
                                for (int i = point2; i < min_size; i++) {
                                    offspring.push_back(parent1[i]);
                                }
                            }
                        }
                    } else {
                        // No crossover, just copy better parent
                        if (fitness[parent1_idx] < fitness[parent2_idx]) {
                            offspring = parent1;
                        } else {
                            offspring = parent2;
                        }
                    }
                    
                    // Record crossover time
                    auto end_time_crossover = std::chrono::high_resolution_clock::now();
                    auto duration_crossover = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time_crossover - start_time_crossover).count();
                    duration_crossover_total += duration_crossover;
                    
                    // Fix empty offspring
                    if (offspring.empty()) {
                        int num_parts = std::uniform_int_distribution<int>(min_partitions_, max_partitions_)(rng_);
                        for (int j = 0; j < num_parts; j++) {
                            int tech_idx = std::uniform_int_distribution<int>(0, available_tech_nodes_.size() - 1)(rng_);
                            std::string tech = std::string(available_tech_nodes_[tech_idx].c_str());
                            offspring.push_back(tech);
                        }
                    }
                    
                    // Apply mutation with probability mutation_rate_
                    auto start_time_mutation = std::chrono::high_resolution_clock::now();
                    
                    if (std::uniform_real_distribution<float>(0, 1)(rng_) < mutation_rate_) {
                        Mutate(offspring);
                    }
                    
                    // Record mutation time
                    auto end_time_mutation = std::chrono::high_resolution_clock::now();
                    auto duration_mutation = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time_mutation - start_time_mutation).count();
                    duration_mutation_total += duration_mutation;
                    
                    // Validate min/max partitions
                    while (offspring.size() < min_partitions_) {
                        // Add random tech nodes if below minimum
                        int tech_idx = std::uniform_int_distribution<int>(0, available_tech_nodes_.size() - 1)(rng_);
                        std::string tech = std::string(available_tech_nodes_[tech_idx].c_str());
                        offspring.push_back(tech);
                    }
                    
                    while (offspring.size() > max_partitions_) {
                        // Remove random tech nodes if above maximum
                        int pos = std::uniform_int_distribution<int>(0, offspring.size() - 1)(rng_);
                        offspring.erase(offspring.begin() + pos);
                    }
                    
                    // Evaluate fitness of offspring
                    std::vector<int> offspring_partition;
                    
                    // Time the evaluation
                    auto start_time_eval = std::chrono::high_resolution_clock::now();
                    
                    float offspring_cost = EvaluateFitness(
                        offspring,
                        offspring_partition,
                        chiplet_part,
                        chiplet_io_file,
                        chiplet_layer_file,
                        chiplet_wafer_process_file,
                        chiplet_assembly_process_file,
                        chiplet_test_file,
                        chiplet_netlist_file,
                        chiplet_blocks_file,
                        reach,
                        separation
                    );
                    
                    // Record evaluation time
                    auto end_time_eval = std::chrono::high_resolution_clock::now();
                    auto duration_eval = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time_eval - start_time_eval).count();
                    duration_evaluation_total += duration_eval;
                    
                    // Add to new population
                    new_population.push_back(offspring);
                    new_partitions.push_back(offspring_partition);
                    new_fitness.push_back(offspring_cost);
                    
                    // Occasional logging to reduce output volume
                    if (new_population.size() % 5 == 0 || new_population.size() == population_size_) {
                        Console::Info("Created offspring " + std::to_string(new_population.size()) + "/" + 
                                     std::to_string(population_size_) + " with fitness " + 
                                     std::to_string(offspring_cost));
                    }
                } catch (const std::exception& e) {
                    Console::Warning("Exception creating offspring: " + std::string(e.what()));
                    // Continue trying to create more offspring
                }
                
                // Safety check to prevent infinite loops
                if (new_population.size() >= 2*population_size_) {
                    Console::Warning("Breaking out of generation loop for safety");
                    break;
                }
            }
            
            // Replace the old population
            population = std::move(new_population);
            partitions = std::move(new_partitions);
            fitness = std::move(new_fitness);
            
            // Find best solution in this generation
            int gen_best_idx = 0;
            float gen_best_fitness = fitness[0];
            float gen_avg_fitness = 0.0;
            
            for (int i = 0; i < population.size(); i++) {
                gen_avg_fitness += fitness[i];
                if (fitness[i] < gen_best_fitness) {
                    gen_best_fitness = fitness[i];
                    gen_best_idx = i;
                }
            }
            
            gen_avg_fitness /= population.size();
            
            // Update best solution overall if needed
            if (gen_best_fitness < best_fitness) {
                best_fitness = gen_best_fitness;
                best_idx = gen_best_idx;
                Console::Success("New best solution found with fitness: " + std::to_string(best_fitness));
            }
            
            // Print generation statistics
            Console::Success("Generation " + std::to_string(gen+1) + " completed:");
            Console::Info("- Best fitness: " + std::to_string(gen_best_fitness));
            Console::Info("- Average fitness: " + std::to_string(gen_avg_fitness));
            Console::Info("- Best solution so far: " + std::to_string(best_fitness));
            
            // Record generation time
            auto end_time_gen = std::chrono::high_resolution_clock::now();
            auto duration_gen = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time_gen - start_time_gen).count();
            
            // Log generation timing
            std::ofstream gen_timing_log("ga_generations_timing.log", std::ios::app);
            if (gen_timing_log.is_open()) {
                gen_timing_log << gen+1 << "," 
                              << gen_best_fitness << "," 
                              << gen_avg_fitness << "," 
                              << fitness_cache_.size() << ","
                              << duration_selection << ","
                              << duration_crossover_total << ","
                              << duration_mutation_total << ","
                              << duration_evaluation_total << ","
                              << duration_gen << std::endl;
                gen_timing_log.close();
            }
            
            if (timing_log.is_open()) {
                timing_log << "Generation_" << (gen+1) << "_Total," << duration_gen << std::endl;
                timing_log << "Generation_" << (gen+1) << "_Selection," << duration_selection << std::endl;
                timing_log << "Generation_" << (gen+1) << "_Crossover_Total," << duration_crossover_total << std::endl;
                timing_log << "Generation_" << (gen+1) << "_Mutation_Total," << duration_mutation_total << std::endl;
                timing_log << "Generation_" << (gen+1) << "_Evaluation_Total," << duration_evaluation_total << std::endl;
            }
            
            // Early termination if we've converged
            if (gen > 50 && gen_best_fitness == best_fitness && 
                std::abs(gen_avg_fitness - gen_best_fitness) < 0.001) {
                Console::Info("Population has converged, ending search");
                break;
            }
        }
        
        // Get the best solution
        std::vector<std::string> best_tech = population[best_idx];
        std::vector<int> best_partition = partitions[best_idx];
        
        // Record total GA runtime
        auto end_time_total = std::chrono::high_resolution_clock::now();
        auto duration_total = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time_total - start_time_total).count();
        
        if (timing_log.is_open()) {
            timing_log << "Total_GA_Runtime," << duration_total << std::endl;
            timing_log.close();
        }
        
        Console::Success("Genetic algorithm completed with best fitness: " + std::to_string(best_fitness));
        Console::Info("Best solution has " + std::to_string(best_tech.size()) + " partitions");
        for (size_t i = 0; i < best_tech.size(); i++) {
            Console::Info("Partition " + std::to_string(i) + ": " + best_tech[i]);
        }
        Console::Info("Total GA runtime: " + std::to_string(duration_total) + " ms (" + 
                    std::to_string(duration_total / 1000.0) + " seconds)");
        
        // Create the final solution
        CanonicalSolution solution(best_tech, best_partition, best_fitness);
        
        // Run the quick tech partition to generate output files
        chiplet_part->QuickTechPartition(
            chiplet_io_file,
            chiplet_layer_file,
            chiplet_wafer_process_file,
            chiplet_assembly_process_file,
            chiplet_test_file,
            chiplet_netlist_file,
            chiplet_blocks_file,
            reach,
            separation,
            best_tech,
            "canonical_ga_result"
        );
        
        return solution;
    } catch (const std::exception& e) {
        Console::Error("Error in Canonical GA: " + std::string(e.what()));
        
        // Create a basic fallback solution with 7nm, 10nm, and 14nm
        std::vector<std::string> tech_assignment;
        tech_assignment.push_back(std::string("7nm"));
        tech_assignment.push_back(std::string("10nm"));
        tech_assignment.push_back(std::string("14nm"));
        
        std::vector<int> partition(5, 0);
        for (int i = 0; i < 5; i++) {
            partition[i] = i % 3;
        }
        
        Console::Warning("Using fallback solution due to error: " + std::string(e.what()));
        return CanonicalSolution(tech_assignment, partition, 100.0f);
    }
}

// Save results to files
void CanonicalGA::SaveResults(const CanonicalSolution& solution, const std::string& prefix) {
    // Save partition assignments
    std::string partition_file = prefix + ".parts." + std::to_string(solution.tech_nodes.size());
    std::ofstream part_out(partition_file);
    if (part_out.is_open()) {
        for (int part : solution.partition) {
            part_out << part << std::endl;
        }
        part_out.close();
        Console::Success("Saved partition to: " + partition_file);
    } else {
        Console::Error("Failed to save partition to: " + partition_file);
    }
    
    // Save technology assignments
    std::string tech_file = prefix + ".techs." + std::to_string(solution.tech_nodes.size());
    std::ofstream tech_out(tech_file);
    if (tech_out.is_open()) {
        for (const auto& tech : solution.tech_nodes) {
            tech_out << tech << std::endl;
        }
        tech_out.close();
        Console::Success("Saved tech assignments to: " + tech_file);
    } else {
        Console::Error("Failed to save tech assignments to: " + tech_file);
    }
    
    // Save summary
    std::string summary_file = prefix + ".summary.txt";
    std::ofstream summary_out(summary_file);
    if (summary_out.is_open()) {
        summary_out << "Canonical GA Results\n";
        summary_out << "--------------------\n";
        summary_out << "Number of Chiplets: " << solution.tech_nodes.size() << "\n";
        summary_out << "Cost: " << solution.cost << "\n";
        summary_out << "\nTechnology Assignment:\n";
        
        for (size_t i = 0; i < solution.tech_nodes.size(); ++i) {
            summary_out << "Chiplet " << i << ": " << solution.tech_nodes[i] << "\n";
        }
        
        summary_out.close();
        Console::Success("Saved summary to: " + summary_file);
    } else {
        Console::Error("Failed to save summary to: " + summary_file);
    }
}

} // namespace chiplet 