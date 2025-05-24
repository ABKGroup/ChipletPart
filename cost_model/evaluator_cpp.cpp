#include "evaluator_cpp.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

// Scale Areas Function based on the Area Scaling Factors from the "Scaling Equations for the Accurate Prediction of CMOS Device Performance from 180nm to 7nm" paper.
float area_scaling_factor(std::string initial_tech_node, std::string actual_tech_node, bool is_memory) {
    // Define the scaling factors for the area.
    std::vector<std::vector<float>> area_scaling_factors = {
        {1, 0.53, 0.35, 0.16, 0.075, 0.067, 0.061, 0.036, 0.021},
        {1.9, 1, 0.66, 0.31, 0.14, 0.13, 0.12, 0.068, 0.039},
        {2.8, 1.5, 1, 0.46, 0.21, 0.19, 0.17, 0.1, 0.059},
        {6.1, 3.3, 2.2, 1, 0.46, 0.41, 0.38, 0.22, 0.13},
        {13, 7.1, 4.7, 2.2, 1, 0.89, 0.82, 0.48, 0.28},
        {15, 7.9, 5.3, 2.4, 1.1, 1, 0.91, 0.54, 0.31},
        {16, 8.7, 5.8, 2.7, 1.2, 1.1, 1, 0.59, 0.34},
        {28, 15, 9.8, 4.5, 2.1, 1.9, 1.7, 1, 0.58},
        {48, 25, 17, 7.8, 3.6, 3.2, 2.9, 1.7, 1}
    };
    std::vector<std::vector<float>> memory_area_scaling_factors = {
        {1, 0.53, 0.43, 0.19, 0.1, 0.12, 0.1, 0.096, 0.077},
        {1.9, 1, 0.836, 0.372, 0.187, 0.238, 0.2, 0.18, 0.143},
        {2.2, 1.18, 1, 0.44, 0.22, 0.275, 0.22, 0.21, 0.17},
        {5.1, 2.75, 2.3, 1, 0.51, 0.63, 0.53, 0.49, 0.40},
        {9.75, 5.3, 4.47, 1.98, 1, 1.22, 1.03, 0.96, 0.77},
        {8.2, 4.3, 3.7, 1.6, 0.8, 1, 0.82, 0.79, 0.62},
        {9.6, 5.22, 4.4, 1.9, 0.96, 1.2, 1, 0.94, 0.75},
        {10.5, 5.6, 4.6, 2.02, 1.05, 1.3, 1.06, 1, 0.798},
        {13, 6.8, 5.9, 2.5, 1.3, 1.6, 1.3, 1.2, 1}
    };
    // Tech Nodes
    std::vector<std::string> tech_nodes = {
        "90nm", "65nm", "45nm", "32nm", "20nm", "16nm", "14nm", "10nm", "7nm"
    };
    
    // Find the index of the initial_tech_node and actual_tech_node in the tech_nodes vector.
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
        std::cout << "Initial or actual tech node not found. Exiting..." << std::endl;
        exit(1);
    }
    // Return the scaling factor.
    if (is_memory)
        return memory_area_scaling_factors[initial_index][actual_index];
    else
        return area_scaling_factors[initial_index][actual_index];
}

// Scale Power Function based on the Power Scaling Factors for Inverter
float power_scaling_factor(std::string initial_tech_node, std::string actual_tech_node) {
    // Define the scaling factors for the power.
    std::vector<float> power_scaling_factors = {105, 26.1, 13.0, 8.58, 5.19, 2.47, 1.51, 1.28, 0.995, 0.866, 0.789};
    // Tech Nodes
    std::vector<std::string> tech_nodes = {
        "180nm", "130nm", "90nm", "65nm", "45nm", "32nm", "20nm", "16nm", "14nm", "10nm", "7nm"
    };
    // Find the index of the initial_tech_node and actual_tech_node in the tech_nodes vector.
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
    return power_scaling_factors[actual_index]/power_scaling_factors[initial_index];
}

