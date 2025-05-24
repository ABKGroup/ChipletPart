#include "design/IO.hpp"
#include <iostream>

using namespace design;

int main() {
    // Create an IO object with default values
    IO io1;
    
    // Set various properties
    io1.SetType("HBM3");
    io1.SetRxArea(0.05);
    io1.SetTxArea(0.05);
    io1.SetShoreline(0.2);
    io1.SetBandwidth(16.0);
    io1.SetWireCount(512);
    io1.SetBidirectional("true");
    io1.SetEnergyPerBit(5.0);
    io1.SetReach(2.5);
    
    // Print the state
    std::cout << "IO 1:" << std::endl;
    std::cout << io1 << std::endl;
    
    // Check if fully defined
    std::cout << "IO 1 fully defined: " << (io1.IoFullyDefined() ? "True" : "False") << std::endl;
    
    // Set static and try to change bandwidth
    io1.SetStatic();
    std::cout << "Attempting to change bandwidth after setting static:" << std::endl;
    int result = io1.SetBandwidth(32.0);
    std::cout << "Result: " << (result == 0 ? "Success" : "Failure") << std::endl;
    
    // Create an IO with invalid parameters
    IO io2;
    io2.SetType("InvalidIO");
    io2.SetRxArea(-1.0); // This will cause an error and not set the value
    
    // Print state
    std::cout << "\nIO 2:" << std::endl;
    std::cout << io2 << std::endl;
    
    // Check if fully defined
    std::cout << "IO 2 fully defined: " << (io2.IoFullyDefined() ? "True" : "False") << std::endl;
    std::cout << "Will not attempt to set IO 2 as static due to invalid state" << std::endl;
    
    // Create an IO using constructor with valid parameters
    IO io3("SerDes", 0.01, 0.01, 0.1, 10.0, 128, "true", 2.0, 10.0, true);
    
    // Print state
    std::cout << "\nIO 3 (created with constructor):" << std::endl;
    std::cout << io3 << std::endl;
    
    std::cout << "\nExample completed successfully." << std::endl;
    return 0;
} 