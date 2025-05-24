#include "../FMRefiner.h"
#include "../Hypergraph.h"
#include <iostream>
#include <vector>

using namespace chiplet;

int main() {
    std::cout << "Testing GWTW Floorplanner..." << std::endl;

    // Create a simple hypergraph for testing
    int num_vertices = 5;
    int num_hyperedges = 4;
    Matrix<int> hyperedges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 4}
    };
    
    // Vertex weights (area)
    Matrix<float> vertex_weights(num_vertices, std::vector<float>{100.0});
    
    // Hyperedge weights (connection strength)
    Matrix<float> hyperedge_weights(num_hyperedges, std::vector<float>{1.0});
    
    // Reaches for each hyperedge
    std::vector<float> reaches = {10.0, 10.0, 10.0, 10.0};
    
    // IO areas
    std::vector<float> io_sizes = {1.0, 1.0, 1.0, 1.0};
    
    // Create hypergraph
    auto hgraph = std::make_shared<Hypergraph>(
        1, 1, hyperedges, vertex_weights, hyperedge_weights, reaches, io_sizes);
    
    // Simple partition - everything in one chiplet
    std::vector<int> partition = {0, 0, 0, 1, 1};
    
    // Create refiner
    std::vector<int> reaches_int = {10, 10, 10, 10};
    ChipletRefiner refiner(2, 1, 10, reaches_int, true);
    
    // Configure GWTW parameters
    refiner.SetGWTWIterations(2);
    refiner.SetGWTWMaxTemp(100.0);
    refiner.SetGWTWMinTemp(1e-12);
    refiner.SetGWTWSyncFreq(0.1);
    refiner.SetGWTWTopK(2);
    refiner.SetGWTWTempDerateFactor(1.0);
    refiner.SetGWTWTopKRatio({0.5, 0.5});
    
    // Run floorplanner
    auto result = refiner.RunFloorplanner(partition, hgraph, 1000, 100, 0.95);
    
    // Check results
    bool is_valid = std::get<3>(result);
    
    if (is_valid) {
        std::cout << "GWTW Floorplanner test PASSED" << std::endl;
    } else {
        std::cout << "GWTW Floorplanner test FAILED" << std::endl;
    }
    
    return 0;
} 