// Read library definitions from files
LibraryDicts* readLibraries(std::string io_file, std::string layer_file, std::string wafer_process_file, 
                            std::string assembly_process_file, std::string test_file, std::string netlist_file) {
    LibraryDicts* libraryDicts = new LibraryDicts();
    
    try {
        // Use the existing file-level parsing functions instead of ReadAllDefinitionsFromFiles
        libraryDicts->wafer_processes = read_design::WaferProcessDefinitionListFromFile(wafer_process_file);
        libraryDicts->ios = read_design::IODefinitionListFromFile(io_file);
        libraryDicts->layers = read_design::LayerDefinitionListFromFile(layer_file);
        libraryDicts->assemblies = read_design::AssemblyProcessDefinitionListFromFile(assembly_process_file);
        
        libraryDicts->tests = read_design::TestProcessDefinitionListFromFile(test_file);   
        // Parse netlist for adjacency matrix and bandwidth utilization
        auto result = read_design::GlobalAdjacencyMatrixFromFile(netlist_file, libraryDicts->ios);
        libraryDicts->global_adjacency_matrix = std::get<0>(result);
        libraryDicts->average_bandwidth_utilization = std::get<1>(result);
        libraryDicts->block_names = std::get<2>(result);
    } catch (const std::exception& e) {
        std::cerr << "Exception while reading library files: " << e.what() << std::endl;
        delete libraryDicts;
        return nullptr;
    }
    
    return libraryDicts;
}

// Read block definitions from file
std::vector<block> readBlocks(std::string blocks_file) {
    std::vector<block> blocks;

    std::ifstream blocksFile(blocks_file);
    std::string line;
    while (std::getline(blocksFile, line)) {
        std::istringstream iss(line);
        std::string name;
        float area;
        float power;
        std::string tech;
        bool is_memory;
        if (!(iss >> name >> area >> power >> tech >> is_memory)) {
            std::cout << "Error reading blocks file" << std::endl;
            break;
        }
        block newBlock;
        newBlock.name = name;
        newBlock.area = area;
        newBlock.power = power;
        newBlock.tech = tech;
        newBlock.is_memory = is_memory;
        blocks.push_back(newBlock);
    }

    return blocks;
}

// Initialize database and return library dictionaries
LibraryDicts* init(std::string io_file, std::string layer_file, std::string wafer_process_file, 
                  std::string assembly_process_file, std::string test_file, std::string netlist_file, 
                  std::string blocks_file) {
    // First read the libraries
    LibraryDicts* libraryDicts = readLibraries(io_file, layer_file, wafer_process_file, 
                                             assembly_process_file, test_file, netlist_file);
    
    // If library reading failed, return null
    if (!libraryDicts) {
        return nullptr;
    }
    
    return libraryDicts;
}

// Free resources associated with library dictionaries
int destroyDatabase(LibraryDicts* libraryDicts) {
    if (libraryDicts) {
        delete libraryDicts;
        return 0;
    }
    return -1;
}

// Helper function to get number of partitions
int getNumPartitions(const std::vector<int>& partitionIDs) {
    int numPartitions = 0;
    for (int partitionId : partitionIDs) {
        if (partitionId > numPartitions) {
            numPartitions = partitionId;
        }
    }
    return numPartitions + 1;
}

// Helper function to group blocks by partition
std::vector<std::vector<int>> getPartitionVector(const std::vector<int>& partitionIDs) {
    // Get the largest partition ID in partitionIDs.
    int numPartitions = getNumPartitions(partitionIDs);

    // Get a list of block numbers for each partition.
    std::vector<std::vector<int>> partitionVector = std::vector<std::vector<int>>(numPartitions, std::vector<int>());

    for (int blockId = 0; blockId < partitionIDs.size(); blockId++) {
        partitionVector[partitionIDs[blockId]].push_back(blockId);
    }

    return partitionVector;
}

// Calculate areas for each partition - exactly matching getAreas implementation
float getAreas(std::vector<float>& chiplet_areas, const std::vector<std::vector<int>>& partitionVector, const std::vector<block>& blocks, const std::vector<std::string>& tech_array, int numPartitions) {
    float total_area = 0.0;
    for (int i = 0; i < numPartitions; ++i) {
        chiplet_areas.push_back(0.0);
    }
    for (int partitionId = 0; partitionId < numPartitions; partitionId++) {
        for (int blockId = 0; blockId < partitionVector[partitionId].size(); blockId++) {
            chiplet_areas[partitionId] += blocks[partitionVector[partitionId][blockId]].area*area_scaling_factor(blocks[partitionVector[partitionId][blockId]].tech, tech_array[partitionId], blocks[partitionVector[partitionId][blockId]].is_memory);
        }
        total_area += chiplet_areas[partitionId];
    }
    return total_area;
}

