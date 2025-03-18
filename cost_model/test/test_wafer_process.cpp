#include <iostream>
#include <cassert>
#include <cmath>
#include "design/WaferProcess.hpp"

using namespace design;

// Helper function to check if a value is close to an expected value and report sign if it's not
bool check_close(double actual, double expected, double epsilon = 0.001) {
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
    // Create WaferProcess with all parameters, using the exact same parameters as in Python test
    WaferProcess wp("test_wafer_process", 234.0, 1.2, 0.98, 0.87, 32.0, 23.0, "False", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6);
    
    // Check if fully defined
    assert(wp.WaferFullyDefined() == true);
    
    // Verify all properties using getter methods
    assert(wp.GetName() == "test_wafer_process");
    assert(check_close(wp.GetWaferDiameter(), 234.0));
    assert(check_close(wp.GetEdgeExclusion(), 1.2));
    assert(check_close(wp.GetWaferProcessYield(), 0.98));
    assert(check_close(wp.GetDicingDistance(), 0.87));
    assert(check_close(wp.GetReticleX(), 32.0));
    assert(check_close(wp.GetReticleY(), 23.0));
    assert(wp.GetWaferFillGrid() == false);
    assert(check_close(wp.GetNreFrontEndCostPerMm2Memory(), 0.1));
    assert(check_close(wp.GetNreBackEndCostPerMm2Memory(), 0.2));
    assert(check_close(wp.GetNreFrontEndCostPerMm2Logic(), 0.3));
    assert(check_close(wp.GetNreBackEndCostPerMm2Logic(), 0.4));
    assert(check_close(wp.GetNreFrontEndCostPerMm2Analog(), 0.5));
    assert(check_close(wp.GetNreBackEndCostPerMm2Analog(), 0.6));
    
    // Create another WaferProcess with wafer_fill_grid set to true
    WaferProcess wp2("test_wafer_process", 234.0, 1.2, 0.98, 0.87, 32.0, 23.0, "True", 0.1, 0.2, 0.3, 0.4, 0.5, 0.6);
    
    // Check wafer_fill_grid property
    assert(wp2.GetWaferFillGrid() == true);
    
    std::cout << "All WaferProcess tests passed successfully!" << std::endl;
    return 0;
} 