#include "../include/ReadDesignFromFile.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // Set example input files
    std::string wafer_process_file = "example_files/wafer_process_definitions.xml";
    std::string io_file = "example_files/io_definitions.xml";
    std::string layer_file = "example_files/layer_definitions.xml";
    std::string assembly_process_file = "example_files/assembly_process_definitions.xml";
    std::string test_process_file = "example_files/test_process_definitions.xml";

    // Change default files if provided as command-line arguments
    if (argc > 1) {
        wafer_process_file = argv[1];
    }
    if (argc > 2) {
        io_file = argv[2];
    }
    if (argc > 3) {
        layer_file = argv[3];
    }
    if (argc > 4) {
        assembly_process_file = argv[4];
    }
    if (argc > 5) {
        test_process_file = argv[5];
    }

    // Read wafer process definitions
    std::cout << "Reading wafer process definitions from " << wafer_process_file << std::endl;
    try {
        std::vector<design::WaferProcess> wafer_processes = read_design::WaferProcessDefinitionListFromFile(wafer_process_file);
        std::cout << "Successfully read " << wafer_processes.size() << " wafer process definitions" << std::endl;
        
        // Print wafer process details
        for (const auto& wp : wafer_processes) {
            std::cout << "  - Wafer Process: " << wp.GetName() 
                      << ", Diameter: " << wp.GetWaferDiameter() 
                      << ", Yield: " << wp.GetWaferProcessYield() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading wafer process definitions: " << e.what() << std::endl;
    }

    // Read IO definitions
    std::cout << "\nReading IO definitions from " << io_file << std::endl;
    try {
        std::vector<design::IO> ios = read_design::IODefinitionListFromFile(io_file);
        std::cout << "Successfully read " << ios.size() << " IO definitions" << std::endl;
        
        // Print IO details
        for (const auto& io : ios) {
            std::cout << "  - IO Type: " << io.GetType() 
                      << ", Bandwidth: " << io.GetBandwidth() 
                      << ", Energy/Bit: " << io.GetEnergyPerBit() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading IO definitions: " << e.what() << std::endl;
    }

    // Read layer definitions
    std::cout << "\nReading layer definitions from " << layer_file << std::endl;
    try {
        std::vector<design::Layer> layers = read_design::LayerDefinitionListFromFile(layer_file);
        std::cout << "Successfully read " << layers.size() << " layer definitions" << std::endl;
        
        // Print layer details
        for (const auto& layer : layers) {
            std::cout << "  - Layer: " << layer.GetName() 
                      << ", Active: " << (layer.GetActive() ? "Yes" : "No") 
                      << ", Cost/mm²: " << layer.GetCostPerMm2() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading layer definitions: " << e.what() << std::endl;
    }

    // Read assembly process definitions
    std::cout << "\nReading assembly process definitions from " << assembly_process_file << std::endl;
    try {
        std::vector<design::Assembly> assembly_processes = read_design::AssemblyProcessDefinitionListFromFile(assembly_process_file);
        std::cout << "Successfully read " << assembly_processes.size() << " assembly process definitions" << std::endl;
        
        // Print assembly process details
        for (const auto& ap : assembly_processes) {
            std::cout << "  - Assembly Process: " << ap.GetName() 
                      << ", Die Separation: " << ap.GetDieSeparation() 
                      << ", Alignment Yield: " << ap.GetAlignmentYield() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading assembly process definitions: " << e.what() << std::endl;
    }

    // Read test process definitions
    std::cout << "\nReading test process definitions from " << test_process_file << std::endl;
    try {
        std::vector<design::Test> test_processes = read_design::TestProcessDefinitionListFromFile(test_process_file);
        std::cout << "Successfully read " << test_processes.size() << " test process definitions" << std::endl;
        
        // Print test process details
        for (const auto& tp : test_processes) {
            std::cout << "  - Test Process: " << tp.GetName() 
                      << ", Cost/Second: " << tp.GetCostPerSecond() 
                      << ", Self Test: " << (tp.GetTestSelf() ? "Yes" : "No") << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading test process definitions: " << e.what() << std::endl;
    }

    std::cout << "\nDesign file reading complete!" << std::endl;
    return 0;
} 