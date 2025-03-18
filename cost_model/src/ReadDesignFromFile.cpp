#include "../include/ReadDesignFromFile.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace read_design {

// Helper function to convert string to boolean
bool StrToBool(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return lower_str == "true";
}

// Helper function to convert boolean to boolean
bool StrToBool(bool value) {
    return value;
}

/**
 * @brief Read wafer process definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of WaferProcess objects
 */
std::vector<design::WaferProcess> WaferProcessDefinitionListFromFile(const std::string& filename) {
    // Create XML parser and read the file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());
    
    if (!result) {
        std::cerr << "Error parsing XML file: " << result.description() << std::endl;
        return {};
    }
    
    // Get the root node
    pugi::xml_node root = doc.document_element();
    
    // List of wafer processes
    std::vector<design::WaferProcess> wp_list;
    
    // Iterate over the wafer process definitions
    for (pugi::xml_node wp_def : root.children()) {
        // get the values here first 
        std::string name = wp_def.attribute("name").value();
        double wafer_diameter = std::stod(wp_def.attribute("wafer_diameter").value());
        double edge_exclusion = std::stod(wp_def.attribute("edge_exclusion").value());
        double wafer_process_yield = std::stod(wp_def.attribute("wafer_process_yield").value());
        double dicing_distance = std::stod(wp_def.attribute("dicing_distance").value());
        double reticle_x = std::stod(wp_def.attribute("reticle_x").value());
        double reticle_y = std::stod(wp_def.attribute("reticle_y").value());
        std::string wafer_fill_grid = wp_def.attribute("wafer_fill_grid").value();
        double nre_front_end_cost_per_mm2_memory = std::stod(wp_def.attribute("nre_front_end_cost_per_mm2_memory").value());
        double nre_back_end_cost_per_mm2_memory = std::stod(wp_def.attribute("nre_back_end_cost_per_mm2_memory").value());
        double nre_front_end_cost_per_mm2_logic = std::stod(wp_def.attribute("nre_front_end_cost_per_mm2_logic").value());
        double nre_back_end_cost_per_mm2_logic = std::stod(wp_def.attribute("nre_back_end_cost_per_mm2_logic").value());
        double nre_front_end_cost_per_mm2_analog = std::stod(wp_def.attribute("nre_front_end_cost_per_mm2_analog").value());
        double nre_back_end_cost_per_mm2_analog = std::stod(wp_def.attribute("nre_back_end_cost_per_mm2_analog").value());
        // Create a WaferProcess object
        design::WaferProcess wp(
            name, // name
            wafer_diameter, // wafer_diameter
            edge_exclusion, // edge_exclusion
            wafer_process_yield, // wafer_process_yield
            dicing_distance, // dicing_distance
            reticle_x, // reticle_x
            reticle_y, // reticle_y
            wafer_fill_grid, // wafer_fill_grid
            nre_front_end_cost_per_mm2_memory, // nre_front_end_cost_per_mm2_memory
            nre_back_end_cost_per_mm2_memory, // nre_back_end_cost_per_mm2_memory
            nre_front_end_cost_per_mm2_logic, // nre_front_end_cost_per_mm2_logic
            nre_back_end_cost_per_mm2_logic, // nre_back_end_cost_per_mm2_logic
            nre_front_end_cost_per_mm2_analog, // nre_front_end_cost_per_mm2_analog
            nre_back_end_cost_per_mm2_analog, // nre_back_end_cost_per_mm2_analog
            false // static
        );

        wp.SetStatic();

        // Add to the list
        wp_list.push_back(wp);
    }
    
    return wp_list;
}

/**
 * @brief Read IO definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of IO objects
 */