// Calculate power for each partition - exactly matching getPowers implementation 
float getPowers(std::vector<float>& chiplet_powers, const std::vector<std::vector<int>>& partitionVector, const std::vector<block>& blocks, const std::vector<std::string>& tech_array, int numPartitions) {
    float total_power = 0.0;
    for (int i = 0; i < numPartitions; ++i) {
        chiplet_powers.push_back(0.0);
    }
    for (int partitionId = 0; partitionId < numPartitions; partitionId++) {
        for (int blockId = 0; blockId < partitionVector[partitionId].size(); blockId++) {
            chiplet_powers[partitionId] += blocks[partitionVector[partitionId][blockId]].power*power_scaling_factor(blocks[partitionVector[partitionId][blockId]].tech, tech_array[partitionId]);
        }
        total_power += chiplet_powers[partitionId];
    }
    return total_power;
}

// Build a chip model based on partition IDs
std::shared_ptr<design::Chip> buildModel(const std::vector<int>& partitionIDs, 
                                       const std::vector<std::string>& tech_array, 
                                       const std::vector<float>& aspect_ratios, 
                                       const std::vector<float>& x_locations, 
                                       const std::vector<float>& y_locations, 
                                       LibraryDicts* libraryDicts, 
                                       const std::vector<block>& blocks,
                                       bool approx_state) {
    // Get the number of partitions
    int numPartitions = getNumPartitions(partitionIDs);

    // explicitly trim the vectors to the number of partitions
    std::vector<std::string> tech_array_trimmed(tech_array.begin(), tech_array.begin() + numPartitions);
    std::vector<float> aspect_ratios_trimmed(aspect_ratios.begin(), aspect_ratios.begin() + numPartitions);
    std::vector<float> x_locations_trimmed(x_locations.begin(), x_locations.begin() + numPartitions);
    std::vector<float> y_locations_trimmed(y_locations.begin(), y_locations.begin() + numPartitions);

    // Validate input vector sizes
    if (tech_array.size() != numPartitions) {
        //std::cerr << "Error in buildModel: tech_array size (" << tech_array.size() 
        //          << ") does not match number of partitions (" << numPartitions << ")" << std::endl;
        return nullptr;
    }
    
    if (aspect_ratios.size() != numPartitions) {
        //std::cerr << "Error in buildModel: aspect_ratios size (" << aspect_ratios.size() 
        //          << ") does not match number of partitions (" << numPartitions << ")" << std::endl;
        return nullptr;
    }
    
    if (x_locations.size() != numPartitions) {
        std::cerr << "Error in buildModel: x_locations size (" << x_locations.size() 
                  << ") does not match number of partitions (" << numPartitions << ")" << std::endl;
        return nullptr;
    }
    
    if (y_locations.size() != numPartitions) {
        std::cerr << "Error in buildModel: y_locations size (" << y_locations.size() 
                  << ") does not match number of partitions (" << numPartitions << ")" << std::endl;
        return nullptr;
    }
    
    // Group blocks by partition
    std::vector<std::vector<int>> partitionVector = getPartitionVector(partitionIDs);
    
    // Calculate areas and power for each partition
    std::vector<float> chiplet_areas;
    std::vector<float> chiplet_powers;
    
    float total_area = getAreas(chiplet_areas, partitionVector, blocks, tech_array_trimmed, numPartitions);

    float total_power = getPowers(chiplet_powers, partitionVector, blocks, tech_array_trimmed, numPartitions);
    
    // Get block chiplet names and block names
    std::vector<std::string> block_chiplet_names = getBlockChipletNames(numPartitions);
    std::vector<std::string> block_names = getBlockNames(blocks);
    
    // Combine blocks to create new connectivity matrices based on partition assignments
    // This matches the Python implementation's call to combine_blocks
    
    // Convert int matrices to double matrices for CombineBlocks
    std::map<std::string, std::vector<std::vector<double>>> global_adj_matrix_double;
    for (const auto& [key, matrix] : libraryDicts->global_adjacency_matrix) {
        std::vector<std::vector<double>> double_matrix;
        for (const auto& row : matrix) {
            std::vector<double> double_row(row.begin(), row.end());
            double_matrix.push_back(double_row);
        }
        global_adj_matrix_double[key] = double_matrix;
    }
    
    // Use CombineBlocks with the converted matrices
    auto combined_result = construct_chip::CombineBlocks(
        global_adj_matrix_double,
        libraryDicts->average_bandwidth_utilization,
        block_names,
        partitionVector
    );
    
    // Extract the results
    std::map<std::string, std::vector<std::vector<double>>> combined_adjacency_matrix = std::get<0>(combined_result);
    std::map<std::string, std::vector<std::vector<double>>> combined_average_bandwidth_utilization = std::get<1>(combined_result);
    std::vector<std::string> combined_block_names = std::get<2>(combined_result);
    
    // Convert the combined adjacency matrix back to int if needed for CreateChipFromXML
    std::map<std::string, std::vector<std::vector<int>>> combined_adjacency_matrix_int;
    for (const auto& [key, matrix] : combined_adjacency_matrix) {
        std::vector<std::vector<int>> int_matrix;
        for (const auto& row : matrix) {
            std::vector<int> int_row;
            for (double val : row) {
                int_row.push_back(static_cast<int>(val));
            }
            int_matrix.push_back(int_row);
        }
        combined_adjacency_matrix_int[key] = int_matrix;
    }

    // Create a chip XML using ConstructChip
    pugi::xml_document chip_doc = construct_chip::CreateElementTree(
        tech_array, 
        std::vector<double>(aspect_ratios_trimmed.begin(), aspect_ratios_trimmed.end()), 
        std::vector<double>(x_locations_trimmed.begin(), x_locations_trimmed.end()), 
        std::vector<double>(y_locations_trimmed.begin(), y_locations_trimmed.end()), 
        std::vector<double>(chiplet_powers.begin(), chiplet_powers.end()), 
        std::vector<double>(chiplet_areas.begin(), chiplet_areas.end()), 
        numPartitions
    );
    
    // Convert the XML to a Chip object using ReadDesignFromFile
    // Use the combined connectivity matrices instead of the original ones
    std::shared_ptr<design::Chip> chip = read_design::CreateChipFromXML(
        chip_doc.child("chip"), 
        libraryDicts->wafer_processes, 
        libraryDicts->assemblies, 
        libraryDicts->tests, 
        libraryDicts->layers, 
        libraryDicts->ios, 
        combined_adjacency_matrix_int,  // Use combined matrix instead of original
        combined_average_bandwidth_utilization,  // Use combined matrix instead of original
        block_chiplet_names,  // Use block_chiplet_names as in Python version
        approx_state
    );

    //chip->PrintDescription();
    
    return chip;
}

