#include "../include/ConstructChip.hpp"
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <tuple>
#include <iostream>

namespace construct_chip {

// Global variables matching the Python implementation
const std::vector<std::string> tech_nodes = {"45nm", "16nm", "14nm", "10nm", "7nm"};
const std::vector<std::string> stackup_names = {"1:combined_45nm", "1:combined_12nm", "1:combined_12nm", "1:combined_10nm", "1:combined_7nm"};
const std::string quantity = "100000000000";

/**
 * @brief Extract the standard technology node from a more complex string format
 * 
 * @param tech_string Technology string (e.g., "BT_TSMC_7nm")
 * @return Extracted technology node (e.g., "7nm")
 */
std::string ExtractTechNode(const std::string& tech_string) {
    // If "TSMC" is in the string, extract just the technology node part
    if (tech_string.find("TSMC") != std::string::npos) {
        // Extract the part after the last underscore (e.g., "7nm" from "BT_TSMC_7nm")
        size_t last_underscore = tech_string.find_last_of('_');
        if (last_underscore != std::string::npos) {
            return tech_string.substr(last_underscore + 1);
        }
    }
    // Otherwise return the original string
    return tech_string;
}

/**
 * @brief Create an XML element tree representing a chip
 * 
 * @param subtree_tech_nodes Vector of technology nodes for subtrees
 * @param subtree_aspect_ratios Vector of aspect ratios for subtrees
 * @param subtree_x_locations Vector of x locations for subtrees
 * @param subtree_y_locations Vector of y locations for subtrees
 * @param subtree_power Vector of power values for subtrees
 * @param subtree_core_area Vector of core area values for subtrees
 * @param num_subtrees Number of subtrees
 * @return XML document representing the chip
 */
pugi::xml_document CreateElementTree(
    const std::vector<std::string>& subtree_tech_nodes,
    const std::vector<double>& subtree_aspect_ratios,
    const std::vector<double>& subtree_x_locations,
    const std::vector<double>& subtree_y_locations,
    const std::vector<double>& subtree_power,
    const std::vector<double>& subtree_core_area,
    int num_subtrees)
{
    // Create an XML document
    pugi::xml_document doc;
    
    // Create the root element with attributes
    pugi::xml_node root = doc.append_child("chip");
    root.append_attribute("name") = "interposer";
    root.append_attribute("bb_area") = "";
    root.append_attribute("bb_cost") = "";
    root.append_attribute("bb_power") = "";
    root.append_attribute("bb_quality") = "";
    root.append_attribute("aspect_ratio") = "";
    root.append_attribute("x_location") = "";
    root.append_attribute("y_location") = "";
    root.append_attribute("fraction_memory") = "0.0";
    root.append_attribute("fraction_logic") = "0.0";
    root.append_attribute("fraction_analog") = "1.0";
    root.append_attribute("gate_flop_ratio") = "1.0";
    root.append_attribute("reticle_share") = "1.0";
    root.append_attribute("buried") = "False";
    root.append_attribute("assembly_process") = "silicon_individual_bonding";
    root.append_attribute("test_process") = "KGD_free_test";
    root.append_attribute("stackup") = "1:combined_interposer_silicon";
    root.append_attribute("wafer_process") = "process_1";
    root.append_attribute("v_rail") = "5";
    root.append_attribute("reg_eff") = "1.0";
    root.append_attribute("reg_type") = "none";
    root.append_attribute("core_voltage") = "1.0";
    root.append_attribute("quantity") = quantity.c_str();
    root.append_attribute("core_area") = "0.0";  // Set the "core_area" attribute for the root
    root.append_attribute("power") = "0.0";      // Set the "power" attribute for the root

    // Create the specified number of subtrees with the same attributes
    for (int i = 0; i < num_subtrees; i++) {
        pugi::xml_node subtree = root.append_child("chip");
        subtree.append_attribute("name") = std::to_string(i).c_str();
        subtree.append_attribute("bb_area") = "";
        subtree.append_attribute("bb_cost") = "";
        subtree.append_attribute("bb_power") = "";
        subtree.append_attribute("bb_quality") = "";
        
        // Handle aspect ratio
        if (i < subtree_aspect_ratios.size() && subtree_aspect_ratios[i] != 0.0) {
            subtree.append_attribute("aspect_ratio") = std::to_string(subtree_aspect_ratios[i]).c_str();
        } else {
            subtree.append_attribute("aspect_ratio") = "";
        }
        
        // Handle x location
        if (i < subtree_x_locations.size() && subtree_x_locations[i] != 0.0) {
            subtree.append_attribute("x_location") = std::to_string(subtree_x_locations[i]).c_str();
        } else {
            subtree.append_attribute("x_location") = "";
        }
        
        // Handle y location
        if (i < subtree_y_locations.size() && subtree_y_locations[i] != 0.0) {
            subtree.append_attribute("y_location") = std::to_string(subtree_y_locations[i]).c_str();
        } else {
            subtree.append_attribute("y_location") = "";
        }
        
        subtree.append_attribute("fraction_memory") = "0.0";
        subtree.append_attribute("fraction_logic") = "1.0";
        subtree.append_attribute("fraction_analog") = "0.0";
        subtree.append_attribute("gate_flop_ratio") = "1.0";
        subtree.append_attribute("reticle_share") = "1.0";
        subtree.append_attribute("buried") = "False";
        subtree.append_attribute("assembly_process") = "silicon_individual_bonding";
        subtree.append_attribute("test_process") = "KGD_free_test";
        
        // Find the technology node in the global tech_nodes list and set the stackup
        if (i < subtree_tech_nodes.size()) {
            auto it = std::find(tech_nodes.begin(), tech_nodes.end(), subtree_tech_nodes[i]);
            if (it != tech_nodes.end()) {
                // Get the index of the tech node
                size_t index = std::distance(tech_nodes.begin(), it);
                // Use the corresponding stackup name
                if (index < stackup_names.size()) {
                    subtree.append_attribute("stackup") = stackup_names[index].c_str();
                }
            }
        }
        
        subtree.append_attribute("wafer_process") = "process_1";
        subtree.append_attribute("v_rail") = "5";
        subtree.append_attribute("reg_eff") = "1.0";
        subtree.append_attribute("reg_type") = "none";
        subtree.append_attribute("core_voltage") = "1.0";
        subtree.append_attribute("quantity") = quantity.c_str();
        
        // Set core_area and power attributes from the input lists
        if (i < subtree_core_area.size()) {
            subtree.append_attribute("core_area") = std::to_string(subtree_core_area[i]).c_str();
        }
        
        if (i < subtree_power.size()) {
            subtree.append_attribute("power") = std::to_string(subtree_power[i]).c_str();
        }
    }
    
    return doc;
}

/**
 * @brief Combine blocks to create a new adjacency matrix
 * 
 * @param global_adjacency_matrix Original adjacency matrix
 * @param average_bandwidth_utilization Average bandwidth utilization
 * @param block_names Names of blocks
 * @param block_combinations Vector of vectors representing block combinations
 * @return Tuple of combined adjacency matrix, combined average bandwidth utilization, and block names
 */
std::tuple<
    std::map<std::string, std::vector<std::vector<double>>>,
    std::map<std::string, std::vector<std::vector<double>>>,
    std::vector<std::string>
> CombineBlocks(
    const std::map<std::string, std::vector<std::vector<double>>>& global_adjacency_matrix,
    const std::map<std::string, std::vector<std::vector<double>>>& average_bandwidth_utilization,
    const std::vector<std::string>& block_names,
    const std::vector<std::vector<int>>& block_combinations)
{
    // Create maps to store the combined adjacency matrices
    std::map<std::string, std::vector<std::vector<double>>> combined_adjacency_matrix;
    std::map<std::string, std::vector<std::vector<double>>> combined_average_bandwidth_utilization;
    
    // Create a combined adjacency matrix for the new block
    for (const auto& [io_type, adjacency_matrix] : global_adjacency_matrix) {
        // Initialize the combined matrices with ones, matching numpy's behavior
        std::vector<std::vector<double>> combined_matrix(
            block_combinations.size(), 
            std::vector<double>(block_combinations.size(), 1.0)
        );
        
        std::vector<std::vector<double>> combined_ave_bw(
            block_combinations.size(), 
            std::vector<double>(block_combinations.size(), 1.0)
        );
        
        // Process each partition
        int partition_number = 0;
        for (const auto& partition : block_combinations) {
            for (int block : partition) {
                for (const std::string& block_name : block_names) {
                    // Get the index of block_name in block_names
                    auto it = std::find(block_names.begin(), block_names.end(), block_name);
                    if (it != block_names.end()) {
                        int index_block_name = std::distance(block_names.begin(), it);
                        
                        // Check if index_block_name is not in current partition
                        if (std::find(partition.begin(), partition.end(), index_block_name) == partition.end()) {
                            // Find which partition block_number is in
                            int block_number = index_block_name;
                            int other_partition_number = 0;
                            
                            for (const auto& other_partition : block_combinations) {
                                if (std::find(other_partition.begin(), other_partition.end(), block_number) != other_partition.end()) {
                                    // Get the values from the original matrices
                                    double adj_value = 0.0;
                                    double bw_value = 0.0;
                                    if (block < adjacency_matrix.size() && block_number < adjacency_matrix[block].size()) {
                                        adj_value = adjacency_matrix[block][block_number];
                                    }
                                    
                                    const auto& bw_matrix = average_bandwidth_utilization.at(io_type);
                                    if (block < bw_matrix.size() && block_number < bw_matrix[block].size()) {
                                        bw_value = bw_matrix[block][block_number];
                                    }
                                    
                                    if (combined_matrix[partition_number][other_partition_number] == 0) {
                                        combined_ave_bw[partition_number][other_partition_number] = bw_value;
                                        combined_matrix[partition_number][other_partition_number] = adj_value;
                                    } else {
                                        double old_ave_bw_scaled = combined_ave_bw[partition_number][other_partition_number] * 
                                                                  combined_matrix[partition_number][other_partition_number];
                                        double new_ave_bw_scaled = bw_value * adj_value;
                                        
                                        combined_matrix[partition_number][other_partition_number] += adj_value;
                                        
                                        // Avoid division by zero
                                        if (combined_matrix[partition_number][other_partition_number] != 0) {
                                            combined_ave_bw[partition_number][other_partition_number] = 
                                                (old_ave_bw_scaled + new_ave_bw_scaled) / 
                                                combined_matrix[partition_number][other_partition_number];
                                        }
                                    }
                                }
                                other_partition_number++;
                            }
                        }
                    }
                }
            }
            partition_number++;
        }
        
        // set diagonal elements to 0
        for (int i = 0; i < combined_matrix.size(); i++) {
            combined_matrix[i][i] = 0;
            combined_ave_bw[i][i] = 0;
        }
        combined_adjacency_matrix[io_type] = combined_matrix;
        combined_average_bandwidth_utilization[io_type] = combined_ave_bw;
    }

    return std::make_tuple(combined_adjacency_matrix, combined_average_bandwidth_utilization, block_names);
}

/**
 * @brief Increment the netlist for a specific partition
 * 
 * @param connectivity Connectivity matrix
 * @param bandwidth_change_for_slope Change in bandwidth for slope calculation
 * @param partition_id Partition ID to increment
 * @return Updated connectivity matrix
 */
std::map<std::string, std::vector<std::vector<double>>> IncrementNetlist(
    const std::map<std::string, std::vector<std::vector<double>>>& connectivity,
    double bandwidth_change_for_slope,
    int partition_id)
{
    // Create a copy of the connectivity matrix that we can modify
    std::map<std::string, std::vector<std::vector<double>>> updated_connectivity = connectivity;
    
    // Increment the netlist for the partition_id
    for (auto& [io_type, connectivity_matrix] : updated_connectivity) {
        // Make sure the partition_id is within bounds
        if (partition_id >= 0 && 
            partition_id < connectivity_matrix.size() && 
            partition_id < connectivity_matrix[partition_id].size()) {
            
            // Increment the diagonal element corresponding to the partition_id
            connectivity_matrix[partition_id][partition_id] += bandwidth_change_for_slope;
        }
    }
    
    return updated_connectivity;
}

} // namespace construct_chip 