std::vector<design::IO> IODefinitionListFromFile(const std::string& filename) {
    // Create XML parser and read the file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());
    
    if (!result) {
        std::cerr << "Error parsing XML file: " << result.description() << std::endl;
        return {};
    }
    
    // Get the root node
    pugi::xml_node root = doc.document_element();
    
    // List of IOs
    std::vector<design::IO> io_list;
    
    // Iterate over the IO definitions
    for (pugi::xml_node io_def : root.children()) {
        // get the values here
        std::string type = io_def.attribute("type").value();
        double rx_area = std::stod(io_def.attribute("rx_area").value());
        double tx_area = std::stod(io_def.attribute("tx_area").value());
        double shoreline = std::stod(io_def.attribute("shoreline").value());
        double bandwidth = std::stod(io_def.attribute("bandwidth").value());
        int wire_count = std::stoi(io_def.attribute("wire_count").value());
        std::string bidirectional = io_def.attribute("bidirectional").value();
        double energy_per_bit = std::stod(io_def.attribute("energy_per_bit").value());
        double reach = std::stod(io_def.attribute("reach").value());
        // Create an IO object
        design::IO io(
            type, // type
            rx_area, // rx_area
            tx_area, // tx_area
            shoreline, // shoreline
            bandwidth, // bandwidth
            wire_count, // wire_count
            bidirectional, // bidirectional
            energy_per_bit, // energy_per_bit
            reach, // reach
            false // static
        );
        
        io.SetStatic();

        // Add to the list
        io_list.push_back(io);
    }
    
    return io_list;   
}

/**
 * @brief Read layer definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of Layer objects
 */
std::vector<design::Layer> LayerDefinitionListFromFile(const std::string& filename) {
    // Create XML parser and read the file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());
    
    if (!result) {
        std::cerr << "Error parsing XML file: " << result.description() << std::endl;
        return {};
    }
    
    // Get the root node
    pugi::xml_node root = doc.document_element();
    
    // List of layers
    std::vector<design::Layer> layer_list;
    
    // Iterate over the layer definitions
    for (pugi::xml_node layer_def : root.children()) {
        // get values here 
        std::string name = layer_def.attribute("name").value();
        std::string active = layer_def.attribute("active").value();
        float cost_per_mm2 = std::stof(layer_def.attribute("cost_per_mm2").value());
        float transistor_density = std::stof(layer_def.attribute("transistor_density").value());
        float defect_density = std::stof(layer_def.attribute("defect_density").value());
        float critical_area_ratio = std::stof(layer_def.attribute("critical_area_ratio").value());
        float clustering_factor = std::stof(layer_def.attribute("clustering_factor").value());
        float litho_percent = std::stof(layer_def.attribute("litho_percent").value());
        float mask_cost = std::stof(layer_def.attribute("nre_mask_cost").value());
        float stitching_yield = std::stof(layer_def.attribute("stitching_yield").value());
        // Create a Layer object
        design::Layer layer(
            name, // name
            active, // active
            cost_per_mm2, // cost_per_mm2
            transistor_density, // transistor_density
            defect_density, // defect_density
            critical_area_ratio, // critical_area_ratio
            clustering_factor, // clustering_factor
            litho_percent, // litho_percent
            mask_cost, // mask_cost
            stitching_yield, // stitching_yield
            false // static
        );

        layer.SetStatic();
        
        // Add to the list
        layer_list.push_back(layer);
    }
    
    return layer_list;
}

/**
 * @brief Read assembly process definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of Assembly objects
 */
