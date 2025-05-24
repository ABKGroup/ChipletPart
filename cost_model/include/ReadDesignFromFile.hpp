#ifndef READ_DESIGN_FROM_FILE_HPP
#define READ_DESIGN_FROM_FILE_HPP

#include "design/Design.hpp"
#include "pugixml.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

namespace read_design {

/**
 * @brief Read wafer process definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of WaferProcess objects
 */
std::vector<design::WaferProcess> WaferProcessDefinitionListFromFile(const std::string& filename);

/**
 * @brief Read IO definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of IO objects
 */
std::vector<design::IO> IODefinitionListFromFile(const std::string& filename);

/**
 * @brief Read layer definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of Layer objects
 */
std::vector<design::Layer> LayerDefinitionListFromFile(const std::string& filename);

/**
 * @brief Read assembly process definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of Assembly objects
 */
std::vector<design::Assembly> AssemblyProcessDefinitionListFromFile(const std::string& filename);

/**
 * @brief Read test process definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of Test objects
 */
std::vector<design::Test> TestProcessDefinitionListFromFile(const std::string& filename);

/**
 * @brief Build adjacency matrix from netlist
 * 
 * @param filename Path to the XML file
 * @param io_list List of IO objects
 * @return Tuple containing the adjacency matrix, bandwidth utilization, and block names
 */
std::tuple<
    std::map<std::string, std::vector<std::vector<int>>>,
    std::map<std::string, std::vector<std::vector<double>>>,
    std::vector<std::string>
> GlobalAdjacencyMatrixFromFile(const std::string& filename, const std::vector<design::IO>& io_list);

/**
 * @brief Read chip design from file
 * 
 * @param chip_design_filename Path to chip design XML file
 * @param wafer_process_list List of available wafer processes
 * @param io_list List of available IOs
 * @param layer_list List of available layers
 * @param assembly_process_list List of available assembly processes
 * @param test_process_list List of available test processes
 * @return Chip object
 */
std::shared_ptr<design::Chip> ReadChipDesignFromFile(
    const std::string& chip_design_filename,
    const std::vector<design::WaferProcess>& wafer_process_list,
    const std::vector<design::IO>& io_list,
    const std::vector<design::Layer>& layer_list,
    const std::vector<design::Assembly>& assembly_process_list,
    const std::vector<design::Test>& test_process_list
);

/**
 * @brief Create a chip from a chip XML node
 * 
 * @param etree XML node containing chip definition
 * @param wafer_process_list List of available wafer processes
 * @param assembly_process_list List of available assembly processes
 * @param test_process_list List of available test processes
 * @param layer_list List of available layers
 * @param io_list List of available IOs
 * @param global_adjacency_matrix Global adjacency matrix
 * @param average_bandwidth_utilization Average bandwidth utilization matrix
 * @param block_names List of block names
 * @return std::shared_ptr<design::Chip> Created chip
 */
std::shared_ptr<design::Chip> CreateChipFromXML(
    pugi::xml_node etree,
    const std::vector<design::WaferProcess>& wafer_process_list,
    const std::vector<design::Assembly>& assembly_process_list,
    const std::vector<design::Test>& test_process_list,
    const std::vector<design::Layer>& layer_list,
    const std::vector<design::IO>& io_list,
    const std::map<std::string, std::vector<std::vector<int>>>& global_adjacency_matrix,
    const std::map<std::string, std::vector<std::vector<double>>>& average_bandwidth_utilization,
    const std::vector<std::string>& block_names,
    const bool approx_static = false
);

} // namespace read_design

#endif // READ_DESIGN_FROM_FILE_HPP 