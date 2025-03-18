#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "../include/ConstructChip.hpp"

int main() {
    std::cout << "ChipletPart Construction Example\n";
    std::cout << "================================\n\n";
    
    // Example 1: Extract technology node
    std::cout << "Example 1: Extract Technology Node\n";
    std::cout << "---------------------------------\n";
    
    std::vector<std::string> test_nodes = {
        "45nm", 
        "BT_TSMC_7nm", 
        "BT_TSMC_12nm", 
        "TSMC_5nm"
    };
    
    for (const auto& node : test_nodes) {
        std::cout << "Original: " << node << " -> Extracted: " 
                  << construct_chip::ExtractTechNode(node) << std::endl;
    }
    std::cout << std::endl;
    
    // Example 2: Create Element Tree
    std::cout << "Example 2: Create Element Tree\n";
    std::cout << "-----------------------------\n";
    
    // Sample data for creating a chip with 3 subtrees
    std::vector<std::string> subtree_tech_nodes = {"7nm", "10nm", "45nm"};
    std::vector<double> subtree_aspect_ratios = {1.0, 1.5, 2.0};
    std::vector<double> subtree_x_locations = {0.0, 10.0, 20.0};
    std::vector<double> subtree_y_locations = {0.0, 10.0, 20.0};
    std::vector<double> subtree_power = {1.0, 2.0, 3.0};
    std::vector<double> subtree_core_area = {100.0, 200.0, 300.0};
    int num_subtrees = 3;
    
    pugi::xml_document doc = construct_chip::CreateElementTree(
        subtree_tech_nodes,
        subtree_aspect_ratios,
        subtree_x_locations,
        subtree_y_locations,
        subtree_power,
        subtree_core_area,
        num_subtrees
    );
    
    std::cout << "Created XML document with root element name: " 
              << doc.child("chip").attribute("name").value() << std::endl;
    std::cout << "Number of child chips: " 
              << std::distance(doc.child("chip").children().begin(), doc.child("chip").children().end()) 
              << std::endl;
    
    // Example 3: Increment Netlist
    std::cout << "\nExample 3: Increment Netlist\n";
    std::cout << "-------------------------\n";
    
    // Create a simple connectivity matrix
    std::map<std::string, std::vector<std::vector<double>>> connectivity;
    connectivity["io1"] = {{1.0, 2.0}, {3.0, 4.0}};
    connectivity["io2"] = {{5.0, 6.0}, {7.0, 8.0}};
    
    std::cout << "Original connectivity for io1[1][1]: " 
              << connectivity["io1"][1][1] << std::endl;
    std::cout << "Original connectivity for io2[1][1]: " 
              << connectivity["io2"][1][1] << std::endl;
    
    // Increment the netlist
    double bandwidth_change = 10.0;
    int partition_id = 1;
    
    auto updated_connectivity = construct_chip::IncrementNetlist(
        connectivity,
        bandwidth_change,
        partition_id
    );
    
    std::cout << "Updated connectivity for io1[1][1]: " 
              << updated_connectivity["io1"][1][1] << std::endl;
    std::cout << "Updated connectivity for io2[1][1]: " 
              << updated_connectivity["io2"][1][1] << std::endl;
    
    std::cout << "\nExample completed successfully!\n";
    return 0;
} 