std::vector<design::Assembly> AssemblyProcessDefinitionListFromFile(const std::string& filename) {
    // Create XML parser and read the file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());
    
    if (!result) {
        std::cerr << "Error parsing XML file: " << result.description() << std::endl;
        return {};
    }
    
    // Get the root node
    pugi::xml_node root = doc.document_element();
    
    // List of assembly processes
    std::vector<design::Assembly> assembly_process_list;
    
    // Iterate over the assembly process definitions
    for (pugi::xml_node assembly_process_def : root.children()) {
        // get the values here 
        std::string name = assembly_process_def.attribute("name").value();
        float materials_cost_per_mm2 = std::stof(assembly_process_def.attribute("materials_cost_per_mm2").value());
        // Handle bb_cost_per_second which can be null
        std::string bb_cost_str = assembly_process_def.attribute("bb_cost_per_second").value();
        float bb_cost_per_second = bb_cost_str.empty() ? -1.0 : std::stof(bb_cost_str);
        float picknplace_machine_cost = std::stof(assembly_process_def.attribute("picknplace_machine_cost").value());
        float picknplace_machine_lifetime = std::stof(assembly_process_def.attribute("picknplace_machine_lifetime").value());
        float picknplace_machine_uptime = std::stof(assembly_process_def.attribute("picknplace_machine_uptime").value());
        float picknplace_technician_yearly_cost = std::stof(assembly_process_def.attribute("picknplace_technician_yearly_cost").value());
        float picknplace_time = std::stof(assembly_process_def.attribute("picknplace_time").value());
        int picknplace_group = std::stoi(assembly_process_def.attribute("picknplace_group").value());
        float bonding_machine_cost = std::stof(assembly_process_def.attribute("bonding_machine_cost").value());
        float bonding_machine_lifetime = std::stof(assembly_process_def.attribute("bonding_machine_lifetime").value());
        float bonding_machine_uptime = std::stof(assembly_process_def.attribute("bonding_machine_uptime").value());
        float bonding_technician_yearly_cost = std::stof(assembly_process_def.attribute("bonding_technician_yearly_cost").value());
        float bonding_time = std::stof(assembly_process_def.attribute("bonding_time").value());
        int bonding_group = std::stoi(assembly_process_def.attribute("bonding_group").value());
        float die_separation = std::stof(assembly_process_def.attribute("die_separation").value());
        float edge_exclusion = std::stof(assembly_process_def.attribute("edge_exclusion").value());
        float bonding_pitch = std::stof(assembly_process_def.attribute("bonding_pitch").value());
        float max_pad_current_density = std::stof(assembly_process_def.attribute("max_pad_current_density").value());
        float alignment_yield = std::stof(assembly_process_def.attribute("alignment_yield").value());
        float bonding_yield = std::stof(assembly_process_def.attribute("bonding_yield").value());
        float dielectric_bond_defect_density = std::stof(assembly_process_def.attribute("dielectric_bond_defect_density").value());
        
        // Create an Assembly object with default parameters
        design::Assembly assembly_process(
            name, // name
            materials_cost_per_mm2, // materials_cost_per_mm2
            bb_cost_per_second, // bb_cost_per_second
            picknplace_machine_cost, // picknplace_machine_cost
            picknplace_machine_lifetime, // picknplace_machine_lifetime
            picknplace_machine_uptime, // picknplace_machine_uptime
            picknplace_technician_yearly_cost, // picknplace_technician_yearly_cost
            picknplace_time, // picknplace_time
            picknplace_group, // picknplace_group
            bonding_machine_cost, // bonding_machine_cost
            bonding_machine_lifetime, // bonding_machine_lifetime
            bonding_machine_uptime, // bonding_machine_uptime
            bonding_technician_yearly_cost, // bonding_technician_yearly_cost
            bonding_time, // bonding_time
            bonding_group, // bonding_group
            die_separation, // die_separation
            edge_exclusion, // edge_exclusion
            max_pad_current_density, // max_pad_current_density
            bonding_pitch, // bonding_pitch
            alignment_yield, // alignment_yield
            bonding_yield, // bonding_yield
            dielectric_bond_defect_density, // dielectric_bond_defect_density
            false // static
        );
        
        // Compute costs per second
        assembly_process.ComputePicknplaceCostPerSecond();
        assembly_process.ComputeBondingCostPerSecond();        
        assembly_process.SetStatic();
        
        // Add to the list
        assembly_process_list.push_back(assembly_process);
    }
    
    return assembly_process_list;
}

