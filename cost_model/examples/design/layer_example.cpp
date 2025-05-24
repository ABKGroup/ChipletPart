#include "design/Layer.hpp"
#include "design/WaferProcess.hpp"
#include <iostream>

using namespace design;

int main() {
    std::cout << "Layer Class Example\n";
    std::cout << "==================\n\n";
    
    // Create a Layer with default values (non-static)
    Layer layer1;
    
    // Set properties
    std::cout << "Setting properties for a layer...\n";
    layer1.SetName("Metal1");
    layer1.SetActive("true");
    layer1.SetCostPerMm2(0.05);
    layer1.SetTransistorDensity(10.0);  // 10 million transistors per mm^2
    layer1.SetDefectDensity(0.1);
    layer1.SetCriticalAreaRatio(0.2);
    layer1.SetClusteringFactor(2.0);
    layer1.SetLithoPercent(0.9);
    layer1.SetMaskCost(1000000.0);
    layer1.SetStitchingYield(0.99);
    
    // Print the layer
    std::cout << "\nLayer 1:\n" << layer1 << "\n\n";
    
    // Check if the layer is fully defined
    std::cout << "Layer1 fully defined: " << (layer1.LayerFullyDefined() ? "Yes" : "No") << "\n\n";
    
    // Get gates per mm^2
    std::cout << "Gates per mm^2: " << layer1.GetGatesPerMm2() << "\n\n";
    
    // Set the layer as static
    std::cout << "Setting Layer1 as static...\n";
    layer1.SetStatic();
    
    // Try to change a property after setting as static
    std::cout << "Attempting to change mask cost after setting static...\n";
    int result = layer1.SetMaskCost(2000000.0);
    std::cout << "SetMaskCost result: " << result << " (1 means error, 0 means success)\n\n";
    
    // Create a second layer with incomplete parameters
    std::cout << "Creating Layer2 with incomplete parameters...\n";
    Layer layer2("Metal2", "true", 0.07);
    
    // Print the second layer
    std::cout << "\nLayer 2:\n" << layer2 << "\n\n";
    
    // Check if the second layer is fully defined
    std::cout << "Layer2 fully defined: " << (layer2.LayerFullyDefined() ? "Yes" : "No") << "\n\n";
    
    // Create a WaferProcess for calculating Layer Costs
    std::cout << "Creating a WaferProcess for cost calculations...\n";
    WaferProcess wafer_process;
    wafer_process.SetWaferDiameter(300.0);  // 300mm wafer
    wafer_process.SetEdgeExclusion(3.0);    // 3mm edge exclusion
    wafer_process.SetDicingDistance(0.1);   // 0.1mm dicing distance
    wafer_process.SetWaferFillGrid("true");
    wafer_process.SetReticleX(26.0);        // 26mm reticle x dimension
    wafer_process.SetReticleY(32.0);        // 32mm reticle y dimension
    
    // Calculate layer yield and cost for a given area
    FloatType area = 100.0;  // 100 mm^2
    FloatType aspect_ratio = 1.0;  // Square chip
    
    std::cout << "\nCalculating yield and cost for a " << area << " mm^2 chip...\n";
    std::cout << "Layer yield: " << layer1.LayerYield(area) << "\n";
    
    try {
        std::cout << "Layer cost: $" << layer1.LayerCost(area, aspect_ratio, wafer_process) << "\n";
        std::cout << "Cost per mm^2: $" << layer1.ComputeCostPerMm2(area, aspect_ratio, wafer_process) << "\n\n";
        
        // Calculate dies per wafer
        FloatType x_dim = 10.0;  // 10mm
        FloatType y_dim = 10.0;  // 10mm
        FloatType usable_wafer_diameter = 294.0;  // 300mm - 2*3mm edge exclusion
        FloatType dicing_distance = 0.1;  // 0.1mm
        
        std::cout << "Calculating dies per wafer for a " << x_dim << "x" << y_dim << " mm die...\n";
        std::cout << "Grid layout: " << layer1.ComputeGridDiesPerWafer(x_dim, y_dim, usable_wafer_diameter, dicing_distance) << " dies\n";
        std::cout << "No-grid layout: " << layer1.ComputeNogridDiesPerWafer(x_dim, y_dim, usable_wafer_diameter, dicing_distance) << " dies\n";
    }
    catch (const std::exception& e) {
        std::cout << "Error during calculation: " << e.what() << "\n";
    }
    
    std::cout << "\nExample completed successfully!\n";
    return 0;
} 