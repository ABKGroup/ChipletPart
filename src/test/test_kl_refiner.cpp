#include "../KLRefiner.h"
#include "../Hypergraph.h"
#include <iostream>
#include <vector>
#include <random>
#include <unordered_set>
#include <numeric>

using namespace chiplet;

// Helper function to calculate block balance for our test
Matrix<float> GetBlockBalance(const HGraphPtr& hgraph, const std::vector<int>& solution) {
  // Find maximum partition ID
  int max_part_id = -1;
  for (int part_id : solution) {
    max_part_id = std::max(max_part_id, part_id);
  }
  
  int num_parts = max_part_id + 1;
  
  // Initialize block balance matrix
  Matrix<float> block_balance(num_parts, std::vector<float>(hgraph->GetVertexDimensions(), 0.0f));
  
  // Calculate balance for each partition
  for (int v = 0; v < hgraph->GetNumVertices(); ++v) {
    int part_id = solution[v];
    if (part_id >= 0 && part_id < num_parts) {
      const auto& weights = hgraph->GetVertexWeights(v);
      for (size_t i = 0; i < weights.size(); ++i) {
        block_balance[part_id][i] += weights[i];
      }
    }
  }
  
  return block_balance;
}

// Create a simple hypergraph for testing
HGraphPtr createTestHypergraph(int num_vertices, int num_hyperedges) {
  // Create random vertices and hyperedges
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> weight_dist(1.0, 10.0);
  std::uniform_int_distribution<int> vertex_dist(0, num_vertices - 1);
  
  // Vertex weights (single dimension)
  Matrix<float> vertex_weights(num_vertices, std::vector<float>(1, 0.0));
  for (int i = 0; i < num_vertices; i++) {
    vertex_weights[i][0] = weight_dist(gen);
  }
  
  // Create hyperedges
  Matrix<int> hyperedges;
  Matrix<float> hyperedge_weights;
  std::vector<float> reaches;
  std::vector<float> io_sizes;
  
  for (int i = 0; i < num_hyperedges; i++) {
    // Create a hyperedge with 2-5 vertices
    std::uniform_int_distribution<int> size_dist(2, std::min(5, num_vertices));
    int edge_size = size_dist(gen);
    
    std::vector<int> edge;
    std::unordered_set<int> used_vertices;
    
    // Add random vertices to the edge
    while (edge.size() < edge_size) {
      int v = vertex_dist(gen);
      if (used_vertices.find(v) == used_vertices.end()) {
        edge.push_back(v);
        used_vertices.insert(v);
      }
    }
    
    hyperedges.push_back(edge);
    hyperedge_weights.push_back(std::vector<float>(1, weight_dist(gen)));
    reaches.push_back(1.0);
    io_sizes.push_back(1.0);
  }
  
  // Create the hypergraph
  return std::make_shared<Hypergraph>(
      1,  // vertex dimensions
      1,  // hyperedge dimensions
      hyperedges,
      vertex_weights,
      hyperedge_weights,
      reaches,
      io_sizes
  );
}

// Create an initial random partition
std::vector<int> createRandomPartition(int num_vertices, int num_parts) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> part_dist(0, num_parts - 1);
  
  std::vector<int> partition(num_vertices);
  for (int i = 0; i < num_vertices; i++) {
    partition[i] = part_dist(gen);
  }
  
  return partition;
}

// Calculate the cut size (number of hyperedges that span multiple partitions)
int calculateCutSize(const HGraphPtr& hgraph, const std::vector<int>& partition) {
  int cut_size = 0;
  
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    std::unordered_set<int> parts_seen;
    for (const int v : hgraph->Vertices(e)) {
      parts_seen.insert(partition[v]);
    }
    
    if (parts_seen.size() > 1) {
      cut_size++;
    }
  }
  
  return cut_size;
}

int main(int argc, char* argv[]) {
  // Parameters
  const int num_vertices = 100;
  const int num_hyperedges = 300;
  const int num_parts = 4;
  const int kl_max_swaps = 50;
  const int kl_iterations = 3;
  
  std::cout << "Creating test hypergraph with " << num_vertices << " vertices and "
            << num_hyperedges << " hyperedges..." << std::endl;
  
  // Create test hypergraph
  HGraphPtr hgraph = createTestHypergraph(num_vertices, num_hyperedges);
  
  // Create initial random partition
  std::vector<int> partition = createRandomPartition(num_vertices, num_parts);
  
  // Calculate initial cut size
  int initial_cut_size = calculateCutSize(hgraph, partition);
  std::cout << "Initial partition cut size: " << initial_cut_size << std::endl;
  
  // Create balance constraints (allow 10% imbalance)
  Matrix<float> balance = GetBlockBalance(hgraph, partition);
  float total_weight = 0.0;
  for (int i = 0; i < num_parts; i++) {
    total_weight += balance[i][0];
  }
  
  float target_weight = total_weight / num_parts;
  float upper_factor = 1.1;  // 10% imbalance
  float lower_factor = 0.9;  // 10% imbalance
  
  Matrix<float> upper_balance(num_parts, std::vector<float>(1, target_weight * upper_factor));
  Matrix<float> lower_balance(num_parts, std::vector<float>(1, target_weight * lower_factor));
  
  // Create and run KL refiner
  std::cout << "Running KL refinement..." << std::endl;
  KLRefiner refiner(num_parts, kl_iterations, kl_max_swaps, false);
  
  // Refine the partition
  refiner.Refine(hgraph, upper_balance, lower_balance, partition);
  
  // Calculate final cut size
  int final_cut_size = calculateCutSize(hgraph, partition);
  std::cout << "Final partition cut size: " << final_cut_size << std::endl;
  std::cout << "Improvement: " << (initial_cut_size - final_cut_size) << " (" 
            << (100.0 * (initial_cut_size - final_cut_size) / initial_cut_size) << "%)" << std::endl;
  
  // Calculate final balance
  Matrix<float> final_balance = GetBlockBalance(hgraph, partition);
  std::cout << "Final partition balance:" << std::endl;
  for (int i = 0; i < num_parts; i++) {
    std::cout << "  Part " << i << ": " << final_balance[i][0] << std::endl;
  }
  
  // Test with floorplanner enabled
  std::cout << "\nRunning KL refinement with floorplanner..." << std::endl;
  
  // Reset partition
  partition = createRandomPartition(num_vertices, num_parts);
  initial_cut_size = calculateCutSize(hgraph, partition);
  std::cout << "Initial partition cut size: " << initial_cut_size << std::endl;
  
  // Create refiner with floorplanner enabled
  KLRefiner refiner_with_fp(num_parts, kl_iterations, kl_max_swaps, true);
  
  // Configure floorplanner
  refiner_with_fp.SetFloorplannerParams(2, 100, 20);
  
  // Refine the partition
  refiner_with_fp.Refine(hgraph, upper_balance, lower_balance, partition);
  
  // Calculate final cut size
  final_cut_size = calculateCutSize(hgraph, partition);
  std::cout << "Final partition cut size: " << final_cut_size << std::endl;
  std::cout << "Improvement: " << (initial_cut_size - final_cut_size) << " (" 
            << (100.0 * (initial_cut_size - final_cut_size) / initial_cut_size) << "%)" << std::endl;
  
  return 0;
} 