/**
 * @brief Read test process definitions from file
 * 
 * @param filename Path to the XML file
 * @return Vector of Test objects
 */
std::vector<design::Test> TestProcessDefinitionListFromFile(const std::string& filename) {
    // Create XML parser and read the file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());
    
    if (!result) {
        std::cerr << "Error parsing XML file: " << result.description() << std::endl;
        return {};
    }
    
    // Get the root node
    pugi::xml_node root = doc.document_element();
    
    // List of test processes
    std::vector<design::Test> test_process_list;
    
    // Iterate over the test process definitions
    for (pugi::xml_node test_process_def : root.children()) {
        // get values here 
        std::string name = test_process_def.attribute("name").value();
        double time_per_test_cycle = std::stod(test_process_def.attribute("time_per_test_cycle").value());
        double cost_per_second = std::stod(test_process_def.attribute("cost_per_second").value());
        int samples_per_input = std::stoi(test_process_def.attribute("samples_per_input").value());
        std::string test_self_str = test_process_def.attribute("test_self").value();
        std::string test_self = test_self_str.empty() ? "False" : test_self_str;
        std::string bb_self_pattern_count_str = test_process_def.attribute("bb_self_pattern_count").value();
        double bb_self_pattern_count = bb_self_pattern_count_str.empty() ? 0.0 : std::stod(bb_self_pattern_count_str);
        std::string bb_self_scan_chain_length_str = test_process_def.attribute("bb_self_scan_chain_length").value();
        double bb_self_scan_chain_length = bb_self_scan_chain_length_str.empty() ? 0.0 : std::stod(bb_self_scan_chain_length_str);
        double self_defect_coverage = std::stod(test_process_def.attribute("self_defect_coverage").value());
        double self_test_reuse = std::stod(test_process_def.attribute("self_test_reuse").value());
        int self_num_scan_chains = std::stoi(test_process_def.attribute("self_num_scan_chains").value());
        int self_num_io_per_scan_chain = std::stoi(test_process_def.attribute("self_num_io_per_scan_chain").value());  
        int self_num_test_io_offset = std::stoi(test_process_def.attribute("self_num_test_io_offset").value());
        std::string self_test_failure_dist_str = test_process_def.attribute("self_test_failure_dist").value();
        std::string self_test_failure_dist = self_test_failure_dist_str.empty() ? "normal" : self_test_failure_dist_str;
        std::string test_assembly_str = test_process_def.attribute("test_assembly").value();
        std::string test_assembly = test_assembly_str.empty() ? "False" : test_assembly_str;
        std::string bb_assembly_pattern_count_str = test_process_def.attribute("bb_assembly_pattern_count").value();
        double bb_assembly_pattern_count = bb_assembly_pattern_count_str.empty() ? 0.0 : std::stod(bb_assembly_pattern_count_str);
        std::string bb_assembly_scan_chain_length_str = test_process_def.attribute("bb_assembly_scan_chain_length").value();
        double bb_assembly_scan_chain_length = bb_assembly_scan_chain_length_str.empty() ? 0.0 : std::stod(bb_assembly_scan_chain_length_str);
        double assembly_defect_coverage = std::stod(test_process_def.attribute("assembly_defect_coverage").value());
        double assembly_test_reuse = std::stod(test_process_def.attribute("assembly_test_reuse").value());
        int assembly_num_scan_chains = std::stoi(test_process_def.attribute("assembly_num_scan_chains").value());
        int assembly_num_io_per_scan_chain = std::stoi(test_process_def.attribute("assembly_num_io_per_scan_chain").value());
        int assembly_num_test_io_offset = std::stoi(test_process_def.attribute("assembly_num_test_io_offset").value());
        std::string assembly_test_failure_dist_str = test_process_def.attribute("assembly_test_failure_dist").value();
        std::string assembly_test_failure_dist = assembly_test_failure_dist_str.empty() ? "normal" : assembly_test_failure_dist_str;

        // Create a Test object with default parameters
        design::Test test_process(
            name, // name
            time_per_test_cycle, // time_per_test_cycle
            cost_per_second, // cost_per_second
            samples_per_input, // samples_per_input
            test_self, // test_self
            bb_self_pattern_count, // bb_self_pattern_count
            bb_self_scan_chain_length, // bb_self_scan_chain_length
            self_defect_coverage, // self_defect_coverage
            self_test_reuse, // self_test_reuse
            self_num_scan_chains, // self_num_scan_chains
            self_num_io_per_scan_chain, // self_num_io_per_scan_chain
            self_num_test_io_offset, // self_num_test_io_offset
            self_test_failure_dist, // self_test_failure_dist
            test_assembly, // test_assembly
            bb_assembly_pattern_count, // bb_assembly_pattern_count
            bb_assembly_scan_chain_length, // bb_assembly_scan_chain_length
            assembly_defect_coverage, // assembly_defect_coverage
            assembly_test_reuse, // assembly_test_reuse
            assembly_num_scan_chains, // assembly_num_scan_chains
            assembly_num_io_per_scan_chain, // assembly_num_io_per_scan_chain
            assembly_num_test_io_offset, // assembly_num_test_io_offset
            assembly_test_failure_dist, // assembly_test_failure_dist
            false // static
        );
        
        test_process.SetStatic(true);
        
        // Add to the list
        test_process_list.push_back(test_process);
    }

    return test_process_list;
}

