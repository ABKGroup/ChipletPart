#ifndef CONSTRUCT_CHIP_HPP
#define CONSTRUCT_CHIP_HPP

#include "pugixml.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>
#include <map>

namespace construct_chip {

/**
 * @brief Extract the standard technology node from a more complex string format
 * 
 * @param tech_string Technology string (e.g., "BT_TSMC_7nm")
 * @return Extracted technology node (e.g., "7nm")
 */
std::string ExtractTechNode(const std::string& tech_string);

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
    int num_subtrees
);

/**
 * @brief Save an XML document to a file
 * 
 * @param doc XML document to save
 * @param filename Path to the output file
 * @return true if successful, false otherwise
 */
bool SaveElementTree(const pugi::xml_document& doc, const std::string& filename);

/**
 * @brief Create a subtree element with the specified attributes
 * 
 * @param parent Parent XML node
 * @param tech_node Technology node
 * @param aspect_ratio Aspect ratio
 * @param x_location X location
 * @param y_location Y location
 * @param power Power value
 * @param core_area Core area value
 * @param subtree_index Index of the subtree
 * @return XML node representing the subtree
 */
pugi::xml_node CreateSubtreeElement(
    pugi::xml_node& parent,
    const std::string& tech_node,
    double aspect_ratio,
    double x_location,
    double y_location,
    double power,
    double core_area,
    int subtree_index
);

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
    const std::vector<std::vector<int>>& block_combinations
);

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
    int partition_id
);

// Define constants shared with the Python implementation
extern const std::vector<std::string> TECH_NODES;
extern const std::vector<std::string> STACKUP_NAMES;
extern const std::string DEFAULT_QUANTITY;

} // namespace construct_chip

#endif // CONSTRUCT_CHIP_HPP 