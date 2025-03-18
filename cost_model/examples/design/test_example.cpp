#include "design/Test.hpp"
#include <iostream>

int main() {
    // Create a simple Test object
    std::cout << "Creating Test object with minimal parameters..." << std::endl;
    design::Test test1(
        "TestMinimal",               // name
        0.1,                         // time_per_test_cycle
        20.0,                        // cost_per_second
        100,                         // samples_per_input
        "false",                     // test_self
        design::DEFAULT_FLOAT_VALUE, // bb_self_pattern_count
        design::DEFAULT_FLOAT_VALUE, // bb_self_scan_chain_length
        design::DEFAULT_FLOAT_VALUE, // self_defect_coverage
        design::DEFAULT_FLOAT_VALUE, // self_test_reuse
        design::DEFAULT_INT_VALUE,   // self_num_scan_chains
        design::DEFAULT_INT_VALUE,   // self_num_io_per_scan_chain
        design::DEFAULT_INT_VALUE,   // self_num_test_io_offset
        "",                          // self_test_failure_dist
        "false",                     // test_assembly
        design::DEFAULT_FLOAT_VALUE, // bb_assembly_pattern_count
        design::DEFAULT_FLOAT_VALUE, // bb_assembly_scan_chain_length
        design::DEFAULT_FLOAT_VALUE, // assembly_defect_coverage
        design::DEFAULT_FLOAT_VALUE, // assembly_test_reuse
        design::DEFAULT_INT_VALUE,   // assembly_num_scan_chains
        design::DEFAULT_INT_VALUE,   // assembly_num_io_per_scan_chain
        design::DEFAULT_INT_VALUE,   // assembly_num_test_io_offset
        "",                          // assembly_test_failure_dist
        false                        // static
    );
    std::cout << test1 << std::endl;
    
    // Create a more complex Test object with self testing
    std::cout << "\nCreating Test object with self testing enabled..." << std::endl;
    design::Test test2(
        "TestWithSelfTest",          // name
        0.05,                        // time_per_test_cycle
        30.0,                        // cost_per_second
        200,                         // samples_per_input
        "true",                      // test_self
        1000.0,                      // bb_self_pattern_count
        100.0,                       // bb_self_scan_chain_length
        0.9,                         // self_defect_coverage
        1.0,                         // self_test_reuse
        8,                           // self_num_scan_chains
        4,                           // self_num_io_per_scan_chain
        2,                           // self_num_test_io_offset
        "Gaussian",                  // self_test_failure_dist
        "false",                     // test_assembly
        design::DEFAULT_FLOAT_VALUE, // bb_assembly_pattern_count
        design::DEFAULT_FLOAT_VALUE, // bb_assembly_scan_chain_length
        design::DEFAULT_FLOAT_VALUE, // assembly_defect_coverage
        design::DEFAULT_FLOAT_VALUE, // assembly_test_reuse
        design::DEFAULT_INT_VALUE,   // assembly_num_scan_chains
        design::DEFAULT_INT_VALUE,   // assembly_num_io_per_scan_chain
        design::DEFAULT_INT_VALUE,   // assembly_num_test_io_offset
        "",                          // assembly_test_failure_dist
        false                        // static
    );
    std::cout << test2 << std::endl;
    
    // Compute test time and cost
    design::FloatType gate_count = 1000000.0; // 1 million gates
    std::cout << "\nSelf test time for " << gate_count << " gates: " 
              << test2.ComputeSelfTime(gate_count) << " seconds" << std::endl;
    std::cout << "Self test cost for " << gate_count << " gates: " 
              << test2.ComputeSelfCost(gate_count) << " units" << std::endl;
    
    // Required IO calculation
    std::cout << "Required self test IO: " << test2.ComputeRequiredSelfIo() << std::endl;
    
    // Create a complete Test object with both self and assembly testing
    std::cout << "\nCreating Test object with self and assembly testing enabled..." << std::endl;
    design::Test test3(
        "TestComplete",              // name
        0.02,                        // time_per_test_cycle
        40.0,                        // cost_per_second
        500,                         // samples_per_input
        "true",                      // test_self
        2000.0,                      // bb_self_pattern_count
        200.0,                       // bb_self_scan_chain_length
        0.95,                        // self_defect_coverage
        1.5,                         // self_test_reuse
        12,                          // self_num_scan_chains
        6,                           // self_num_io_per_scan_chain
        4,                           // self_num_test_io_offset
        "Poisson",                   // self_test_failure_dist
        "true",                      // test_assembly
        1500.0,                      // bb_assembly_pattern_count
        150.0,                       // bb_assembly_scan_chain_length
        0.98,                        // assembly_defect_coverage
        1.2,                         // assembly_test_reuse
        10,                          // assembly_num_scan_chains
        5,                           // assembly_num_io_per_scan_chain
        3,                           // assembly_num_test_io_offset
        "Weibull",                   // assembly_test_failure_dist
        false                        // static
    );
    
    // Set gate-flop ratio for assembly test
    test3.SetAssemblyGateFlopRatio(10.0);
    
    std::cout << test3 << std::endl;
    
    // Compute test time and cost
    std::cout << "\nSelf test time: " << test3.ComputeSelfTime(gate_count) << " seconds" << std::endl;
    std::cout << "Assembly test time: " << test3.ComputeAssemblyTime(gate_count) << " seconds" << std::endl;
    std::cout << "Total test time: " << test3.ComputeTotalTime(gate_count) << " seconds" << std::endl;
    
    std::cout << "Self test cost: " << test3.ComputeSelfCost(gate_count) << " units" << std::endl;
    std::cout << "Assembly test cost: " << test3.ComputeAssemblyCost(gate_count) << " units" << std::endl;
    std::cout << "Total test cost: " << test3.ComputeTotalCost(gate_count) << " units" << std::endl;
    
    // Required IO calculation
    std::cout << "Required self test IO: " << test3.ComputeRequiredSelfIo() << std::endl;
    std::cout << "Required assembly test IO: " << test3.ComputeRequiredAssemblyIo() << std::endl;
    
    // Make the test static
    std::cout << "\nSetting test to static..." << std::endl;
    test3.SetStatic();
    std::cout << "Is test static? " << (test3.GetStatic() ? "Yes" : "No") << std::endl;
    
    // Try to modify the static test
    std::cout << "\nAttempting to modify static test..." << std::endl;
    design::IntType result = test3.SetTimePerTestCycle(0.01);
    std::cout << "Modification result: " << (result == 0 ? "Success" : "Failure") << std::endl;
    
    std::cout << "\nTest example completed successfully." << std::endl;
    
    return 0;
} 