/**
 * @brief Construct a global adjacency matrix from the netlist file (exactly replicating Python implementation)
 * 
 * @param filename Path to the netlist XML file
 * @param io_list List of available IOs
 * @return Tuple containing the global adjacency matrix, average bandwidth utilization, and block names
 */
std::tuple<
    std::map<std::string, std::vector<std::vector<int>>>,
    std::map<std::string, std::vector<std::vector<double>>>,
    std::vector<std::string>
> GlobalAdjacencyMatrixFromFile(const std::string& filename, const std::vector<design::IO>& io_list) {
    // Load XML document
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());
    
    if (!result) {
        std::cerr << "Error parsing XML file: " << result.description() << std::endl;
        return std::make_tuple(
            std::map<std::string, std::vector<std::vector<int>>>(),
            std::map<std::string, std::vector<std::vector<double>>>(),
            std::vector<std::string>()
        );
    }
    
    // Get root element
    pugi::xml_node root = doc.document_element();
    
    // Initialize empty block_names list - exactly as in Python
    std::vector<std::string> block_names;
    
    // Initialize empty dictionaries for matrices - exactly as in Python
    std::map<std::string, std::vector<std::vector<int>>> global_adjacency_matrix;
    std::map<std::string, std::vector<std::vector<double>>> average_bandwidth_utilization;
    
    // Iterate over each net definition, just like Python version
    for (pugi::xml_node net_def : root.children()) {
        double link_average_bandwidth_utilization = 0.0;
        try {
            link_average_bandwidth_utilization = std::stod(net_def.attribute("average_bandwidth_utilization").value());
        } catch (const std::exception&) {
            // Default to 0.0 if attribute is missing or invalid
        }
        
        std::string link_type = net_def.attribute("type").value();
        if (link_type.empty()) {
            continue;  // Skip if no type specified
        }
        
        // Check if the type of net is already a key in the dictionary
        if (global_adjacency_matrix.find(link_type) == global_adjacency_matrix.end()) {
            // Add new net type with empty matrix
            global_adjacency_matrix[link_type] = std::vector<std::vector<int>>(
                block_names.size(), std::vector<int>(block_names.size(), 0));
            average_bandwidth_utilization[link_type] = std::vector<std::vector<double>>(
                block_names.size(), std::vector<double>(block_names.size(), 0.0));
        }
        
        // Dynamically add blocks and resize matrices, exactly as in Python
        std::string block0 = net_def.attribute("block0").value();
        std::string block1 = net_def.attribute("block1").value();
        
        if (block0.empty() || block1.empty()) {
            continue;  // Skip if blocks not specified
        }
        
        // If block0 is not in the list of block names, add it
        if (std::find(block_names.begin(), block_names.end(), block0) == block_names.end()) {
            block_names.push_back(block0);
            // Add a row and column to each matrix
            for (auto& matrix_pair : global_adjacency_matrix) {
                auto& matrix = matrix_pair.second;
                size_t old_size = matrix.size();
                // If matrix was empty, initialize with 1x1
                if (old_size == 0) {
                    matrix = std::vector<std::vector<int>>(1, std::vector<int>(1, 0));
                } else {
                    // Add a new row
                    matrix.push_back(std::vector<int>(old_size, 0));
                    // Add a new column to each row
                    for (auto& row : matrix) {
                        row.push_back(0);
                    }
                }
            }
            for (auto& matrix_pair : average_bandwidth_utilization) {
                auto& matrix = matrix_pair.second;
                size_t old_size = matrix.size();
                // If matrix was empty, initialize with 1x1
                if (old_size == 0) {
                    matrix = std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0));
                } else {
                    // Add a new row
                    matrix.push_back(std::vector<double>(old_size, 0.0));
                    // Add a new column to each row
                    for (auto& row : matrix) {
                        row.push_back(0.0);
                    }
                }
            }
        }
        
        // If block1 is not in the list of block names, add it
        if (std::find(block_names.begin(), block_names.end(), block1) == block_names.end()) {
            block_names.push_back(block1);
            // Add a row and column to each matrix
            for (auto& matrix_pair : global_adjacency_matrix) {
                auto& matrix = matrix_pair.second;
                size_t old_size = matrix.size();
                // If matrix was empty, initialize with 1x1
                if (old_size == 0) {
                    matrix = std::vector<std::vector<int>>(1, std::vector<int>(1, 0));
                } else {
                    // Add a new row
                    matrix.push_back(std::vector<int>(old_size, 0));
                    // Add a new column to each row
                    for (auto& row : matrix) {
                        row.push_back(0);
                    }
                }
            }
            for (auto& matrix_pair : average_bandwidth_utilization) {
                auto& matrix = matrix_pair.second;
                size_t old_size = matrix.size();
                // If matrix was empty, initialize with 1x1
                if (old_size == 0) {
                    matrix = std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0));
                } else {
                    // Add a new row
                    matrix.push_back(std::vector<double>(old_size, 0.0));
                    // Add a new column to each row
                    for (auto& row : matrix) {
                        row.push_back(0.0);
                    }
                }
            }
        }
        
        // Find the IO bandwidth and bidirectional properties
        double io_bandwidth = 0.0;
        bool bidirectional = false;
        
        for (const auto& io : io_list) {
            if (link_type == io.GetType()) {
                io_bandwidth = io.GetBandwidth();
                bidirectional = io.GetBidirectional(); // GetBidirectional() returns a bool (BoolType)
                break;
            }
        }
        
        if (io_bandwidth == 0.0) {
            std::cerr << "Warning: Net type " << link_type << " not found in io_list." << std::endl;
            continue;
        }
        
        // Find the indices of the blocks
        int block1_index = std::distance(block_names.begin(), 
                                         std::find(block_names.begin(), block_names.end(), block0));
        int block2_index = std::distance(block_names.begin(), 
                                         std::find(block_names.begin(), block_names.end(), block1));
        
        // Calculate ios_to_add and peak_utilization_factor
        int ios_to_add;
        std::string bb_count = net_def.attribute("bb_count").value();
        if (bb_count.empty()) {
            double bandwidth = 0.0;
            try {
                bandwidth = std::stod(net_def.attribute("bandwidth").value());
            } catch (const std::exception&) {
                // Default to 0.0 if attribute is missing or invalid
            }
            ios_to_add = static_cast<int>(std::ceil(bandwidth / io_bandwidth));
        } else {
            try {
                ios_to_add = std::stoi(bb_count);
            } catch (const std::exception&) {
                ios_to_add = 0;
            }
        }
        
        if (ios_to_add <= 0) {
            continue;  // Skip if no IOs to add
        }
        
        double peak_utilization_factor = 1.0;
        try {
            double bandwidth = std::stod(net_def.attribute("bandwidth").value());
            if (ios_to_add > 0) {
                peak_utilization_factor = bandwidth / (ios_to_add * io_bandwidth);
            }
        } catch (const std::exception&) {
            // Keep default 1.0 if attribute is missing or invalid
        }
        
        link_average_bandwidth_utilization *= peak_utilization_factor;
        
        // Update the matrices, exactly matching Python logic
        if (!bidirectional) {
            if (average_bandwidth_utilization[link_type][block1_index][block2_index] == 0) {
                average_bandwidth_utilization[link_type][block1_index][block2_index] = link_average_bandwidth_utilization;
            } else {
                average_bandwidth_utilization[link_type][block1_index][block2_index] = 
                    (average_bandwidth_utilization[link_type][block1_index][block2_index] * 
                    global_adjacency_matrix[link_type][block1_index][block2_index] + 
                    link_average_bandwidth_utilization * ios_to_add) / 
                    (global_adjacency_matrix[link_type][block1_index][block2_index] + ios_to_add);
            }
            global_adjacency_matrix[link_type][block1_index][block2_index] += ios_to_add;
        } else {
            // Handle bidirectional case (block1 to block2)
            if (average_bandwidth_utilization[link_type][block1_index][block2_index] == 0) {
                average_bandwidth_utilization[link_type][block1_index][block2_index] = link_average_bandwidth_utilization;
            } else {
                average_bandwidth_utilization[link_type][block1_index][block2_index] = 
                    (average_bandwidth_utilization[link_type][block1_index][block2_index] * 
                    global_adjacency_matrix[link_type][block1_index][block2_index] + 
                    link_average_bandwidth_utilization * ios_to_add) / 
                    (global_adjacency_matrix[link_type][block1_index][block2_index] + ios_to_add);
            }
            global_adjacency_matrix[link_type][block1_index][block2_index] += ios_to_add;
            
            // Handle bidirectional case (block2 to block1)
            if (average_bandwidth_utilization[link_type][block2_index][block1_index] == 0) {
                average_bandwidth_utilization[link_type][block2_index][block1_index] = link_average_bandwidth_utilization;
            } else {
                average_bandwidth_utilization[link_type][block2_index][block1_index] = 
                    (average_bandwidth_utilization[link_type][block2_index][block1_index] * 
                    global_adjacency_matrix[link_type][block2_index][block1_index] + 
                    link_average_bandwidth_utilization * ios_to_add) / 
                    (global_adjacency_matrix[link_type][block2_index][block1_index] + ios_to_add);
            }
            global_adjacency_matrix[link_type][block2_index][block1_index] += ios_to_add;
        }
    }
    
    return std::make_tuple(global_adjacency_matrix, average_bandwidth_utilization, block_names);
}

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
 * @param approx_state Approximation state
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
    const bool approx_state) {
    
    // Create a new chip using the factory method
    std::shared_ptr<design::Chip> chip = design::Chip::Create(
        "", // filename
        &etree, // etree
        nullptr, // parent_chip
        &wafer_process_list, // wafer_process_list
        &assembly_process_list, // assembly_process_list
        &test_process_list, // test_process_list
        &layer_list, // layers
        &io_list, // ios
        &global_adjacency_matrix, // adjacency_matrix_definitions
        &average_bandwidth_utilization, // average_bandwidth_utilization
        &block_names, // block_names
        false, // static_value - should be false by default
        approx_state // approx_state - this is now correctly passed as approx_state
    );
    
    //chip->Initialize();

    return chip;
}

} // namespace read_design 