#include <iostream>
#include <cassert>
#include <cmath>
#include "design/IO.hpp"

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
    // Create IO with all parameters, using the exact same parameters as in Python test
    IO io("test_io_process", 1.0, 2.0, 0.5, 3.0, 4, "True", 0.9, 5.0, true);
    
    // Check if fully defined
    assert(io.IoFullyDefined() == true);
    
    // Verify all properties using getter methods
    assert(io.GetType() == "test_io_process");
    assert(check_close(io.GetRxArea(), 1.0));
    assert(check_close(io.GetTxArea(), 2.0));
    assert(check_close(io.GetShoreline(), 0.5));
    assert(check_close(io.GetBandwidth(), 3.0));
    assert(io.GetWireCount() == 4);
    assert(io.GetBidirectional() == true);
    assert(check_close(io.GetEnergyPerBit(), 0.9));
    assert(check_close(io.GetReach(), 5.0));
    
    // Create another IO with bidirectional set to false
    IO io2("test_io_process", 1.0, 2.0, 0.5, 3.0, 4, "False", 0.9, 5.0, true);
    
    // Check bidirectional property
    assert(io2.GetBidirectional() == false);
    
    std::cout << "All IO tests passed successfully!" << std::endl;
    return 0;
} 