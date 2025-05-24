#include "design/WaferProcess.hpp"
#include <iostream>

int main() {
    std::cout << "WaferProcess Class Example\n";
    std::cout << "==========================\n\n";
    
    // Create a wafer process with default values (non-static)
    design::WaferProcess wp1;
    
    // Set properties
    std::cout << "Setting properties for a wafer process...\n";
    wp1.SetName("TSMC_7nm");
    wp1.SetWaferDiameter(300.0);
    wp1.SetEdgeExclusion(3.0);
    wp1.SetWaferProcessYield(0.95);
    wp1.SetDicingDistance(0.2);
    wp1.SetReticleX(26.5);
    wp1.SetReticleY(31.5);
    wp1.SetWaferFillGrid("True");
    wp1.SetNreFrontEndCostPerMm2Memory(100.0);
    wp1.SetNreBackEndCostPerMm2Memory(150.0);
    wp1.SetNreFrontEndCostPerMm2Logic(200.0);
    wp1.SetNreBackEndCostPerMm2Logic(250.0);
    wp1.SetNreFrontEndCostPerMm2Analog(300.0);
    wp1.SetNreBackEndCostPerMm2Analog(350.0);
    
    // Print the wafer process
    std::cout << "\nWafer Process WP1:\n" << wp1 << "\n\n";
    
    // Check if the wafer process is fully defined
    std::cout << "WP1 fully defined: " << (wp1.WaferFullyDefined() ? "Yes" : "No") << "\n\n";
    
    // Set the wafer process as static
    std::cout << "Setting WP1 as static...\n";
    wp1.SetStatic();
    
    // Try to change a property after setting as static
    std::cout << "Attempting to change WaferDiameter after setting static...\n";
    int result = wp1.SetWaferDiameter(450.0);
    std::cout << "SetWaferDiameter result: " << result << " (1 means error, 0 means success)\n\n";
    
    // Create a second wafer process with invalid parameters
    std::cout << "Creating WP2 with invalid parameters...\n";
    design::WaferProcess wp2("InvalidWP", -1.0, 0.0, 2.0, 0.0, 0.0, 0.0, "True", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, false);
    
    // Print the second wafer process
    std::cout << "\nWafer Process WP2:\n" << wp2 << "\n\n";
    
    // Check if the second wafer process is fully defined
    std::cout << "WP2 fully defined: " << (wp2.WaferFullyDefined() ? "Yes" : "No") << "\n\n";
    
    // Note that we don't try to set WP2 as static since it would call exit(1)
    std::cout << "Not attempting to set WP2 as static because it would call exit(1)\n\n";

    // Example of creating a valid wafer process with constructor
    std::cout << "\nCreating WP3 with constructor...\n";
    design::WaferProcess wp3(
        "TSMC_5nm",      // name
        300.0,           // wafer_diameter
        3.0,             // edge_exclusion
        0.92,            // wafer_process_yield
        0.18,            // dicing_distance
        24.0,            // reticle_x
        28.0,            // reticle_y
        "True",          // wafer_fill_grid
        120.0,           // nre_front_end_cost_per_mm2_memory
        180.0,           // nre_back_end_cost_per_mm2_memory
        240.0,           // nre_front_end_cost_per_mm2_logic
        300.0,           // nre_back_end_cost_per_mm2_logic
        360.0,           // nre_front_end_cost_per_mm2_analog
        420.0,           // nre_back_end_cost_per_mm2_analog
        true             // static
    );

    // Print the third wafer process
    std::cout << "\nWafer Process WP3:\n" << wp3 << "\n\n";

    std::cout << "Example completed successfully!\n";
    return 0;
} 