#include "design/Chip.hpp"
#include "design/Layer.hpp"
#include "design/WaferProcess.hpp"
#include "design/Assembly.hpp"
#include "design/Test.hpp"
#include "design/IO.hpp"
#include <iostream>
#include <memory>
#include <map>
#include <vector>

using namespace design;

int main() {
    // Create a wafer process
    auto wafer_process = std::make_shared<WaferProcess>(
        "Example Wafer Process",  // name
        300.0,                    // wafer_diameter
        3.0,                      // edge_exclusion
        0.9,                      // wafer_process_yield
        0.1,                      // dicing_distance
        26.0,                     // reticle_x
        32.0,                     // reticle_y
        "True",                   // wafer_fill_grid
        1000.0,                   // nre_front_end_cost_per_mm2_memory
        2000.0,                   // nre_back_end_cost_per_mm2_memory
        1500.0,                   // nre_front_end_cost_per_mm2_logic
        3000.0,                   // nre_back_end_cost_per_mm2_logic
        2000.0,                   // nre_front_end_cost_per_mm2_analog
        4000.0,                   // nre_back_end_cost_per_mm2_analog
        false                     // static_value
    );
    
    // Create an assembly process
    auto assembly_process = std::make_shared<Assembly>(
        "Example Assembly Process",  // name
        0.5,                         // alignment_yield
        0.95,                        // bonding_yield
        0.98,                        // dielectric_bond_yield
        0.1,                         // bonding_pitch
        0.5,                         // max_pad_current_density
        10.0,                        // picknplace_time_per_step
        5,                           // picknplace_group
        15.0,                        // bonding_time_per_step
        10,                          // bonding_group
        20.0,                        // machine_cost_per_hour
        30.0,                        // technician_cost_per_hour
        false                        // static_value
    );
    
    // Create a test process
    auto test_process = std::make_shared<Test>(
        "Example Test Process",  // name
        0.001,                   // time_per_test_cycle
        100.0,                   // cost_per_second
        10,                      // samples_per_input
        "True",                  // test_self
        1000.0,                  // bb_self_pattern_count
        100.0,                   // bb_self_scan_chain_length
        0.9,                     // self_defect_coverage
        0.5,                     // self_test_reuse
        4,                       // self_num_scan_chains
        2,                       // self_num_io_per_scan_chain
        0,                       // self_num_test_io_offset
        "Normal",                // self_test_failure_dist
        "True",                  // test_assembly
        2000.0,                  // bb_assembly_pattern_count
        200.0,                   // bb_assembly_scan_chain_length
        0.95,                    // assembly_defect_coverage
        0.6,                     // assembly_test_reuse
        8,                       // assembly_num_scan_chains
        4,                       // assembly_num_io_per_scan_chain
        2,                       // assembly_num_test_io_offset
        "Normal",                // assembly_test_failure_dist
        false                    // static_value
    );
    
    // Create a layer
    Layer layer(
        "Example Layer",  // name
        "Metal",          // layer_type
        0.5,              // thickness
        1.0,              // min_width
        1.0,              // min_spacing
        10.0,             // gates_per_mm2
        false             // static_value
    );
    
    // Create lists required by the Chip constructor
    std::vector<WaferProcess> wafer_process_list = {*wafer_process};
    std::vector<Assembly> assembly_process_list = {*assembly_process};
    std::vector<Test> test_process_list = {*test_process};
    std::vector<Layer> layers = {layer};
    std::vector<IO> ios = {
        IO("Example IO", 0.1, 0.1, 0.5, 10.0, 4, "True", 0.01, 10.0, false)
    };
    std::map<String, std::vector<std::vector<IntType>>> adjacency_matrix;
    std::vector<String> block_names = {"Example Block"};
    
    // Initialize the adjacency matrix with a sample entry for "Example IO"
    adjacency_matrix["Example IO"] = std::vector<std::vector<IntType>>(
        block_names.size(), std::vector<IntType>(block_names.size(), 1)
    );
    
    // Create a bandwidth utilization matrix structure
    std::map<String, std::vector<std::vector<FloatType>>> bandwidth_utilization;
    bandwidth_utilization["Example IO"] = std::vector<std::vector<FloatType>>(
        block_names.size(), std::vector<FloatType>(block_names.size(), 0.5)
    );
    
    std::vector<Layer> stackup = {layer};
    
    // Create a chip
    auto chip = std::make_shared<Chip>(
        "Example Chip",           // filename
        nullptr,                  // etree
        nullptr,                  // parent_chip
        &wafer_process_list,      // wafer_process_list
        &assembly_process_list,   // assembly_process_list
        &test_process_list,       // test_process_list
        &layers,                  // layers
        &ios,                     // ios
        &adjacency_matrix,        // adjacency_matrix_definitions
        &bandwidth_utilization,   // average_bandwidth_utilization
        &block_names,             // block_names
        false                     // static_value
    );
    
    // Set chip properties
    chip->SetCoreArea(100.0);
    chip->SetAspectRatio(1.0);
    chip->SetFractionMemory(0.3);
    chip->SetFractionLogic(0.6);
    chip->SetFractionAnalog(0.1);
    chip->SetWaferProcess(wafer_process);
    chip->SetAssemblyProcess(assembly_process);
    chip->SetTestProcess(test_process);
    chip->SetStackup(stackup);
    chip->SetQuantity(1000);
    
    // Print chip information
    std::cout << *chip << std::endl;
    
    // Calculate and print chip metrics
    std::cout << "\nChip Metrics:" << std::endl;
    std::cout << "Assembly Core Area: " << chip->GetAssemblyCoreArea() << " mm^2" << std::endl;
    std::cout << "Die Cost: " << chip->ComputeDieCost() << std::endl;
    std::cout << "Self Cost: " << chip->ComputeSelfCost() << std::endl;
    std::cout << "Total Cost: " << chip->ComputeTotalCost() << std::endl;
    std::cout << "Chip Yield: " << chip->ComputeChipYield() << std::endl;
    std::cout << "Wafer Area Efficiency: " << chip->WaferAreaEff() << std::endl;
    
    return 0;
} 