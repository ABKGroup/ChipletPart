#include <iostream>
#include <cassert>
#include <cmath>
#include <optional>
#include "design/Test.hpp"

using namespace design;

// Helper function to check if a value is close to an expected value and report sign if it's not
bool check_close(double actual, double expected, double epsilon = 0.0001) {
    double diff = actual - expected;
    if (std::abs(diff) >= epsilon) {
        if (diff < 0) {
            std::cout << "FAIL: " << actual << " is less than expected " << expected << " by " << -diff << std::endl;
        } else {
            std::cout << "FAIL: " << actual << " is greater than expected " << expected << " by " << diff << std::endl;
        }
        return false;
    }
    return true;
}

int main() {
    // Create a Test with the same parameters as in the Python test
    Test tp(
        "test_test_process",    // name
        0.000001,               // time_per_test_cycle
        0.01,                   // cost_per_second
        1,                      // samples_per_input
        "False",                // test_self
        DEFAULT_FLOAT_VALUE,    // bb_self_pattern_count
        DEFAULT_FLOAT_VALUE,    // bb_self_scan_chain_length
        0.9,                    // self_defect_coverage
        1,                      // self_test_reuse
        7,                      // self_num_scan_chains
        2,                      // self_num_io_per_scan_chain
        1,                      // self_num_test_io_offset
        "normal",               // self_test_failure_dist
        "False",                // test_assembly
        DEFAULT_FLOAT_VALUE,    // bb_assembly_pattern_count
        DEFAULT_FLOAT_VALUE,    // bb_assembly_scan_chain_length
        0.5,                    // assembly_defect_coverage
        1,                      // assembly_test_reuse
        3,                      // assembly_num_scan_chains
        4,                      // assembly_num_io_per_scan_chain
        2,                      // assembly_num_test_io_offset
        "normal",               // assembly_test_failure_dist
        false                   // static_value - setting to false to match Python behavior
    );
    
    // Test that all properties are correctly set
    assert(tp.TestFullyDefined() == true);
    assert(tp.GetName() == "test_test_process");
    assert(check_close(tp.GetTimePerTestCycle(), 0.000001));
    assert(check_close(tp.GetCostPerSecond(), 0.01));
    assert(tp.GetSamplesPerInput() == 1);
    
    // These are now std::optional<BoolType>, so test using value_or
    assert(tp.GetTestSelf() == false);  // Gets the value or default false
    
    assert(tp.GetBbSelfPatternCount() == DEFAULT_FLOAT_VALUE); // Equivalent to None in Python
    assert(tp.GetBbSelfScanChainLength() == DEFAULT_FLOAT_VALUE); // Equivalent to None in Python
    assert(check_close(tp.GetSelfDefectCoverage(), 0.9));
    assert(check_close(tp.GetSelfTestReuse(), 1));
    assert(tp.GetSelfNumScanChains() == 7);
    assert(tp.GetSelfNumIoPerScanChain() == 2);
    assert(tp.GetSelfNumTestIoOffset() == 1);
    assert(tp.GetSelfTestFailureDist() == "normal");
    
    // Test the test_assembly_ std::optional<BoolType>
    assert(tp.GetTestAssembly() == false);  // Gets the value or default false
    
    assert(tp.GetBbAssemblyPatternCount() == DEFAULT_FLOAT_VALUE); // Equivalent to None in Python
    assert(tp.GetBbAssemblyScanChainLength() == DEFAULT_FLOAT_VALUE); // Equivalent to None in Python
    assert(check_close(tp.GetAssemblyDefectCoverage(), 0.5));
    assert(check_close(tp.GetAssemblyTestReuse(), 1));
    assert(tp.GetAssemblyNumScanChains() == 3);
    assert(tp.GetAssemblyNumIoPerScanChain() == 4);
    assert(tp.GetAssemblyNumTestIoOffset() == 2);
    assert(tp.GetAssemblyTestFailureDist() == "normal");
    
    std::cout << "All Test tests passed!" << std::endl;
    return 0;
} 