// Free resources associated with a chip model
int destroyModel(std::shared_ptr<design::Chip> model) {
    // Nothing to do for shared_ptr - it will be automatically released when ref count reaches zero
    return 0;
}

// Calculate cost from scratch for a given partition configuration
float getCostFromScratch(const std::vector<int>& partitionIds, 
                        const std::vector<std::string>& tech_array, 
                        const std::vector<float>& aspect_ratios, 
                        const std::vector<float>& x_locations, 
                        const std::vector<float>& y_locations, 
                        LibraryDicts* libraryDicts, 
                        const std::vector<block>& blocks, 
                        float cost_coefficient, 
                        float power_coefficient,
                        bool approx_state) {
    // Build the chip model with approx_state parameter
    std::shared_ptr<design::Chip> chip = buildModel(
        partitionIds, tech_array, aspect_ratios, x_locations, y_locations, libraryDicts, blocks, approx_state);
    
    // Check if model building failed
    if (chip == nullptr) {
        //std::cerr << "Error in getCostFromScratch: Failed to build chip model" << std::endl;
        return std::numeric_limits<float>::max(); // Return max float value to indicate invalid solution
    }
    
    // Calculate the cost and power
    float cost = chip->ComputeCost();
    float power = chip->GetPower();  // Use GetPower() instead of ComputePower()
    std::cout << "Cost: " << cost << ", Power: " << power << std::endl;
    
    // Calculate the weighted sum
    float total_cost = cost_coefficient * cost + power_coefficient * power;
    
    return total_cost;
}

