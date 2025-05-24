#include "include/design/DesignCommon.hpp"
#include <iostream>
#include <iomanip>

int main() {
    double bonding_yield = 0.999999;
    int n_bonds = 42948;
    
    // Calculate using our custom python_pow function (Boost)
    long double result_custom = design::python_pow(bonding_yield, n_bonds);
    
    // Calculate using standard library pow
    long double result_std = std::pow(bonding_yield, n_bonds);
    
    // Expected Python result from user
    double expected_python = 0.9560462177787956;
    
    // Print results with high precision
    std::cout << std::setprecision(20);
    std::cout << "Boost implementation: " << result_custom << std::endl;
    std::cout << "Std library pow:      " << result_std << std::endl;
    std::cout << "Expected from Python: " << expected_python << std::endl;
    
    // Calculate difference
    long double difference = std::abs(result_custom - expected_python);
    std::cout << "Absolute difference:  " << difference << std::endl;
    
    return 0;
} 