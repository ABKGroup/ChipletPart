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
    
    const size_t num_partitions = block_combinations.size();

    // Map every block index to the partition that contains it. Blocks that are
    // not assigned to any partition (should not happen) map to -1 and are
    // ignored, which matches the old behaviour of silently skipping them.
    std::vector<int> partition_of(block_names.size(), -1);
    for (size_t p = 0; p < num_partitions; ++p) {
        for (int block : block_combinations[p]) {
            if (block >= 0 && static_cast<size_t>(block) < partition_of.size()) {
                partition_of[block] = static_cast<int>(p);
            }
        }
    }

    // Create a combined adjacency matrix for the new block
    for (const auto& [io_type, adjacency_matrix] : global_adjacency_matrix) {
        const auto& bw_matrix = average_bandwidth_utilization.at(io_type);

        // Start from zero: a chiplet pair with no block-level connection must
        // end up with zero IOs. (The previous version initialised with 1.0,
        // which added a phantom connection between every pair of chiplets and
        // biased the bandwidth-weighted averages.)
        std::vector<std::vector<double>> combined_matrix(
            num_partitions, std::vector<double>(num_partitions, 0.0));
        // Running sum of (utilisation * io_count); converted to a weighted
        // average once all block pairs have been accumulated.
        std::vector<std::vector<double>> bw_weighted_sum(
            num_partitions, std::vector<double>(num_partitions, 0.0));

        for (size_t p = 0; p < num_partitions; ++p) {
            for (int block : block_combinations[p]) {
                if (block < 0 || static_cast<size_t>(block) >= adjacency_matrix.size()) {
                    continue;
                }
                const auto& adj_row = adjacency_matrix[block];
                for (size_t other = 0; other < adj_row.size(); ++other) {
                    const int q = other < partition_of.size() ? partition_of[other] : -1;
                    if (q < 0 || static_cast<size_t>(q) == p) {
                        continue; // unassigned block or same chiplet
                    }
                    const double adj_value = adj_row[other];
                    if (adj_value == 0.0) {
                        continue;
                    }
                    double bw_value = 0.0;
                    if (static_cast<size_t>(block) < bw_matrix.size() &&
                        other < bw_matrix[block].size()) {
                        bw_value = bw_matrix[block][other];
                    }
                    combined_matrix[p][q] += adj_value;
                    bw_weighted_sum[p][q] += bw_value * adj_value;
                }
            }
        }

        std::vector<std::vector<double>> combined_ave_bw(
            num_partitions, std::vector<double>(num_partitions, 0.0));
        for (size_t p = 0; p < num_partitions; ++p) {
            for (size_t q = 0; q < num_partitions; ++q) {
                if (combined_matrix[p][q] != 0.0) {
                    combined_ave_bw[p][q] = bw_weighted_sum[p][q] / combined_matrix[p][q];
                }
            }
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