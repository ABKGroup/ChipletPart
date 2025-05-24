#include "design/Test.hpp"
#include "design/Chip.hpp"
#include <iostream>
#include <memory>

// Forward declaration of the initialization function defined in TestChipBinding.cpp
namespace design {
    void InitializeTestWithChipFunctions(Test& test);
}

int main() {
    // Create a test object
    std::cout << "Creating Test object..." << std::endl;
    design::Test test(
        "TestBindingExample",         // name
        0.05,                         // time_per_test_cycle
        30.0,                         // cost_per_second
        200,                          // samples_per_input
        "true",                       // test_self
        1000.0,                       // bb_self_pattern_count
        100.0,                        // bb_self_scan_chain_length
        0.9,                          // self_defect_coverage
        1.0,                          // self_test_reuse
        8,                            // self_num_scan_chains
        4,                            // self_num_io_per_scan_chain
        2,                            // self_num_test_io_offset
        "Gaussian",                   // self_test_failure_dist
        "true",                       // test_assembly
        1500.0,                       // bb_assembly_pattern_count
        150.0,                        // bb_assembly_scan_chain_length
        0.98,                         // assembly_defect_coverage
        1.2,                          // assembly_test_reuse
        10,                           // assembly_num_scan_chains
        5,                            // assembly_num_io_per_scan_chain
        3,                            // assembly_num_test_io_offset
        "Weibull",                    // assembly_test_failure_dist
        false                         // static
    );
    
    // Create a mock Chip object
    std::cout << "Creating chip object for demonstration..." << std::endl;
    auto chip = std::make_shared<design::Chip>();
    
    // Initialize Test with function pointers to access Chip methods
    std::cout << "Initializing Test with Chip function pointers..." << std::endl;
    design::InitializeTestWithChipFunctions(test);
    
    std::cout << "\nDemonstrating Test-Chip interaction:" << std::endl;
    std::cout << "Note: This is just a demonstration. Since Chip is not fully initialized," << std::endl;
    std::cout << "      the actual values returned will be default values." << std::endl;
    
    // Display test information
    std::cout << test << std::endl;
    
    // Calculate number of test IOs required
    std::cout << "Number of test IOs required: " << test.NumTestIos() << std::endl;
    
    std::cout << "\nExample completed successfully." << std::endl;
    
    return 0;
} 