// Calculate cost and slopes for gradient-based optimization
float getCostAndSlopes(const std::vector<int>& partitionIds, 
                      const std::vector<std::string>& tech_array, 
                      const std::vector<float>& aspect_ratios, 
                      const std::vector<float>& x_locations, 
                      const std::vector<float>& y_locations, 
                      LibraryDicts* libraryDicts, 
                      const std::vector<block>& blocks, 
                      std::vector<float>& costAreaSlopes, 
                      std::vector<float>& powerAreaSlopes, 
                      std::vector<float>& costBandwidthSlopes, 
                      std::vector<float>& powerBandwidthSlopes, 
                      float& costConfidenceInterval, 
                      float& powerConfidenceInterval, 
                      float cost_coefficient, 
                      float power_coefficient) {
    // Build the chip model
    std::shared_ptr<design::Chip> chip = buildModel(
        partitionIds, tech_array, aspect_ratios, x_locations, y_locations, libraryDicts, blocks);
    
    // Calculate the base cost and power
    float base_cost = chip->ComputeCost();
    float base_power = chip->GetPower();  // Use GetPower() instead of ComputePower()
    
    // Calculate the weighted sum
    float total_cost = cost_coefficient * base_cost + power_coefficient * base_power;
    
    // Initialize output vectors with zeros
    costAreaSlopes.resize(partitionIds.size(), 0.0);
    powerAreaSlopes.resize(partitionIds.size(), 0.0);
    costBandwidthSlopes.resize(partitionIds.size(), 0.0);
    powerBandwidthSlopes.resize(partitionIds.size(), 0.0);
    
    // Calculate area slopes - we'll use a small delta for numeric differentiation
    double area_delta = 0.01; // 1% change in area
    
    for (size_t i = 0; i < partitionIds.size(); i++) {
        int partition = partitionIds[i];
        if (partition >= 0) {
            // Create a modified blocks vector with increased area for this block
            std::vector<block> modified_blocks = blocks;
            modified_blocks[i].area *= (1.0 + area_delta);
            
            // Build a new chip model and calculate new cost/power
            std::shared_ptr<design::Chip> delta_chip = buildModel(
                partitionIds, tech_array, aspect_ratios, x_locations, y_locations, libraryDicts, modified_blocks);
            
            float delta_cost = delta_chip->ComputeCost();
            float delta_power = delta_chip->GetPower();  // Use GetPower() instead of ComputePower()
            
            // Calculate slopes
            costAreaSlopes[i] = (delta_cost - base_cost) / (area_delta * blocks[i].area);
            powerAreaSlopes[i] = (delta_power - base_power) / (area_delta * blocks[i].area);
        }
    }
    
    // Calculate bandwidth slopes
    double bandwidth_delta = 0.01; // 1% change in bandwidth
    
    for (size_t i = 0; i < tech_array.size(); i++) {
        // Create modified adjacency matrices with increased bandwidth
        std::map<std::string, std::vector<std::vector<double>>> modified_bandwidth = 
            construct_chip::IncrementNetlist(libraryDicts->average_bandwidth_utilization, bandwidth_delta, i);
        
        // Save original bandwidth matrix
        auto original_bandwidth = libraryDicts->average_bandwidth_utilization;
        
        // Temporarily replace the bandwidth matrix
        libraryDicts->average_bandwidth_utilization = modified_bandwidth;
        
        // Build a new chip model and calculate new cost/power
        std::shared_ptr<design::Chip> delta_chip = buildModel(
            partitionIds, tech_array, aspect_ratios, x_locations, y_locations, libraryDicts, blocks);
        
        float delta_cost = delta_chip->ComputeCost();
        float delta_power = delta_chip->GetPower();  // Use GetPower() instead of ComputePower()
        
        // Calculate slopes for this partition
        for (size_t j = 0; j < partitionIds.size(); j++) {
            if (partitionIds[j] == i) {
                costBandwidthSlopes[j] = (delta_cost - base_cost) / bandwidth_delta;
                powerBandwidthSlopes[j] = (delta_power - base_power) / bandwidth_delta;
            }
        }
        
        // Restore original bandwidth matrix
        libraryDicts->average_bandwidth_utilization = original_bandwidth;
    }
    
    // Set confidence intervals (could be calculated based on model uncertainty)
    costConfidenceInterval = 0.05 * base_cost;  // 5% of base cost
    powerConfidenceInterval = 0.05 * base_power; // 5% of base power
    
    return total_cost;
}

// Calculate costs for moving each block to each partition
std::vector<std::vector<float>> getCostIncremental(const std::vector<int>& basePartitionIds, 
                                                 const std::vector<std::string>& tech_array, 
                                                 const std::vector<float>& aspect_ratios, 
                                                 const std::vector<float>& x_locations, 
                                                 const std::vector<float>& y_locations, 
                                                 const int numPartitions, 
                                                 LibraryDicts* libraryDicts, 
                                                 const std::vector<block>& blocks, 
                                                 float cost_coefficient, 
                                                 float power_coefficient) {
    // Calculate the base cost
    float base_cost = getCostFromScratch(
        basePartitionIds, tech_array, aspect_ratios, x_locations, y_locations, 
        libraryDicts, blocks, cost_coefficient, power_coefficient);
    
    // Initialize a matrix to store the cost change for each block and partition
    std::vector<std::vector<float>> cost_matrix(blocks.size(), std::vector<float>(numPartitions, 0.0));
    
    // For each block, calculate the cost of moving it to each partition
    for (size_t i = 0; i < blocks.size(); i++) {
        int current_partition = basePartitionIds[i];
        
        for (int new_partition = 0; new_partition < numPartitions; new_partition++) {
            if (new_partition == current_partition) {
                // No change if staying in the same partition
                cost_matrix[i][new_partition] = 0.0;
            } else {
                // Create modified partition assignments
                std::vector<int> modified_partitions = basePartitionIds;
                modified_partitions[i] = new_partition;
                
                // Calculate new cost
                float new_cost = getCostFromScratch(
                    modified_partitions, tech_array, aspect_ratios, x_locations, y_locations, 
                    libraryDicts, blocks, cost_coefficient, power_coefficient);
                
                // Store the cost difference
                cost_matrix[i][new_partition] = new_cost - base_cost;
            }
        }
    }
    
    return cost_matrix;
}

// Calculate cost for moving a single block between partitions
float getSingleMoveCost(const std::vector<int>& basePartitionIds, 
                       const std::vector<std::string>& tech_array, 
                       const std::vector<float>& aspect_ratios, 
                       const std::vector<float>& x_locations, 
                       const std::vector<float>& y_locations, 
                       const int blockId, 
                       const int fromPartitionId, 
                       const int toPartitionId, 
                       LibraryDicts* libraryDicts, 
                       const std::vector<block>& blocks, 
                       float cost_coefficient, 
                       float power_coefficient) {
    // Check if the block is currently in the fromPartitionId
    if (basePartitionIds[blockId] != fromPartitionId) {
        std::cerr << "Block " << blockId << " is not in partition " << fromPartitionId << std::endl;
        return 0.0;
    }
    
    // Calculate the base cost
    float base_cost = getCostFromScratch(
        basePartitionIds, tech_array, aspect_ratios, x_locations, y_locations, 
        libraryDicts, blocks, cost_coefficient, power_coefficient);
    
    // Create modified partition assignments
    std::vector<int> modified_partitions = basePartitionIds;
    modified_partitions[blockId] = toPartitionId;
    
    // Calculate new cost
    float new_cost = getCostFromScratch(
        modified_partitions, tech_array, aspect_ratios, x_locations, y_locations, 
        libraryDicts, blocks, cost_coefficient, power_coefficient);
    
    // Return the cost difference
    return new_cost - base_cost;
}

// Get block chiplet names for each partition
std::vector<std::string> getBlockChipletNames(int numPartitions) {
    std::vector<std::string> block_chiplet_names;
    block_chiplet_names.reserve(numPartitions);
    
    for (int i = 0; i < numPartitions; i++) {
        block_chiplet_names.push_back(std::to_string(i));
    }
    
    return block_chiplet_names;
}

// Get block names from the blocks vector
std::vector<std::string> getBlockNames(const std::vector<block>& blocks) {
    std::vector<std::string> block_names;
    block_names.reserve(blocks.size());
    
    for (const auto& block : blocks) {
        block_names.push_back(block.name);
    }
    
    return block_names;
} 