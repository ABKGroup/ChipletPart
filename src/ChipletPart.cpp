///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "ChipletPart.h"
#include "Hypergraph.h"
#include "FMRefiner.h" // Add include for ChipletRefiner
#include "KLRefiner.h" // Add include for KLRefiner
#include "Utilities.h"
#include "pugixml.hpp"
#include "OpenMPSupport.h" // Use our unified OpenMP header instead of direct include
#include "GeneticTechPartitioner.h"
#include "CanonicalGA.h" // Add include for CanonicalGA
#include "Console.h" // Include Console.h for formatting
#include <chrono>
#include <codecvt>
#include <filesystem>
#include <future> // For parallel fitness evaluation
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric> // For std::accumulate
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <mutex>   // For std::mutex
#include <atomic>  // For std::atomic
#include <algorithm>
#include <boost/algorithm/string.hpp>

// write the main here (accept arguments from command line)
namespace chiplet {

#ifndef DISABLE_METIS
// METIS header is now included through ChipletPart.h
#endif

// Helper function to interpret METIS error codes
#ifndef DISABLE_METIS
std::string MetisErrorString(int error) {
  switch (error) {
    case METIS_OK: return "METIS_OK: Operation completed successfully";
    case METIS_ERROR_INPUT: return "METIS_ERROR_INPUT: Error in the input parameters";
    case METIS_ERROR_MEMORY: return "METIS_ERROR_MEMORY: Memory allocation failed";
    case METIS_ERROR: return "METIS_ERROR: Other errors";
    default: return "Unknown METIS error code: " + std::to_string(error);
  }
}
#endif

std::vector<int> ChipletPart::CrossBarExpansion(std::vector<int> &crossbars,
                                                int &num_parts) {
  // Early validation
  if (crossbars.size() < num_parts) {
    std::cerr << "Warning: Not enough crossbars (" << crossbars.size() 
              << ") for requested partitions (" << num_parts << ")" << std::endl;
    return {};
  }

  const int num_vertices = hypergraph_->GetNumVertices();
  const int large_graph_threshold = 5000; // Threshold for using parallel methods
  
  // Log start time for performance monitoring
  auto start_time = std::chrono::high_resolution_clock::now();
  
  // Initialize data structures
  std::vector<std::unordered_set<int>> partitions(num_parts);
  std::vector<std::queue<int>> queues(num_parts);
  std::unordered_map<int, int> vertex_to_partition;
  std::vector<std::unordered_map<int, int>> edge_counts(num_vertices);
  
  // Assign crossbars to partitions - prioritize high-degree nodes for better balance
  std::vector<std::pair<int, int>> crossbar_degrees;
  crossbar_degrees.reserve(crossbars.size());
  
  for (int crossbar : crossbars) {
    crossbar_degrees.emplace_back(crossbar, hypergraph_->GetNeighbors(crossbar).size());
  }
  
  // Sort crossbars by degree in descending order
  std::sort(crossbar_degrees.begin(), crossbar_degrees.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
  
  // Assign in round-robin fashion but starting with highest degree nodes
  for (int i = 0; i < std::min(num_parts, static_cast<int>(crossbar_degrees.size())); i++) {
    int partition = i;
    int crossbar = crossbar_degrees[i].first;
    
    partitions[partition].insert(crossbar);
    queues[partition].push(crossbar);
    vertex_to_partition[crossbar] = partition;
  }
  
  // Process BFS queues until all are empty
  bool queues_active = true;
  while (queues_active) {
    queues_active = false;
    
    for (int partition = 0; partition < num_parts; partition++) {
      auto& queue = queues[partition];
      
      // Process a limited batch per partition for more balanced expansion
      const int batch_size = 5;
      int processed = 0;
      
      while (!queue.empty() && processed < batch_size) {
        queues_active = true;
        processed++;
        
        int current = queue.front();
        queue.pop();
        
        // Choose serial or parallel method based on graph size and neighbor count
        const std::vector<int>& neighbors = hypergraph_->GetNeighbors(current);
        if (num_vertices > large_graph_threshold && neighbors.size() > 100) {
          // Use parallel version for large graphs with many neighbors
          processNeighborsParallel(current, partition, partitions, queues, 
                        vertex_to_partition, edge_counts);
        } else {
          // Use serial version for smaller graphs
          processNeighbors(current, partition, partitions, queues, 
                        vertex_to_partition, edge_counts);
        }
      }
    }
  }
  
  // Create final partition assignment
  std::vector<int> result(num_vertices, -1); // -1 indicates unassigned
  
  for (int p = 0; p < num_parts; p++) {
    for (int v : partitions[p]) {
      result[v] = p;
    }
  }
  
  // Assign any remaining unassigned vertices to the nearest partition
  if (num_vertices > large_graph_threshold) {
    // Use parallel version for large graphs
    assignRemainingVerticesParallel(result, vertex_to_partition, num_parts);
  } else {
    // Use serial version for smaller graphs
    assignRemainingVertices(result, vertex_to_partition, num_parts);
  }
  
  // Log performance metrics
  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;
  std::cout << "[INFO] CrossBarExpansion completed in " << elapsed.count() 
            << " seconds for " << num_vertices << " vertices" << std::endl;
  
  return result;
}

// Helper method to process neighbors of a vertex
void ChipletPart::processNeighbors(int vertex, int partition,
                                 std::vector<std::unordered_set<int>>& partitions,
                                 std::vector<std::queue<int>>& queues,
                                 std::unordered_map<int, int>& vertex_to_partition,
                                 std::vector<std::unordered_map<int, int>>& edge_counts) {
  const std::vector<int>& neighbors = hypergraph_->GetNeighbors(vertex);
  
        for (int neighbor : neighbors) {
    // Skip if already in a partition
    if (vertex_to_partition.find(neighbor) != vertex_to_partition.end()) {
      continue;
    }
    
    // Increment edge count between this neighbor and current partition
    edge_counts[neighbor][partition]++;
    
    // Check if neighbor should be added to this partition
    if (shouldAddToPartition(neighbor, edge_counts[neighbor], vertex_to_partition)) {
      partitions[partition].insert(neighbor);
      queues[partition].push(neighbor);
      vertex_to_partition[neighbor] = partition;
    }
  }
}

// Helper method to process neighbors of a vertex in parallel
void ChipletPart::processNeighborsParallel(int vertex, int partition,
                                         std::vector<std::unordered_set<int>>& partitions,
                                         std::vector<std::queue<int>>& queues,
                                         std::unordered_map<int, int>& vertex_to_partition,
                                         std::vector<std::unordered_map<int, int>>& edge_counts) {
  const std::vector<int>& neighbors = hypergraph_->GetNeighbors(vertex);
  
  // Use OpenMP for parallel processing of large neighborhood
  // First collect information in thread-local structures
  std::vector<int> candidates_to_add;
  candidates_to_add.reserve(neighbors.size());
  
  // Phase 1: Process all neighbors in parallel to find candidates
  #pragma omp parallel
  {
    std::vector<int> local_candidates;
    
    #pragma omp for nowait
    for (size_t i = 0; i < neighbors.size(); i++) {
      int neighbor = neighbors[i];
      
      // Skip if already in a partition (thread-safe read access)
      bool already_assigned = false;
      #pragma omp critical(vertex_partition_check)
      {
        already_assigned = (vertex_to_partition.find(neighbor) != vertex_to_partition.end());
      }
      
      if (already_assigned) {
        continue;
      }
      
      // Increment edge count between this neighbor and current partition
      #pragma omp critical(edge_counts_update)
      {
        edge_counts[neighbor][partition]++;
      }
      
      // Check if neighbor should be added to this partition
      // First collect all the edge counts for this neighbor in thread-local copy
      std::unordered_map<int, int> neighbor_edge_counts;
      #pragma omp critical(collect_edge_counts)
      {
        neighbor_edge_counts = edge_counts[neighbor];
      }
      
      // Thread-safe read of vertex_to_partition
      std::unordered_map<int, int> vertex_partition_copy;
      #pragma omp critical(vertex_partition_read)
      {
        vertex_partition_copy = vertex_to_partition;
      }
      
      if (shouldAddToPartition(neighbor, neighbor_edge_counts, vertex_partition_copy)) {
        local_candidates.push_back(neighbor);
      }
    }
    
    // Merge local candidates with shared candidates list
    if (!local_candidates.empty()) {
      #pragma omp critical(merge_candidates)
      {
        candidates_to_add.insert(candidates_to_add.end(), 
                               local_candidates.begin(), 
                               local_candidates.end());
      }
    }
  }
  
  // Phase 2: Process all candidates sequentially to avoid race conditions
  // This is done outside the parallel region to ensure thread safety
  for (int neighbor : candidates_to_add) {
    // Double-check the neighbor hasn't been assigned by another thread
    if (vertex_to_partition.find(neighbor) != vertex_to_partition.end()) {
      continue;
    }
    
    // Process the candidate neighbor
    partitions[partition].insert(neighbor);
    queues[partition].push(neighbor);
    vertex_to_partition[neighbor] = partition;
  }
}

// Determine if a vertex should be added to a partition
bool ChipletPart::shouldAddToPartition(int vertex, 
                                     const std::unordered_map<int, int>& edge_counts,
                                     const std::unordered_map<int, int>& vertex_to_partition) {
  // Already assigned
  if (vertex_to_partition.find(vertex) != vertex_to_partition.end()) {
    return false;
  }
  
  // Find partition with maximum connection strength
  int max_edges = 0;
  int best_partition = -1;
  int total_edges = 0;
  
  for (const auto& pair : edge_counts) {
    int partition = pair.first;
    int count = pair.second;
    total_edges += count;
    if (count > max_edges) {
      max_edges = count;
      best_partition = partition;
    }
  }
  
  // Only add if this partition has a significant majority (at least 60%)
  return (best_partition != -1) && (max_edges > 0.6 * total_edges);
}

// Assign any remaining unassigned vertices
void ChipletPart::assignRemainingVertices(std::vector<int>& partition, 
                                        const std::unordered_map<int, int>& vertex_to_partition,
                                        int num_parts) {
  const int num_vertices = hypergraph_->GetNumVertices();
  
  // Keep iterating until all vertices are assigned
  bool changes_made = true;
  while (changes_made) {
    changes_made = false;
    
    for (int v = 0; v < num_vertices; v++) {
      if (partition[v] != -1) continue; // Skip already assigned vertices
      
      // Find partition with most neighbors
      std::unordered_map<int, int> neighbor_partitions;
      for (int neighbor : hypergraph_->GetNeighbors(v)) {
        if (partition[neighbor] != -1) {
          neighbor_partitions[partition[neighbor]]++;
        }
      }
      
      // Assign to partition with most neighbors
      int best_partition = -1;
      int max_neighbors = 0;
      
      for (const auto& pair : neighbor_partitions) {
        int p = pair.first;
        int count = pair.second;
        if (count > max_neighbors) {
          max_neighbors = count;
          best_partition = p;
        }
      }
      
      if (best_partition != -1) {
        partition[v] = best_partition;
        changes_made = true;
      }
    }
    
    // If we still have unassigned vertices with no neighbors in any partition,
    // assign them randomly to balance partitions
    if (!changes_made) {
  std::vector<int> partition_sizes(num_parts, 0);
      for (int v = 0; v < num_vertices; v++) {
        if (partition[v] != -1) {
          partition_sizes[partition[v]]++;
        }
      }
      
      for (int v = 0; v < num_vertices; v++) {
        if (partition[v] == -1) {
          // Find smallest partition
          int smallest_partition = 0;
          for (int p = 1; p < num_parts; p++) {
            if (partition_sizes[p] < partition_sizes[smallest_partition]) {
              smallest_partition = p;
            }
          }
          
          partition[v] = smallest_partition;
          partition_sizes[smallest_partition]++;
          changes_made = true;
        }
      }
    }
  }
}

// Assign any remaining unassigned vertices in parallel
void ChipletPart::assignRemainingVerticesParallel(std::vector<int>& partition, 
                                                const std::unordered_map<int, int>& vertex_to_partition,
                                                int num_parts) {
  const int num_vertices = hypergraph_->GetNumVertices();
  const int threshold_for_parallel = 1000; // Threshold for parallel processing
  
  // Calculate partition sizes for load balancing
  std::vector<int> partition_sizes(num_parts, 0);
  for (int v = 0; v < num_vertices; v++) {
    if (partition[v] != -1) {
      partition_sizes[partition[v]]++;
    }
  }
  
  // Keep iterating until all vertices are assigned
  bool changes_made = true;
  int iterations = 0;
  const int max_iterations = 10; // Prevent infinite loops
  
  while (changes_made && iterations < max_iterations) {
    changes_made = false;
    iterations++;
    
    // Create a vector to track changes made in parallel
    std::vector<bool> local_changes(omp_utils::get_max_threads(), false);
    
    // First pass: assign based on neighbors
    if (num_vertices > threshold_for_parallel) {
      // Parallel version for large graphs
      #pragma omp parallel
      {
        int thread_id = omp_utils::get_thread_num();
        
        #pragma omp for schedule(dynamic, 64)
        for (int v = 0; v < num_vertices; v++) {
          if (partition[v] != -1) continue; // Skip already assigned vertices
          
          // Find partition with most neighbors
          std::unordered_map<int, int> neighbor_partitions;
          for (int neighbor : hypergraph_->GetNeighbors(v)) {
            if (partition[neighbor] != -1) {
              neighbor_partitions[partition[neighbor]]++;
            }
          }
          
          // Assign to partition with most neighbors
          int best_partition = -1;
          int max_neighbors = 0;
          
          for (const auto& pair : neighbor_partitions) {
            int p = pair.first;
            int count = pair.second;
            if (count > max_neighbors) {
              max_neighbors = count;
              best_partition = p;
            }
          }
          
          if (best_partition != -1) {
            partition[v] = best_partition;
            #pragma omp atomic
            partition_sizes[best_partition]++;
            local_changes[thread_id] = true;
          }
        }
      }
      
      // Check if any thread made changes
      for (bool thread_change : local_changes) {
        if (thread_change) {
          changes_made = true;
            break;
          }
        }
    } else {
      // Sequential version for small graphs
      for (int v = 0; v < num_vertices; v++) {
        if (partition[v] != -1) continue; // Skip already assigned vertices
        
        // Find partition with most neighbors
        std::unordered_map<int, int> neighbor_partitions;
        for (int neighbor : hypergraph_->GetNeighbors(v)) {
          if (partition[neighbor] != -1) {
            neighbor_partitions[partition[neighbor]]++;
          }
        }
        
        // Assign to partition with most neighbors
        int best_partition = -1;
        int max_neighbors = 0;
        
        for (const auto& pair : neighbor_partitions) {
          int p = pair.first;
          int count = pair.second;
          if (count > max_neighbors) {
            max_neighbors = count;
            best_partition = p;
          }
        }
        
        if (best_partition != -1) {
          partition[v] = best_partition;
          partition_sizes[best_partition]++;
          changes_made = true;
        }
      }
    }
    
    // If we still have unassigned vertices with no neighbors in any partition,
    // assign them randomly to balance partitions
    if (!changes_made) {
      std::vector<int> unassigned_vertices;
      
      // Collect unassigned vertices
      for (int v = 0; v < num_vertices; v++) {
        if (partition[v] == -1) {
          unassigned_vertices.push_back(v);
        }
      }
      
      if (!unassigned_vertices.empty()) {
        changes_made = true;
        
        // Find partition sizes for load balancing
        std::vector<std::pair<int, int>> sorted_partitions;
        for (int p = 0; p < num_parts; p++) {
          sorted_partitions.emplace_back(p, partition_sizes[p]);
        }
        
        // Sort partitions by size (ascending)
        std::sort(sorted_partitions.begin(), sorted_partitions.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });
        
        // Distribute unassigned vertices to smallest partitions
        #pragma omp parallel for schedule(dynamic, 16) if(unassigned_vertices.size() > threshold_for_parallel)
        for (size_t i = 0; i < unassigned_vertices.size(); i++) {
          int v = unassigned_vertices[i];
          // Assign to smallest partition (modulo operation for even distribution)
          int target_partition = sorted_partitions[i % num_parts].first;
          partition[v] = target_partition;
          
          // Update partition size (atomic to avoid race conditions)
          #pragma omp atomic
          partition_sizes[target_partition]++;
        }
      }
    }
  }
  
  // Final check for any remaining unassigned vertices
  int remaining = 0;
  for (int v = 0; v < num_vertices; v++) {
    if (partition[v] == -1) {
      remaining++;
    }
  }
  
  if (remaining > 0) {
    std::cout << "[WARNING] " << remaining << " vertices still unassigned after "
              << iterations << " iterations. Assigning to partition 0." << std::endl;
    
    // Assign any remaining vertices to partition 0
    for (int v = 0; v < num_vertices; v++) {
      if (partition[v] == -1) {
        partition[v] = 0;
      }
    }
  }
}

std::vector<int> ChipletPart::SpectralPartition() {
  const int num_vertices = hypergraph_->GetNumVertices();
  const int large_graph_threshold = 5000; // Threshold for using parallel methods
  std::vector<int> partition(num_vertices, -1);
  
  // Start timing
  auto start_time = std::chrono::high_resolution_clock::now();
  std::cout << "[INFO] Starting spectral partitioning for " << num_vertices << " vertices" << std::endl;

  // Early validation: Check for trivial cases
  if (num_vertices == 0) {
    std::cerr << "[ERROR] Empty graph in SpectralPartition" << std::endl;
  return partition;
}

  if (num_vertices <= num_parts_) {
    std::cout << "[INFO] Graph has fewer vertices than requested partitions, assigning one vertex per partition" << std::endl;
    for (int i = 0; i < num_vertices; i++) {
      partition[i] = i % num_parts_;
    }
    return partition;
  }

  // Step 1: Build the Laplacian matrix using Eigen
  Eigen::SparseMatrix<double> L(num_vertices, num_vertices);
  std::vector<Eigen::Triplet<double>> triplets;
  triplets.reserve(num_vertices * 10); // Rough estimate of non-zeros (10 connections per vertex)
  
  // Ensure deterministic behavior from Eigen
  Eigen::initParallel(); // Initialize Eigen's parallelization settings
  
  // First compute the degree matrix D and adjacency matrix A
  #pragma omp parallel if(num_vertices > large_graph_threshold)
  {
    std::vector<Eigen::Triplet<double>> local_triplets;
    
    #pragma omp for nowait
    for (int i = 0; i < num_vertices; i++) {
      const std::vector<int>& neighbors = hypergraph_->GetNeighbors(i);
      // Add diagonal element (degree)
      local_triplets.emplace_back(i, i, neighbors.size());
      // Add off-diagonal elements (-1 for each edge)
      for (int j : neighbors) {
        local_triplets.emplace_back(i, j, -1.0);
      }
    }
    
    // Merge local triplets into global list
    #pragma omp critical
    {
      triplets.insert(triplets.end(), local_triplets.begin(), local_triplets.end());
    }
  }
  
  // Build the sparse matrix from triplets
  L.setFromTriplets(triplets.begin(), triplets.end());
  L.makeCompressed();

  // Step 2: Compute eigenvalues and eigenvectors using Eigen's solver
  // Ensure the matrix is symmetric
  Eigen::SparseMatrix<double> symL = L;
  for (int k = 0; k < symL.outerSize(); ++k) {
    for (Eigen::SparseMatrix<double>::InnerIterator it(symL, k); it; ++it) {
      if (it.row() != it.col()) {
        symL.coeffRef(it.row(), it.col()) = -1.0;
      }
    }
  }
  
  // Choose solver based on matrix size
  Eigen::MatrixXd eigenvectors;
  Eigen::VectorXd eigenvalues;
  
  if (num_vertices > large_graph_threshold) {
    std::cout << "[INFO] Using sparse eigenvalue solver for large graph" << std::endl;
    // For large matrices, compute only the k+3 smallest eigenvalues/vectors
    int num_eigenvectors = std::min(num_parts_ + 3, num_vertices - 1);
    
    // Convert to dense matrix for now (for simplicity)
    // In a full implementation, use a specialized sparse solver like ARPACK
    Eigen::MatrixXd denseL = Eigen::MatrixXd(symL);
    
    // Set a deterministic computation mode for eigenvalues
    Eigen::ComputationInfo info;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigenSolver;
    eigenSolver.computeDirect(denseL, Eigen::ComputeEigenvectors);
    info = eigenSolver.info();
    
    if (info != Eigen::Success) {
      std::cerr << "[ERROR] Eigenvalue computation failed: " 
                << eigenSolver.info() << std::endl;
      return partition;
    }
    
    eigenvalues = eigenSolver.eigenvalues();
    eigenvectors = eigenSolver.eigenvectors();
  } else {
    // For smaller matrices, compute all eigenvalues/vectors
    Eigen::MatrixXd denseL = Eigen::MatrixXd(symL);
    
    // Set a deterministic computation mode for eigenvalues
    Eigen::ComputationInfo info;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigenSolver;
    eigenSolver.computeDirect(denseL, Eigen::ComputeEigenvectors);
    info = eigenSolver.info();
    
    if (info != Eigen::Success) {
      std::cerr << "[ERROR] Eigenvalue computation failed: " 
                << eigenSolver.info() << std::endl;
      return partition;
    }
    
    eigenvalues = eigenSolver.eigenvalues();
    eigenvectors = eigenSolver.eigenvectors();
  }
  
  // Step 3: Set up multi-dimensional embedding using multiple eigenvectors
  
  // Determine number of clusters
  int num_clusters = 4;
  if (num_clusters < 2) num_clusters = 2; // Ensure at least 2 clusters
  
  // Maximum number of eigenvectors we can use (limited by matrix size)
  int max_eigenvectors = std::min(num_vertices, num_clusters + 3);
  
  // Create embedding using multiple eigenvectors
  // Skip the first eigenvector (constant vector with eigenvalue 0)
  // Use the next 'num_clusters' eigenvectors for the embedding
  int embedding_dim = std::min(max_eigenvectors - 1, num_clusters);
  Eigen::MatrixXd embedding(num_vertices, embedding_dim);
  
  for (int i = 0; i < num_vertices; i++) {
    for (int j = 0; j < embedding_dim; j++) {
      // Use eigenvectors with smallest non-zero eigenvalues
      // (skip the first one which should be close to zero)
      embedding(i, j) = eigenvectors(i, j+1);
    }
  }
  
  // Normalize the embedding for better clustering results
  for (int j = 0; j < embedding_dim; j++) {
    double norm = embedding.col(j).norm();
    if (norm > 1e-10) { // Avoid division by near-zero
      embedding.col(j) /= norm;
    }
  }
  
  // Step 4: Apply k-means clustering to the embedding
  std::cout << "[INFO] Applying " << num_clusters << "-means clustering to spectral embedding with " 
            << embedding_dim << " dimensions" << std::endl;
  
  std::vector<int> clusters;
  if (num_vertices > large_graph_threshold) {
    // Use parallel k-means for large graphs
    clusters = kMeansClusteringParallel(embedding, num_clusters);
  } else {
    // Use sequential k-means for smaller graphs
    clusters = kMeansClustering(embedding, num_clusters);
  }
  
  // Assign the clusters to the partition
  for (int i = 0; i < num_vertices; i++) {
    partition[i] = clusters[i];
  }
  
  // Verify that we have a valid partition (no -1 values)
  if (std::find(partition.begin(), partition.end(), -1) != partition.end()) {
    std::cerr << "[WARNING] Invalid partition with unassigned vertices detected" << std::endl;
  }
  
  // Log completion time
  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;
  std::cout << "[INFO] Spectral partitioning with k-means completed in " 
            << elapsed.count() << " seconds" << std::endl;
  
    return partition;
  }

// K-means clustering implementation for spectral partitioning
std::vector<int> ChipletPart::kMeansClustering(const Eigen::MatrixXd& embedding, int k) {
  const int num_points = embedding.rows();
  const int dimensions = embedding.cols();
  
  // Initialize result vector
  std::vector<int> clusters(num_points, 0);
  
  // Initialize centroids randomly using the class's seeded RNG
  std::vector<int> centroid_indices;
  std::uniform_int_distribution<int> dist(0, num_points - 1);
  
  // Select k random points as initial centroids
  while (centroid_indices.size() < k) {
    int idx = dist(rng_);  // Use the class's RNG that was seeded in Partition()
    // Ensure we don't select the same point twice
    if (std::find(centroid_indices.begin(), centroid_indices.end(), idx) == centroid_indices.end()) {
      centroid_indices.push_back(idx);
    }
  }
  
  // Create centroids matrix
  Eigen::MatrixXd centroids(k, dimensions);
  for (int i = 0; i < k; i++) {
    centroids.row(i) = embedding.row(centroid_indices[i]);
  }
  
  // K-means iterations
  bool changed = true;
  const int max_iterations = 100;
  int iter = 0;
  
  while (changed && iter < max_iterations) {
    changed = false;
    iter++;
    
    // Assign points to nearest centroid
    for (int i = 0; i < num_points; i++) {
      double min_dist = std::numeric_limits<double>::max();
      int best_cluster = 0;
      
      for (int j = 0; j < k; j++) {
        // Calculate Euclidean distance
        double dist = (embedding.row(i) - centroids.row(j)).squaredNorm();
        if (dist < min_dist) {
          min_dist = dist;
          best_cluster = j;
        }
      }
      
      // Update cluster assignment if changed
      if (clusters[i] != best_cluster) {
        clusters[i] = best_cluster;
        changed = true;
      }
    }
    
    // Recompute centroids
    Eigen::MatrixXd new_centroids = Eigen::MatrixXd::Zero(k, dimensions);
    std::vector<int> counts(k, 0);
    
    for (int i = 0; i < num_points; i++) {
      new_centroids.row(clusters[i]) += embedding.row(i);
      counts[clusters[i]]++;
    }
    
    for (int j = 0; j < k; j++) {
      if (counts[j] > 0) {
        new_centroids.row(j) /= counts[j];
      } else {
        // If a cluster is empty, reinitialize its centroid
        int random_idx = dist(rng_);
        new_centroids.row(j) = embedding.row(random_idx);
        changed = true; // Force another iteration
      }
    }
    
    centroids = new_centroids;
  }
  
  std::cout << "[INFO] K-means clustering completed in " << iter << " iterations" << std::endl;
  return clusters;
}

// Parallel implementation of K-means clustering for spectral partitioning
std::vector<int> ChipletPart::kMeansClusteringParallel(const Eigen::MatrixXd& embedding, int k) {
  const int num_points = embedding.rows();
  const int dimensions = embedding.cols();
  const int large_dataset_threshold = 5000; // Threshold for parallel processing
  
  // Initialize result vector
  std::vector<int> clusters(num_points, 0);
  
  // Initialize centroids randomly using the class's seeded RNG
  std::vector<int> centroid_indices;
  std::uniform_int_distribution<int> dist(0, num_points - 1);
  
  // Select k random points as initial centroids
  while (centroid_indices.size() < k) {
    int idx = dist(rng_);  // Use the class's RNG that was seeded in Partition()
    // Ensure we don't select the same point twice
    if (std::find(centroid_indices.begin(), centroid_indices.end(), idx) == centroid_indices.end()) {
      centroid_indices.push_back(idx);
    }
  }
  
  // Create centroids matrix
  Eigen::MatrixXd centroids(k, dimensions);
  for (int i = 0; i < k; i++) {
    centroids.row(i) = embedding.row(centroid_indices[i]);
  }
  
  // K-means iterations
  bool changed = true;
  const int max_iterations = 100;
  int iter = 0;
  
  // Create thread-local RNGs for parallel processing
  std::vector<std::mt19937> thread_rngs;
  // Create seed sequence for each thread based on master seed
  #pragma omp parallel
  {
    #pragma omp single
    {
      thread_rngs.resize(omp_utils::get_num_threads());
      for(int i = 0; i < thread_rngs.size(); i++) {
        // Create deterministic seeds for each thread based on master seed
        thread_rngs[i].seed(seed_ + i); 
      }
    }
  }
  
  while (changed && iter < max_iterations) {
    changed = false;
    iter++;
    
    // For large datasets, use parallel processing for point assignment
    if (num_points > large_dataset_threshold) {
      // Array to track if any assignment changed
      std::vector<bool> local_changed(omp_utils::get_max_threads(), false);
      
      // Assign points to nearest centroid in parallel
      #pragma omp parallel
      {
        int thread_id = omp_utils::get_thread_num();
        
        #pragma omp for schedule(dynamic, 128)
        for (int i = 0; i < num_points; i++) {
          double min_dist = std::numeric_limits<double>::max();
          int best_cluster = 0;
          
          for (int j = 0; j < k; j++) {
            // Calculate Euclidean distance
            double dist = (embedding.row(i) - centroids.row(j)).squaredNorm();
            if (dist < min_dist) {
              min_dist = dist;
              best_cluster = j;
            }
          }
          
          // Update cluster assignment if changed
          if (clusters[i] != best_cluster) {
            clusters[i] = best_cluster;
            local_changed[thread_id] = true;
          }
        }
      }
      
      // Check if any thread detected a change
      for (bool thread_change : local_changed) {
        if (thread_change) {
          changed = true;
          break;
        }
      }
    } else {
      // For smaller datasets, use the original sequential approach
      for (int i = 0; i < num_points; i++) {
        double min_dist = std::numeric_limits<double>::max();
        int best_cluster = 0;
        
        for (int j = 0; j < k; j++) {
          // Calculate Euclidean distance
          double dist = (embedding.row(i) - centroids.row(j)).squaredNorm();
          if (dist < min_dist) {
            min_dist = dist;
            best_cluster = j;
          }
        }
        
        // Update cluster assignment if changed
        if (clusters[i] != best_cluster) {
          clusters[i] = best_cluster;
          changed = true;
        }
      }
    }
    
    // Recompute centroids
    // For each centroid, collect sum and count atomically
    Eigen::MatrixXd new_centroids = Eigen::MatrixXd::Zero(k, dimensions);
    std::vector<int> counts(k, 0);
    
    if (num_points > large_dataset_threshold) {
      // Compute sums in parallel
      std::vector<Eigen::MatrixXd> thread_sums(omp_utils::get_max_threads(), Eigen::MatrixXd::Zero(k, dimensions));
      std::vector<std::vector<int>> thread_counts(omp_utils::get_max_threads(), std::vector<int>(k, 0));
      
      #pragma omp parallel
      {
        int thread_id = omp_utils::get_thread_num();
        auto& local_sums = thread_sums[thread_id];
        auto& local_counts = thread_counts[thread_id];
        
        #pragma omp for schedule(dynamic, 128)
        for (int i = 0; i < num_points; i++) {
          local_sums.row(clusters[i]) += embedding.row(i);
          local_counts[clusters[i]]++;
        }
      }
      
      // Merge results from all threads
      for (int t = 0; t < omp_utils::get_max_threads(); t++) {
        new_centroids += thread_sums[t];
        for (int j = 0; j < k; j++) {
          counts[j] += thread_counts[t][j];
        }
      }
    } else {
      // Sequential version for small datasets
      for (int i = 0; i < num_points; i++) {
        new_centroids.row(clusters[i]) += embedding.row(i);
        counts[clusters[i]]++;
      }
    }
    
    // Compute new centroids
    for (int j = 0; j < k; j++) {
      if (counts[j] > 0) {
        new_centroids.row(j) /= counts[j];
      } else {
        // If a cluster is empty, reinitialize its centroid
        int random_idx = dist(rng_);
        new_centroids.row(j) = embedding.row(random_idx);
        changed = true; // Force another iteration
      }
    }
    
    centroids = new_centroids;
  }
  
  std::cout << "[INFO] Parallel K-means clustering completed in " << iter << " iterations" << std::endl;
  
  // Check for empty clusters
  std::set<int> unique_clusters(clusters.begin(), clusters.end());
  if (unique_clusters.size() < k) {
    std::cout << "[WARNING] K-means found only " << unique_clusters.size() 
              << " clusters out of " << k << " requested" << std::endl;
  }
  
  return clusters;
}

void ChipletPart::ReadChipletGraph(std::string hypergraph_file,
                                   std::string chiplet_io_file) {
  // read the chiplet_io_file_
  if (!chiplet_io_file.empty()) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(chiplet_io_file.c_str());
    if (!result) {
      std::cerr << "Error: " << result.description() << std::endl;
      return;
    }
    pugi::xml_node ios = doc.child("ios");
    for (pugi::xml_node io = ios.child("io"); io; io = io.next_sibling("io")) {
      // Read the 'type' attribute
      std::string type = io.attribute("type").as_string();

      // Read the 'reach' attribute
      double reach = io.attribute("reach").as_double();

      // Add to hash map
      io_map_[type] = reach;
    }
  }

  // read hypergraph file
  std::ifstream hypergraph_file_input(hypergraph_file);
  if (!hypergraph_file_input.is_open()) {
    std::cerr << "Error: Cannot open hypergraph file " << hypergraph_file
              << std::endl;
    return;
  }
  // Check the number of vertices, number of hyperedges, weight flag
  std::string cur_line;
  std::getline(hypergraph_file_input, cur_line);
  std::istringstream cur_line_buf(cur_line);
  std::vector<int> stats{std::istream_iterator<int>(cur_line_buf),
                         std::istream_iterator<int>()};
  num_hyperedges_ = stats[0];
  num_vertices_ = stats[1];
  bool hyperedge_weight_flag = false;
  bool vertex_weight_flag = false;
  if (stats.size() == 3) {
    if ((stats[2] % 10) == 1) {
      hyperedge_weight_flag = true;
    }
    if (stats[2] >= 10) {
      vertex_weight_flag = true;
    }
  }

  // clear the related vectors
  hyperedges_.clear();
  hyperedge_weights_.clear();
  vertex_weights_.clear();
  hyperedges_.reserve(num_hyperedges_);
  hyperedge_weights_.reserve(num_hyperedges_);
  vertex_weights_.reserve(num_vertices_);
  std::unordered_map<long long, std::vector<int>> distinct_hyperedges;

  // Read hyperedge information
  for (int i = 0; i < num_hyperedges_; i++) {
    std::getline(hypergraph_file_input, cur_line);
    if (hyperedge_weight_flag == true) {
      std::istringstream cur_line_buf(cur_line);
      std::vector<float> hvec{std::istream_iterator<float>(cur_line_buf),
                              std::istream_iterator<float>()};
      // each line is wt reach {vertex list}
      // the first element is the weight, the second element is the reach
      // the remaining elements are the vertex list
      std::vector<float> hwt = {hvec[0]};
      float reach = hvec[1];
      float io_size = hvec[2];
      io_sizes_.push_back(io_size);
      reach_.push_back(reach);
      std::vector<int> hyperedge(hvec.begin() + 3, hvec.end());
      long long hash = 0;
      for (auto &value : hyperedge) {
        value--; // the vertex id starts from 1 in the hypergraph file
        hash += value * value;
      }

      if (distinct_hyperedges.find(hash) != distinct_hyperedges.end()) {
        continue;
      } else {
        distinct_hyperedges[hash] = hyperedge;
      }

      hyperedge_weights_.push_back(hwt);
      hyperedges_.push_back(hyperedge);
    } else {
      std::istringstream cur_line_buf(cur_line);
      std::vector<int> hyperedge{std::istream_iterator<int>(cur_line_buf),
                                 std::istream_iterator<int>()};
      for (auto &value : hyperedge) {
        value--; // the vertex id starts from 1 in the hypergraph file
      }
      std::vector<float> hwts(hyperedge_dimensions_,
                              1.0); // each dimension has the same weight
      hyperedge_weights_.push_back(hwts);
      hyperedges_.push_back(hyperedge);
    }
  }

  // Read weight for vertices
  for (int i = 0; i < num_vertices_; i++) {
    if (vertex_weight_flag == true) {
      std::getline(hypergraph_file_input, cur_line);
      std::istringstream cur_line_buf(cur_line);
      std::vector<float> vwts{std::istream_iterator<float>(cur_line_buf),
                              std::istream_iterator<float>()};
      vertex_weights_.push_back(vwts);
    } else {
      std::vector<float> vwts(vertex_dimensions_, 1.0);
      vertex_weights_.push_back(vwts);
    }
  }

  hypergraph_ = std::make_shared<Hypergraph>(
      vertex_dimensions_, hyperedge_dimensions_, hyperedges_, vertex_weights_,
      hyperedge_weights_, reach_, io_sizes_);
  std::cout << "[INFO] Number of IP blocks in chiplet graph: " << num_vertices_
            << std::endl;
  std::cout << "[INFO] Number of nets in chiplet graph: " << hyperedges_.size()
            << std::endl;
}

void ChipletPart::TechAssignPartition(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::vector<std::string> techs) {
  // seed the rng_ with 0
  rng_.seed(seed_);
  auto start_time = std::chrono::high_resolution_clock::now();
  // Generate the hypergraph from XML files
  ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);

  GeneticTechPart(chiplet_io_file, chiplet_layer_file, chiplet_wafer_process_file,
              chiplet_assembly_process_file, chiplet_test_file, chiplet_netlist_file,
              chiplet_blocks_file, reach, separation, techs);
  /*GeneticPart(chiplet_io_file, chiplet_layer_file, chiplet_wafer_process_file,
              chiplet_assembly_process_file, chiplet_test_file, chiplet_netlist_file,
              chiplet_blocks_file, reach, separation, techs);*/

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_time = end_time - start_time;
  std::cout << "[INFO] Total time taken for tech-aware ChipletPart: "
            << elapsed_time.count() << "s" << std::endl;
}

void ChipletPart::CreateMatingPool(
    const std::vector<std::vector<std::string>> &population,
    const std::vector<float> &fitness,
    std::vector<std::vector<std::string>> &mating_pool) {

  mating_pool.clear(); // Clear existing entries in mating pool
  std::vector<int> indices(population.size());
  for (size_t i = 0; i < indices.size(); ++i) {
    indices[i] = i;
  }

  // Sorting indices based on fitness values (descending order)
  std::sort(indices.begin(), indices.end(), [&](const int &a, const int &b) {
    return fitness[a] < fitness[b];
  });

  // Select the best individuals to fill the mating pool
  mating_pool.reserve(population_size_);

  int idx = 0;
  while (mating_pool.size() < population_size_) {
    mating_pool.push_back(population[indices[idx]]);
    idx = (idx + 1) % indices.size(); // Wrap around if needed
  }
}

void ChipletPart::GeneticTechPart(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, 
    std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, 
    std::string chiplet_test_file,
    std::string chiplet_netlist_file, 
    std::string chiplet_blocks_file,
    float reach, 
    float separation, 
    std::vector<std::string> &tech_nodes,
    int population_size,
    int num_generations,
    float mutation_rate,
    float crossover_rate,
    int min_partitions,
    int max_partitions,
    std::string output_prefix) {

  // Ensure we have a random seed set
  rng_.seed(seed_);
  
  // Print input parameters with better formatting
  Console::Header("Running Enhanced Genetic Algorithm for Co-optimization");
  
  // Create a table for input parameters
  std::vector<std::string> columns = {"Parameter", "Value"};
  std::vector<int> widths = {30, 50};
  Console::TableHeader(columns, widths);
  
  Console::TableRow({"IO file", chiplet_io_file}, widths);
  Console::TableRow({"Layer file", chiplet_layer_file}, widths);
  Console::TableRow({"Wafer process file", chiplet_wafer_process_file}, widths);
  Console::TableRow({"Assembly process file", chiplet_assembly_process_file}, widths);
  Console::TableRow({"Test file", chiplet_test_file}, widths);
  Console::TableRow({"Netlist file", chiplet_netlist_file}, widths);
  Console::TableRow({"Blocks file", chiplet_blocks_file}, widths);
  Console::TableRow({"Reach", std::to_string(reach)}, widths);
  Console::TableRow({"Separation", std::to_string(separation)}, widths);
  Console::TableRow({"Population Size", std::to_string(population_size)}, widths);
  Console::TableRow({"Number of Generations", std::to_string(num_generations)}, widths);
  Console::TableRow({"Mutation Rate", std::to_string(mutation_rate)}, widths);
  Console::TableRow({"Crossover Rate", std::to_string(crossover_rate)}, widths);
  Console::TableRow({"Min Partitions", std::to_string(min_partitions)}, widths);
  Console::TableRow({"Max Partitions", std::to_string(max_partitions)}, widths);
  Console::TableRow({"Output Prefix", output_prefix}, widths);
  Console::TableRow({"Random Seed", std::to_string(seed_)}, widths);
  std::cout << std::endl;
  
  // Ensure hypergraph is built
  if (hypergraph_ == nullptr) {
    Console::Info("Building hypergraph from provided files...");
    ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);
    
    if (hypergraph_ == nullptr) {
      Console::Error("Failed to create hypergraph from input files!");
      return;
    }
  }
  
  Console::Info("Creating GeneticTechPartitioner with hypergraph containing " + 
           std::to_string(hypergraph_->GetNumVertices()) + " vertices and " +
           std::to_string(hypergraph_->GetNumHyperedges()) + " hyperedges");
  
  // Create the genetic tech partitioner with our hypergraph
  GeneticTechPartitioner partitioner(
      hypergraph_,
      tech_nodes,
      ub_factor_,
      seed_,
      num_generations,
      population_size,
      mutation_rate,
      crossover_rate,
      min_partitions,
      max_partitions
  );
  
  // Set the pointer to this ChipletPart instance for advanced partitioning methods
  partitioner.SetChipletPart(this);
  
  // Run the genetic algorithm
  Console::Info("Running genetic algorithm...");
  auto start_time = std::chrono::high_resolution_clock::now();
  
  GeneticSolution solution = partitioner.Run(
      chiplet_io_file,
      chiplet_layer_file,
      chiplet_wafer_process_file,
      chiplet_assembly_process_file,
      chiplet_test_file,
      chiplet_netlist_file,
      chiplet_blocks_file,
      reach,
      separation
  );
  
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
  
  // Print results
  Console::Header("Genetic Algorithm Results");
  columns = {"Metric", "Value"};
  Console::TableHeader(columns, widths);
  
  Console::TableRow({"Best Cost", std::to_string(solution.cost)}, widths);
  Console::TableRow({"Number of Partitions", std::to_string(solution.num_partitions)}, widths);
  Console::TableRow({"Valid Solution", solution.valid ? "Yes" : "No"}, widths);
  Console::TableRow({"Execution Time (seconds)", std::to_string(duration)}, widths);
  std::cout << std::endl;
  
  // Save results to files
  output_prefix = chiplet_netlist_file + "." + output_prefix;
  partitioner.SaveResults(solution, output_prefix);
  
  // Store the solution in our class for further use if needed
  solution_ = solution.partition;
  num_parts_ = solution.num_partitions;
  
  Console::Success("Enhanced genetic algorithm completed successfully!");
}


void ChipletPart::GeneticPart(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::vector<std::string> &tech_nodes) {

  // Generate the hypergraph from XML files
  ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);

  int no_improvement_counter = 0;
  int max_no_improvement_generations = 2;
  const std::string hseparator(60, '='); // Creates a separator line
  std::cout << hseparator << std::endl;
  std::cout << std::setw((hseparator.size() + 10) / 2)
            << "RUNNING GENETIC ALGORITHM! " << std::endl;
  std::cout << hseparator << std::endl;

  // the objective of the genetic algorithm is to find
  // a near optimal assignment of tech nodes to partitions
  // such that the cost of the assignment is minimized
  // the cost of running a partition with a given tech
  // node is obtained from the function QuickPart

  // the genetic algorithm is implemented as follows:
  // 1. generate the initial population
  // 2. evaluate the fitness of each individual
  // 3. select the best individuals
  // 4. crossover the best individuals
  // 5. mutate the best individuals
  // 6. repeat steps 2-5 until the stopping criteria is met

  // the stopping criteria is met when the number of generations
  // is reached or the cost of the best individual does not change
  // for a given number of generations or an early threshold is set

  auto CrossOver = [this](std::vector<std::string> &parent1,
                          std::vector<std::string> &parent2) {
    // Determine the minimum size of the two parents
    size_t min_size = std::min(parent1.size(), parent2.size());

    // Create the child with the size of the shorter parent
    std::vector<std::string> child(min_size);

    // Generate a crossover point based on the shorter parent
    std::uniform_int_distribution<int> uni_dist(0, min_size - 1);
    int c = uni_dist(rng_);

    // Perform the crossover, up to the min size
    for (size_t i = 0; i < min_size; ++i) {
      child[i] = (i < c) ? parent1[i] : parent2[i];
    }

    return child;
  };

  auto Mutate = [this](std::vector<std::string> &individual,
                       std::vector<std::string> &tech_nodes) {
    std::uniform_int_distribution<int> part_dist(0, num_parts_ - 1);
    std::uniform_int_distribution<int> tech_dist(0, tech_nodes.size() - 1);
    individual[part_dist(rng_)] = tech_nodes[tech_dist(rng_)];
  };

  // step 1 begins

  std::vector<std::vector<std::string>> population;
  std::vector<float> fitness;
  std::vector<std::string> best_individual;
  std::vector<int> best_partition;
  float best_cost = std::numeric_limits<float>::max();
  int num_generations = 20;
  int num_individuals = 10;
  // Use the already-set seed from seed_ parameter
  num_parts_ = tech_nodes.size();

  // assign the three technodes to num_parts_
  for (int i = 0; i < num_individuals; i++) {
    std::vector<std::string> individual;
    // Use rng_ rather than rand()
    int num_parts = std::uniform_int_distribution<int>(1, 8)(rng_);
    for (int j = 0; j < num_parts; j++) {
      individual.push_back(tech_nodes[std::uniform_int_distribution<int>(0, tech_nodes.size() - 1)(rng_)]);
    }
    population.push_back(individual);
  }

  bool done = false;
  const std::string separator(60, '*'); // Creates a separator line
  for (int i = 0; i < num_generations_; ++i) {
    std::cout << separator << std::endl;
    std::cout << "[INFO] Starting [generation " << i << "] with [population "
              << population.size() << "]" << std::endl;
    std::cout << separator << std::endl;
    // print the technodes for this generation
    for (int j = 0; j < population.size(); ++j) {
      std::cout << "[INFO] Individual " << j << ": ";
      for (auto &tech : population[j]) {
        std::cout << tech << " ";
      }
      std::cout << std::endl;
    }
    if (population.size() < num_individuals) {
      std::cerr << "Error: Population size is less than the number of "
                   "individuals"
                << std::endl;
      return;
    }
    // 1. Evaluate fitness
    for (int j = 0; j < population.size(); ++j) {
      // print the individual
      auto part_tuple =
          QuickPart(chiplet_io_file, chiplet_layer_file,
                    chiplet_wafer_process_file, chiplet_assembly_process_file,
                    chiplet_test_file, chiplet_netlist_file,
                    chiplet_blocks_file, reach, separation, population[j]);
      float cost = std::get<0>(part_tuple);
      std::cout << "[INFO] Cost for individual " << j << " is " << cost
                << std::endl;
      std::vector<int> part = std::get<1>(part_tuple);
      int num_parts = std::set<int>(part.begin(), part.end()).size();
      fitness.push_back(cost);
      if (cost < best_cost) {
        best_cost = cost;
        best_partition = std::get<1>(part_tuple);
        best_individual = population[j];
      }
    }

    // print the best cost for this generation and its tech assignment
    std::cout << "[INFO] Best cost for generation " << i << " is " << best_cost
              << " for "
              << *std::max_element(best_partition.begin(),
                                   best_partition.end()) +
                     1
              << " partitions" << std::endl;

    std::cout << "[INFO] Best individual for generation " << i << ": ";
    for (int j = 0; j < best_individual.size(); j++) {
      std::cout << best_individual[j] << " ";
    }

    std::cout << std::endl;

    // 2. Selection
    std::vector<std::vector<std::string>> mating_pool;
    CreateMatingPool(population, fitness, mating_pool);

    // 3. Crossover
    std::vector<std::vector<std::string>> new_population;

    for (size_t j = 0; j < mating_pool.size() / 2; ++j) {
      // Debug prints for mating pool access
      if (2 * j < mating_pool.size() && 2 * j + 1 < mating_pool.size()) {
        std::vector<std::string> parent1 = mating_pool[2 * j];
        std::vector<std::string> parent2 = mating_pool[2 * j + 1];
        std::vector<std::string> child1 = CrossOver(parent1, parent2);
        std::vector<std::string> child2 = CrossOver(parent2, parent1);
        new_population.push_back(child1);
        new_population.push_back(child2);
      } else {
        std::cout << "Error: Out of bounds access to mating pool!" << std::endl;
        break;
      }
    }

    // 4. Mutation
    std::uniform_real_distribution<float> mutation_dist(0.0, 1.0);
    for (auto &individual : new_population) {
      if (mutation_dist(rng_) < mutation_rate_) {
        Mutate(individual, tech_nodes);
      }
    }

    // Optional: Elitism - Retain some of the best individuals from the old
    // population
    int elitism_count = 2; // Keep the top 2 individuals

    // sort the population based on fitness
    std::vector<int> indices(population.size());
    for (size_t j = 0; j < indices.size(); ++j) {
      indices[j] = j;
    }

    // Sorting indices based on fitness values (ascending order)

    std::sort(indices.begin(), indices.end(), [&](const int &a, const int &b) {
      return fitness[a] < fitness[b];
    });

    // now sort the population based on the indices
    std::vector<std::vector<std::string>> sorted_population;
    std::vector<float> sorted_fitness;
    for (size_t j = 0; j < indices.size(); ++j) {
      sorted_population.push_back(population[indices[j]]);
      sorted_fitness.push_back(fitness[indices[j]]);
    }

    population = sorted_population;

    // Keep the elite individuals
    std::vector<std::vector<std::string>> elite_individuals(
        population.begin(), population.begin() + elitism_count);

    // Replace the old population with the new population, but add back the
    // elite
    new_population.insert(new_population.end(), elite_individuals.begin(),
                          elite_individuals.end());
    population = new_population;

    // Sort population by fitness if not already sorted
    std::sort(fitness.begin(), fitness.end(), std::greater<float>());

    if (i > gen_threshold_) {
      if (best_cost <= fitness[0]) {
        no_improvement_counter++;
      } else {
        no_improvement_counter = 0; // Reset if there's improvement
        best_cost = fitness[0];     // Update best cost
      }

      // Stop if no improvement for a certain number of generations
      if (no_improvement_counter >= max_no_improvement_generations) {
        done = true;
        break;
      }
    }

    if (done) {
      break;
    }
  }

  // write the best partition to a file
  std::ofstream partition_output("tech_assignment.chipletpart.parts." +
                                 std::to_string(num_parts_));

  for (auto &part : best_partition) {
    partition_output << part << std::endl;
  }

  partition_output.close();

  std::cout << "[INFO] Best cost for the tech assignment is " << best_cost
            << std::endl;
  std::cout << "[INFO] Best individual for the tech assignment: ";
  for (int j = 0; j < best_individual.size(); j++) {
    std::cout << best_individual[j] << " ";
  }
  std::cout << std::endl;

  // write the best individual to a file
  std::ofstream tech_assignment_file("tech_assignment.chipletpart.techs." +
                                     std::to_string(num_parts_));
  for (auto &tech : best_individual) {
    tech_assignment_file << tech << std::endl;
  }

  tech_assignment_file.close();
}

std::tuple<float, std::vector<int>> ChipletPart::InitQuickPart(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::vector<std::string> &tech_nodes) {
  Matrix<float> upper_block_balance;
  Matrix<float> lower_block_balance;
  std::vector<float> zero_vector(vertex_dimensions_, 0.0);
  // removing balance constraints
  for (int i = 0; i < num_parts_; i++) {
    upper_block_balance.emplace_back(hypergraph_->GetTotalVertexWeights());
    lower_block_balance.emplace_back(zero_vector);
  }

  num_parts_ = tech_nodes.size();
  // Do num_parts-way cuts
  std::vector<int> kway_partition = METISPart(num_parts_);
  // remove all part.* files
  std::string command = "rm -f .part.*";
  int status = system(command.c_str());

  std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);
  
  // Return the METIS partition and a placeholder cost of 0.0
  return std::make_tuple(0.0, kway_partition);
}

std::tuple<float, std::vector<int>> ChipletPart::QuickPart(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::vector<std::string> &tech_nodes) {
  rng_.seed(seed_);
  num_parts_ = tech_nodes.size();
  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();

  // Pre-allocate vectors to avoid resizing
  std::vector<std::vector<int>> init_partitions;

  // Prepare upper and lower block balance
  std::vector<float> total_weights = hypergraph_->GetTotalVertexWeights();
  Matrix<float> upper_block_balance(num_parts_, total_weights);
  Matrix<float> lower_block_balance(
      num_parts_, std::vector<float>(vertex_dimensions_, 0.0));

  if (num_parts_ == 1) {
    std::vector<int> partition(hypergraph_->GetNumVertices(), 0);
    init_partitions.push_back(partition);
  } else {
    init_partitions.push_back(METISPart(num_parts_));
  }

  // Remove .part.* files safely
  if (std::filesystem::exists(".part.*")) {
    std::filesystem::remove_all(
        ".part.*"); // C++17 feature, faster than system call
  }

  // Define variables with default values
  float best_cost = 0.0;
  int best_partition_idx = 0;
  
  float cost = 0.0; // Define cost variable with default value
  return std::make_tuple(cost, init_partitions[best_partition_idx]);
}

void ChipletPart::Partition(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::string tech) {

  rng_.seed(seed_);
  auto start_time = std::chrono::high_resolution_clock::now();

  // Print input parameters with better formatting
  Console::Header("Reading Chiplet Files and Generating Hypergraph");
  
  // Create a table for input parameters
  std::vector<std::string> columns = {"Parameter", "Value"};
  std::vector<int> widths = {30, 50};
  Console::TableHeader(columns, widths);
  
  Console::TableRow({"IO file", chiplet_io_file}, widths);
  Console::TableRow({"Layer file", chiplet_layer_file}, widths);
  Console::TableRow({"Wafer process file", chiplet_wafer_process_file}, widths);
  Console::TableRow({"Assembly process file", chiplet_assembly_process_file}, widths);
  Console::TableRow({"Test file", chiplet_test_file}, widths);
  Console::TableRow({"Netlist file", chiplet_netlist_file}, widths);
  Console::TableRow({"Blocks file", chiplet_blocks_file}, widths);
  Console::TableRow({"Reach", std::to_string(reach)}, widths);
  Console::TableRow({"Separation", std::to_string(separation)}, widths);
  Console::TableRow({"Technology", tech}, widths);
  std::cout << std::endl;
  
  // Generate the hypergraph from XML files
  ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);
  
  // set max moves to 50% of the number of vertices
  if (hypergraph_->GetNumVertices() > 200) {
    max_moves_ = static_cast<int>(hypergraph_->GetNumVertices() * 0.05);
    refine_iters_ = 1;
  } else {
    max_moves_ = static_cast<int>(hypergraph_->GetNumVertices() * 0.5);
    refine_iters_ = 3;
  }
  
  bool floorplanning = false;
  
  // Create a ChipletRefiner with cost model files to test initialization
  Console::Info("Creating ChipletRefiner with cost model files to test initialization");
  std::vector<int> reaches(hypergraph_->GetNumHyperedges(), 2);
  auto refiner = std::make_shared<chiplet::ChipletRefiner>(
      num_parts_,                   // num_parts
      refine_iters_,                // refiner_iters
      max_moves_,                   // max_move
      reaches,                      // reaches
      floorplanning,
      chiplet_io_file,              // io_file
      chiplet_layer_file,           // layer_file
      chiplet_wafer_process_file,   // wafer_process_file
      chiplet_assembly_process_file, // assembly_process_file
      chiplet_test_file,            // test_file
      chiplet_netlist_file,         // netlist_file
      chiplet_blocks_file           // blocks_file
  );
  
  if (hypergraph_->GetNumVertices() > 200) {
    refiner->SetBoundary();
  }

  // Check if cost model was initialized
  if (refiner->IsCostModelInitialized()) {
    Console::Success("Cost model was successfully initialized in the ChipletRefiner constructor");
  } else {
    Console::Error("Cost model was NOT initialized in the ChipletRefiner constructor");
  }
  
  // Check if we should use parallel methods
  const int large_graph_threshold = 5000; // Threshold for using parallel methods
  const int num_vertices = hypergraph_->GetNumVertices();
  bool use_parallel = (num_vertices > large_graph_threshold);
  
  if (use_parallel) {
    Console::Info("Using parallel algorithms for large graph with " + 
             std::to_string(num_vertices) + " vertices");
  }

  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();

  // Pre-allocate vectors to avoid resizing
  std::vector<std::vector<int>> init_partitions;
  init_partitions.reserve(10); // assuming we need to store around 10 partitions

  // Prepare upper and lower block balance
  std::vector<float> total_weights = hypergraph_->GetTotalVertexWeights();
  Matrix<float> upper_block_balance(num_parts_, total_weights);
  Matrix<float> lower_block_balance(
      num_parts_, std::vector<float>(vertex_dimensions_, 0.0));

  // Step 1: Run spectral partition - already parallelized in the implementation
  std::vector<int> init_spec_partition = SpectralPartition();
  if (!init_spec_partition.empty() &&
      std::find(init_spec_partition.begin(), init_spec_partition.end(), -1) ==
          init_spec_partition.end()) {
    Console::Info("Spectral partition obtained!");
    init_partitions.push_back(
        std::move(init_spec_partition)); // Use move to avoid copying
  } else {
    std::cerr << "[ERROR] Spectral partition failed! Ignoring this!"
              << std::endl;
  }

  // Step 2: Run CrossBarExpansion - uses parallel helper methods for large graphs
  float quantile = 0.99;
  std::vector<int> crossbars = FindCrossbars(quantile);
  Console::Info("High-degree nodes found: " + std::to_string(crossbars.size()));

  for (int i : chiplets_set_) {
    if (i == 1) {
      init_partitions.push_back(std::vector<int>(hypergraph_->GetNumVertices(),
                                                 0)); // create empty partition
    } else {
      std::vector<int> crossbar_partition =
          CrossBarExpansion(crossbars, i);
      if (!crossbar_partition.empty()) {
        init_partitions.push_back(std::move(crossbar_partition));
      }
    }
  }

  // Step 3: Run KWayCuts - use parallel version for large graphs
  for (int i : chiplets_set_) {
    if (i != 1) {
      if (use_parallel) {
        init_partitions.push_back(KWayCutsParallel(i));
      } else {
      init_partitions.push_back(KWayCuts(i));
      }
      init_partitions.push_back(METISPart(i));
    }
  }

  Console::Info("Random and METIS partitions obtained!");
  Console::Info("Total number of initial partitions: " +
           std::to_string(init_partitions.size()));

  // Step 4: Run FMRefiner for all initial partitions in parallel

  // print all parameters used for partitioning
  Console::Info("refine_iters_: " + std::to_string(refine_iters_));
  Console::Info("max_moves_: " + std::to_string(max_moves_));
  Console::Info("ub_factor_: " + std::to_string(ub_factor_));

  // Setup tech array, aspect ratios, etc. for the refiner
  std::vector<std::string> tech_array(hypergraph_->GetNumVertices(), tech);
  refiner->SetTechArray(tech_array);
  
  // Set up initial aspect ratios - just using 1.0 initially
  std::vector<float> aspect_ratios(hypergraph_->GetNumVertices(), 1.0);
  refiner->SetAspectRatios(aspect_ratios);
  
  // Set up initial x and y locations - using 0.0 initially
  std::vector<float> x_locations(hypergraph_->GetNumVertices(), 0.0);
  std::vector<float> y_locations(hypergraph_->GetNumVertices(), 0.0);
  refiner->SetXLocations(x_locations);
  refiner->SetYLocations(y_locations);
  
  // Structure to store results for each partition
  struct PartitionResult {
    int partition_idx;
    int num_parts;
    float cost;
    std::vector<int> partition;
    std::vector<float> aspect_ratios;
    std::vector<float> x_locations;
    std::vector<float> y_locations;
    bool valid;
  };
  
  // Vector to store results from all partitions
  std::vector<PartitionResult> partition_results;
  partition_results.reserve(init_partitions.size());
  
  // Mutex for thread-safe access to the results vector
  std::mutex results_mutex;
  
  // Atomic counter to track progress
  std::atomic<int> partitions_processed(0);
  
  // Determine how many threads to use
  int available_threads = omp_utils::get_max_threads();
  int num_threads = std::min(available_threads, static_cast<int>(init_partitions.size()));

  // Store initial partition costs to filter bad ones
  struct InitialPartitionInfo {
    size_t index;
    int num_parts;
    float cost;
  };
  std::vector<InitialPartitionInfo> partition_costs;
  partition_costs.reserve(init_partitions.size());
  
  // Calculate initial costs and statistics
  float total_cost = 0.0f;
  float min_cost = std::numeric_limits<float>::max();
  float max_cost = std::numeric_limits<float>::min();
  
  // Display header for initial partitions
  Console::Header("Initial Partitioning Analysis");
  
  // Set up table columns for initial partition costs
  std::vector<std::string> part_columns = {"ID", "Parts", "Cost", "Origin"};
  std::vector<int> part_widths = {8, 12, 18, 20};
  Console::TableHeader(part_columns, part_widths);
  
  for (size_t i = 0; i < init_partitions.size(); i++) {
    std::vector<int> partition_copy = init_partitions[i];
    int num_parts = *std::max_element(partition_copy.begin(), partition_copy.end()) + 1;
    float cost = refiner->GetCostFromScratch(partition_copy);
    
    // Update statistics
    total_cost += cost;
    if (cost < min_cost) {
      min_cost = cost;
    }
    if (cost > max_cost) {
      max_cost = cost;
    }
    
    // Determine partition origin
    std::string origin;
    if (i == 0 && init_partitions.size() > 0) {
      origin = "Spectral";
    } else if (i < crossbars.size() + 1) {
      origin = "Node expansion";
    } else if (i % 2 == 0 && i >= crossbars.size() + 1) {
      origin = "KWayCuts";
    } else {
      origin = "METIS";
    }
    
    // Store the cost for filtering
    partition_costs.push_back({i, num_parts, cost});
    
    // Display in table format
    Console::TableRow({
      std::to_string(i),
      std::to_string(num_parts),
      std::to_string(cost),
      origin
    }, part_widths);
  }
  std::cout << std::endl;
  
  // Calculate mean and variance
  float mean_cost = total_cost / partition_costs.size();
  float variance = 0.0f;
  
  for (const auto& p : partition_costs) {
    variance += (p.cost - mean_cost) * (p.cost - mean_cost);
  }
  variance /= partition_costs.size();
  float std_dev = std::sqrt(variance);
  
  // Analyze the distribution and filter out bad partitions
  Console::Subheader("Cost Statistics");
  
  // Display cost statistics in a table
  std::vector<std::string> stat_columns = {"Statistic", "Value"};
  std::vector<int> stat_widths = {25, 20};
  Console::TableHeader(stat_columns, stat_widths);
  
  Console::TableRow({"Minimum cost", std::to_string(min_cost)}, stat_widths);
  Console::TableRow({"Maximum cost", std::to_string(max_cost)}, stat_widths);
  Console::TableRow({"Mean cost", std::to_string(mean_cost)}, stat_widths);
  Console::TableRow({"Standard deviation", std::to_string(std_dev)}, stat_widths);
  Console::TableRow({"Range (max-min)", std::to_string(max_cost - min_cost)}, stat_widths);
  std::cout << std::endl;
  
  // Define thresholds for filtering
  // We'll consider partitions "bad" if:
  // 1. Their cost is more than X standard deviations above the mean
  // 2. OR Their cost is more than Y times the minimum cost
  float std_dev_threshold = 1.5f;   // Z-score threshold
  float relative_cost_threshold = 2.0f;  // How many times worse than the best
  
  Console::Subheader("Partition Filtering");
  Console::Info("Thresholds:");
  Console::Info("• Z-score threshold: " + std::to_string(std_dev_threshold) + 
           " (partitions with score > " + std::to_string(mean_cost + std_dev_threshold * std_dev) + " will be filtered)");
  Console::Info("• Relative threshold: " + std::to_string(relative_cost_threshold) + 
           "x (partitions with cost > " + std::to_string(min_cost * relative_cost_threshold) + " will be filtered)");
  std::cout << std::endl;
  
  // Filter partitions that are statistically bad
  std::vector<std::vector<int>> filtered_partitions;
  std::vector<InitialPartitionInfo> kept_partitions;
  
  // Always keep at least this many partitions
  const int minimum_partitions = 3;
  
  // Headers for filtering results
  std::vector<std::string> filter_columns = {"ID", "Parts", "Cost", "Z-score", "Ratio", "Decision", "Reason"};
  std::vector<int> filter_widths = {5, 8, 12, 10, 8, 12, 30};
  Console::TableHeader(filter_columns, filter_widths);
  
  // First pass: Calculate how many partitions would be kept with current thresholds
  int would_keep_count = 0;
  for (const auto& p : partition_costs) {
    float z_score = (p.cost - mean_cost) / std_dev;
    float relative_score = p.cost / min_cost;
    
    bool would_be_bad = (z_score > std_dev_threshold || relative_score > relative_cost_threshold);
    if (!would_be_bad) {
      would_keep_count++;
    }
  }
  
  // If we'd keep too few partitions, dynamically adjust thresholds
  if (would_keep_count < minimum_partitions) {
    // Sort partitions by cost
    std::vector<InitialPartitionInfo> sorted_costs = partition_costs;
    std::sort(sorted_costs.begin(), sorted_costs.end(), 
              [](const InitialPartitionInfo& a, const InitialPartitionInfo& b) {
                return a.cost < b.cost;
              });
              
    // Set thresholds to include at least minimum_partitions
    if (sorted_costs.size() >= minimum_partitions) {
      const auto& worst_good_partition = sorted_costs[minimum_partitions - 1];
      float new_relative_threshold = worst_good_partition.cost / min_cost + 0.1f; // Add small buffer
      float new_zscore_threshold = ((worst_good_partition.cost - mean_cost) / std_dev) + 0.1f;
      
      relative_cost_threshold = std::max(relative_cost_threshold, new_relative_threshold);
      std_dev_threshold = std::max(std_dev_threshold, new_zscore_threshold);
      
      Console::Warning("Adjusted thresholds to ensure minimum partitions:");
      Console::Info("• New z-score threshold: " + std::to_string(std_dev_threshold));
      Console::Info("• New relative threshold: " + std::to_string(relative_cost_threshold) + "x");
      std::cout << std::endl;
    }
  }
  
  for (size_t i = 0; i < partition_costs.size(); i++) {
    const auto& p = partition_costs[i];
    
    // Calculate z-score: how many standard deviations from the mean
    float z_score = (p.cost - mean_cost) / std_dev;
    
    // Calculate relative score: how many times worse than the best
    float relative_score = p.cost / min_cost;
    
    bool is_bad = false;
    std::string reason;
    std::string decision;
    
    // Check if this partition is an outlier - using OR instead of nested if-else
    if (z_score > std_dev_threshold) {
      is_bad = true;
      reason = "z-score too high: " + std::to_string(z_score);
      decision = Console::RED + "FILTERED OUT" + Console::RESET;
    }
    else if (relative_score > relative_cost_threshold) {
      is_bad = true;
      reason = "cost ratio too high: " + std::to_string(relative_score) + "x worse than best";
      decision = Console::RED + "FILTERED OUT" + Console::RESET;
    }
    else {
      reason = "Good quality partition";
      decision = Console::GREEN + "KEPT" + Console::RESET;
    }
    
    // If we're getting too few partitions, keep this one anyway
    if (is_bad && kept_partitions.size() < minimum_partitions) {
      is_bad = false;
      reason = "Keeping despite poor quality (min partition count)";
      decision = Console::YELLOW + "KEPT" + Console::RESET;
    }
    
    // Show the decision in table format
    Console::TableRow({
      std::to_string(p.index),
      std::to_string(p.num_parts),
      std::to_string(p.cost),
      std::to_string(z_score),
      std::to_string(relative_score),
      decision,
      reason
    }, filter_widths);
    
    if (!is_bad) {
      filtered_partitions.push_back(init_partitions[p.index]);
      kept_partitions.push_back(p);
    }
  }
  std::cout << std::endl;
  
  // Replace only if we filtered something and have enough partitions left
  if (filtered_partitions.size() < init_partitions.size() && !filtered_partitions.empty()) {
    // Display filtering results in a visually appealing way
    int filtered_count = init_partitions.size() - filtered_partitions.size();
    float filter_percent = 100.0f * filtered_count / init_partitions.size();
    
    Console::Subheader("Filtering Results");
    
    // Create a visual summary table
    std::vector<std::string> summary_columns = {"Category", "Count", "Percentage", "Visual"}; 
    std::vector<int> summary_widths = {15, 10, 15, 30};
    Console::TableHeader(summary_columns, summary_widths);
    
    // Kept partitions row
    std::string kept_visual = "[" + std::string(kept_partitions.size(), '=') + 
                            std::string(filtered_count, ' ') + "]";
    Console::TableRow({
      Console::GREEN + "Kept" + Console::RESET,
      std::to_string(kept_partitions.size()),
      std::to_string(100.0f - filter_percent) + "%",
      kept_visual
    }, summary_widths);
    
    // Filtered partitions row
    std::string filtered_visual = "[" + std::string(kept_partitions.size(), ' ') + 
                                std::string(filtered_count, '=') + "]";
    Console::TableRow({
      Console::RED + "Filtered" + Console::RESET,
      std::to_string(filtered_count),
      std::to_string(filter_percent) + "%",
      filtered_visual
    }, summary_widths);
    
    // Total row
    Console::TableRow({
      "Total",
      std::to_string(init_partitions.size()),
      "100%",
      "[" + std::string(init_partitions.size(), '=') + "]"
    }, summary_widths);
    
    std::cout << std::endl;
    
    // Create a visual bar of which partitions were kept vs filtered
    Console::Info("Partition filtering results by ID:");
    std::string id_bar = "[";
    for (size_t i = 0; i < init_partitions.size(); i++) {
      bool was_kept = false;
      for (const auto& kp : kept_partitions) {
        if (kp.index == i) {
          was_kept = true;
          break;
        }
      }
      id_bar += was_kept ? Console::GREEN + "■" + Console::RESET : Console::RED + "■" + Console::RESET;
    }
    id_bar += "]";
    
    Console::Info(id_bar);
    Console::Info(Console::GREEN + "■" + Console::RESET + " = Kept   " + 
             Console::RED + "■" + Console::RESET + " = Filtered");
    std::cout << std::endl;
    
    // Display performance impact message
    Console::Success("Reduced processing workload by " + 
             std::to_string(filter_percent) + "% (" + 
             std::to_string(filtered_count) + " fewer partitions to refine)");
    
    // Replace the original init_partitions with the filtered ones
    init_partitions = std::move(filtered_partitions);
    
    // Recalculate the number of threads based on the filtered partitions
    num_threads = std::min(available_threads, static_cast<int>(init_partitions.size()));
  } else if (filtered_partitions.empty()) {
    Console::Warning("All partitions were considered bad! Keeping original partitions.");
  } else {
    Console::Info("No partitions were filtered out - all appear to be good quality.");
  }

  Console::Header("Parallel Partitioning + Floorplanning");
  Console::Info("Running parallel floorplanning and refinement with " + 
           std::to_string(num_threads) + " threads for " + 
           std::to_string(init_partitions.size()) + " partitions");
  
  auto start_parallel_time = std::chrono::high_resolution_clock::now();
  
  // Process all partitions in parallel
  #if HAVE_OPENMP
  #pragma omp parallel num_threads(num_threads)
  {
    #pragma omp for schedule(dynamic)
    for (size_t i = 0; i < init_partitions.size(); i++) {
  #else
  for (size_t i = 0; i < init_partitions.size(); i++) {
  #endif
      // Create a thread-local copy of the refiner
      auto thread_refiner = std::make_shared<chiplet::ChipletRefiner>(
          num_parts_,                   // num_parts
          refine_iters_,                // refiner_iters
          max_moves_,                   // max_move
          std::vector<int>(hypergraph_->GetNumHyperedges(), 2), // reaches
          floorplanning,
          chiplet_io_file,              // io_file
          chiplet_layer_file,           // layer_file
          chiplet_wafer_process_file,   // wafer_process_file
          chiplet_assembly_process_file, // assembly_process_file
          chiplet_test_file,            // test_file
          chiplet_netlist_file,         // netlist_file
          chiplet_blocks_file           // blocks_file
      );

      if (hypergraph_->GetNumVertices() > 200) {
        thread_refiner->SetBoundary();
      }
      
      // Get a copy of the partition to work with
      std::vector<int> partition_copy = init_partitions[i];
      
      // Find the actual number of partitions in this specific partition
      int num_parts = *std::max_element(partition_copy.begin(), partition_copy.end()) + 1;
      
      // Ensure refiner is configured for this specific partition's number of parts
      thread_refiner->SetNumParts(num_parts);
      
      // Setup tech array for the thread-local refiner
      thread_refiner->SetTechArray(tech_array);
      thread_refiner->SetAspectRatios(aspect_ratios);
      thread_refiner->SetXLocations(x_locations);
      thread_refiner->SetYLocations(y_locations);
      
      // Set up balance constraints
      // Use the exact number of partitions found in this specific partition
      Matrix<float> upper_block_balance(num_parts, std::vector<float>(vertex_dimensions_, 0.0));
      Matrix<float> lower_block_balance(num_parts, std::vector<float>(vertex_dimensions_, 0.0));
      
      // Calculate total weights for this specific partition
      for (int j = 0; j < vertex_dimensions_; j++) {
        for (int k = 0; k < num_parts; k++) {
          upper_block_balance[k][j] = 
            std::accumulate(total_weights.begin(), total_weights.end(), 0.0f) * ub_factor_ / num_parts;
        }
      }
      
      // Run floorplanner with the correct number of partitions
      std::vector<float> result_aspect_ratios;
      std::vector<float> result_x_locations;
      std::vector<float> result_y_locations;
      bool success = true;
      if (floorplanning) {
        auto floor_result = thread_refiner->RunFloorplanner(partition_copy, hypergraph_, 100, 100, 0.00001);
        result_aspect_ratios = std::get<0>(floor_result);
        result_x_locations = std::get<1>(floor_result);
        result_y_locations = std::get<2>(floor_result);
        success = std::get<3>(floor_result);
      } else {
        // If not floorplanning, just use the initial values
        result_aspect_ratios = aspect_ratios;
        result_x_locations = x_locations;
        result_y_locations = y_locations;
      }
      
      //if (success) {
        // Update thread-local refiner with floorplanner results
        thread_refiner->SetAspectRatios(result_aspect_ratios);
        thread_refiner->SetXLocations(result_x_locations);
        thread_refiner->SetYLocations(result_y_locations);
        
        // Run refinement
        float initial_cost = thread_refiner->GetCostFromScratch(partition_copy);
        thread_refiner->Refine(hypergraph_, upper_block_balance, lower_block_balance, partition_copy);
        
        // Run KL refinement after FM refinement
        //Console::Info("Running KL refinement for partition " + std::to_string(i));
        try {
          auto kl_refiner = std::make_shared<chiplet::KLRefiner>(
              num_parts,          // num_parts
              3,                  // refiner_iters - KL often needs fewer iterations
              50,                 // max_swaps per iteration
              floorplanning       // use floorplanner
          );
          
          // Set appropriate weight scale factor to avoid excessive gain values
          kl_refiner->SetWeightScaleFactor(0.001f);
          
          // Set the FM refiner as the cost evaluator for the KL refiner
          // This ensures KL uses the same cost model as FM
          kl_refiner->SetCostEvaluator(thread_refiner);
          
          // Set floorplanner parameters if needed
          if (floorplanning) {
            kl_refiner->SetFloorplannerParams(2, 100, 20);
          }
          
          // Run KL refinement on the same partition
          kl_refiner->Refine(hypergraph_, upper_block_balance, lower_block_balance, partition_copy);
          
          //Console::Success("KL refinement completed for partition " + std::to_string(i));
        } catch (const std::exception& e) {
          Console::Error("Exception in KL refinement: " + std::string(e.what()));
        } catch (...) {
          Console::Error("Unknown exception in KL refinement");
        }
        
        float final_cost = thread_refiner->GetCostFromScratch(partition_copy);
        int final_num_parts = *std::max_element(partition_copy.begin(), partition_copy.end()) + 1;
        // Store results for this partition
        PartitionResult result;
        result.partition_idx = i;
        result.num_parts = final_num_parts;
        result.cost = final_cost;
        result.partition = partition_copy;
        result.aspect_ratios = result_aspect_ratios;
        result.x_locations = result_x_locations;
        result.y_locations = result_y_locations;
        result.valid = success;
        
        // Thread-safe update of results vector
        {
          std::lock_guard<std::mutex> lock(results_mutex);
          partition_results.push_back(result);
        }
      //}
      
      // Update progress counter
      int completed = ++partitions_processed;
      
      // Thread-safe update of progress display
      {
        std::lock_guard<std::mutex> lock(results_mutex);
        float progress = static_cast<float>(completed) / init_partitions.size();
        int num_parts_processed = *std::max_element(partition_copy.begin(), partition_copy.end()) + 1;
        float cost_processed = thread_refiner->GetCostFromScratch(partition_copy);
        //Console::Info("Processed partition has " + 
        //         std::to_string(num_parts_processed) + " parts" + " and cost: " + std::to_string(cost_processed));
        Console::Info("Processed " + std::to_string(completed) + " partitions out of " + 
                 std::to_string(init_partitions.size()) + " (" + 
                 std::to_string(static_cast<int>(progress * 100)) + "%)");
        std::cout << Console::ProgressBar(progress) << std::endl;
      }
  #if HAVE_OPENMP
    }
  }
  #else
  }
  #endif
  
  auto end_parallel_time = std::chrono::high_resolution_clock::now();
  auto elapsed_parallel_time = std::chrono::duration_cast<std::chrono::seconds>(
      end_parallel_time - start_parallel_time).count();
  
  Console::Success("Parallel floorplanning and refinement completed in " + 
           std::to_string(elapsed_parallel_time) + " seconds");
  
  // Find the best partition based on cost
  if (!partition_results.empty()) {
    // Sort results by cost (lower is better)
    std::sort(partition_results.begin(), partition_results.end(),
              [](const PartitionResult& a, const PartitionResult& b) {
                return a.cost < b.cost;
              });
    
    // Get the best result
    const auto& best_result = partition_results[0];
    
    // Display the best partition with nice formatting
    Console::Header("Best Partition Results");
    
    // Display result in a table
    std::vector<std::string> result_columns = {"Metric", "Value"};
    std::vector<int> result_widths = {30, 30};
    Console::TableHeader(result_columns, result_widths);
    
    // translate aspect ratios to string
    std::string aspect_ratios_str = "[";
    for (size_t i = 0; i < best_result.num_parts; ++i) {
      aspect_ratios_str += std::to_string(best_result.aspect_ratios[i]);
      if (i < best_result.aspect_ratios.size() - 1) {
        aspect_ratios_str += ", ";
      }
    }
    aspect_ratios_str += "]";
    Console::TableRow({"Partition index", std::to_string(best_result.partition_idx)}, result_widths);
    Console::TableRow({"Number of parts", std::to_string(best_result.num_parts)}, result_widths);
    Console::TableRow({"Cost", std::to_string(best_result.cost)}, result_widths);
    Console::TableRow({"Feasibility", best_result.valid ? "Yes" : "No"}, result_widths);
    Console::TableRow({"Aspect Ratios", aspect_ratios_str}, result_widths);
    std::cout << std::endl;
    
    // Show top 3 results if we have more than one
    if (partition_results.size() > 1) {
      Console::Subheader("Top 3 Partition Results by Cost:");
      
      std::vector<std::string> top_columns = {"Rank", "Partition Index", "Parts", "Cost", "Feasible"};
      std::vector<int> top_widths = {10, 20, 10, 20, 10};
      Console::TableHeader(top_columns, top_widths);
      
      // Show up to top 3 results
      int max_to_show = std::min(3, static_cast<int>(partition_results.size()));
      for (int i = 0; i < max_to_show; i++) {
        const auto& result = partition_results[i];
        std::string success = result.valid ? "Yes" : "No";
        Console::TableRow({
          std::to_string(i+1), 
          std::to_string(result.partition_idx),
          std::to_string(result.num_parts),
          std::to_string(result.cost),
          success,
        }, top_widths);
      }
      std::cout << std::endl;
    }
    
    // Update refiner with best results for potential future use
    refiner->SetAspectRatios(best_result.aspect_ratios);
    refiner->SetXLocations(best_result.x_locations);
    refiner->SetYLocations(best_result.y_locations);
    std::vector<int> best_partition = best_result.partition;
    auto floor_result = refiner->RunFloorplanner(
      best_partition, hypergraph_, 10000, 10000, 0.00001);
      std::string success = std::get<3>(floor_result) ? "Yes" : "No";
    Console::Info("Floorplanner results: " + success);

    std::string partition_file = chiplet_netlist_file + ".cpart." + std::to_string(best_result.num_parts);

    // Save the best partition to file
    std::ofstream partition_output(partition_file);
    if (partition_output.is_open()) {
      for (int part_id : best_result.partition) {
        partition_output << part_id << std::endl;
      }
      Console::Success("Best partition saved to " + partition_file);
    }
    
    // Output aspect ratios of best solution
    /*std::ofstream aspect_ratios_output("chipletpart.aspect_ratios");
    if (aspect_ratios_output.is_open()) {
      for (float ar : best_result.aspect_ratios) {
        aspect_ratios_output << ar << std::endl;
      }
      Console::Success("Best aspect ratios saved to best_partition.aspect_ratios");
    }
    
    // Output coordinates of best solution
    std::ofstream coords_output("chipletpart.coords");
    if (coords_output.is_open()) {
      for (size_t i = 0; i < best_result.x_locations.size(); i++) {
        coords_output << best_result.x_locations[i] << " " << best_result.y_locations[i] << std::endl;
      }
      Console::Success("Best coordinates saved to best_partition.coords");
    }*/
  } else {
    Console::Error("No valid partitions found after floorplanning and refinement");
  }

  auto end_time_stamp_global = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time_stamp_global - start_time_stamp_global;
  
  // Display a timing summary with nice formatting
  Console::Header("Timing Summary");
  
  std::vector<std::string> timing_columns = {"Stage", "Time (seconds)"};
  std::vector<int> timing_widths = {40, 20};
  Console::TableHeader(timing_columns, timing_widths);
  
  Console::TableRow({"Parallel floorplanning and refinement", std::to_string(elapsed_parallel_time)}, timing_widths);
  Console::TableRow({"Total partitioning process", std::to_string(elapsed.count())}, timing_widths);
  
  std::cout << std::endl;
  Console::Success("Partitioning completed successfully");
}

void ChipletPart::EvaluatePartition(
    std::string hypergraph_part,
    std::string chiplet_io_file, std::string chiplet_layer_file,
    std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, std::string chiplet_test_file,
    std::string chiplet_netlist_file, std::string chiplet_blocks_file,
    float reach, float separation, std::string tech) {
  std::cout << "[INFO] Reading chiplet files and generating hypergraph representation" << std::endl;
  std::cout << "[INFO] Partition file: " << hypergraph_part << std::endl;
  std::cout << "[INFO] IO file: " << chiplet_io_file << std::endl;
  std::cout << "[INFO] Layer file: " << chiplet_layer_file << std::endl;
  std::cout << "[INFO] Wafer process file: " << chiplet_wafer_process_file << std::endl;
  std::cout << "[INFO] Assembly process file: " << chiplet_assembly_process_file << std::endl;
  std::cout << "[INFO] Test file: " << chiplet_test_file << std::endl;
  std::cout << "[INFO] Netlist file: " << chiplet_netlist_file << std::endl;
  std::cout << "[INFO] Blocks file: " << chiplet_blocks_file << std::endl;
  std::cout << "[INFO] Reach: " << reach << std::endl;
  std::cout << "[INFO] Separation: " << separation << std::endl;
  std::cout << "[INFO] Tech: " << tech << std::endl;

  // Generate the hypergraph from XML files
  ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);

  auto start_time_stamp_global = std::chrono::high_resolution_clock::now();
  Matrix<float> upper_block_balance;
  Matrix<float> lower_block_balance;
  std::vector<float> zero_vector(vertex_dimensions_, 0.0);
  // removing balance constraints
  for (int i = 0; i < num_parts_; i++) {
    upper_block_balance.emplace_back(hypergraph_->GetTotalVertexWeights());
    lower_block_balance.emplace_back(zero_vector);
  }

  std::vector<int> partition;

  // read hypergraph partitioning file
  std::ifstream hypergraph_part_input(hypergraph_part);
  int num_parts = 0;
  if (!hypergraph_part_input.is_open()) {
    std::cerr << "Error: Cannot open hypergraph partition file "
              << hypergraph_part << std::endl;
    return;
  } else {
    partition = std::vector<int>(hypergraph_->GetNumVertices(), 0);
    for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
      hypergraph_part_input >> partition[i];
      num_parts = std::max(num_parts, partition[i]);
    }
  }

  num_parts_ = num_parts + 1;
  std::cout << "[INFO] Number of partitions: " << num_parts_ << std::endl;
  std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);

  // set max moves to 50% of the number of vertices
  if (hypergraph_->GetNumVertices() > 200) {
    max_moves_ = static_cast<int>(hypergraph_->GetNumVertices() * 0.05);
    refine_iters_ = 1;
  } else {
    max_moves_ = static_cast<int>(hypergraph_->GetNumVertices() * 1.0);
    refine_iters_ = 10;
  }
  
  bool floorplanning = true;
  
  // Create a ChipletRefiner with cost model files to test initialization
  Console::Info("Creating ChipletRefiner with cost model files to test initialization");
  auto refiner = std::make_shared<chiplet::ChipletRefiner>(
      num_parts_,                   // num_parts
      refine_iters_,                // refiner_iters
      max_moves_,                   // max_move
      reaches,                      // reaches
      floorplanning,
      chiplet_io_file,              // io_file
      chiplet_layer_file,           // layer_file
      chiplet_wafer_process_file,   // wafer_process_file
      chiplet_assembly_process_file, // assembly_process_file
      chiplet_test_file,            // test_file
      chiplet_netlist_file,         // netlist_file
      chiplet_blocks_file           // blocks_file
  );
  
  if (hypergraph_->GetNumVertices() > 200) {
    refiner->SetBoundary();
  }

  // Check if cost model was initialized
  if (refiner->IsCostModelInitialized()) {
    Console::Success("Cost model was successfully initialized in the ChipletRefiner constructor");
  } else {
    Console::Error("Cost model was NOT initialized in the ChipletRefiner constructor");
  }

  auto floor_result = refiner->RunFloorplanner(partition, hypergraph_, 10000, 10000, 0.00001);
      
  std::vector<float> result_aspect_ratios = std::get<0>(floor_result);
  std::vector<float> result_x_locations = std::get<1>(floor_result);
  std::vector<float> result_y_locations = std::get<2>(floor_result);
  bool success = std::get<3>(floor_result);
  refiner->SetAspectRatios(result_aspect_ratios);
  refiner->SetXLocations(result_x_locations);
  refiner->SetYLocations(result_y_locations);
  std::vector<std::string> tech_array(num_parts, tech);
  refiner->SetTechArray(tech_array);
        
  // Run refinement
  float initial_cost = refiner->GetCostFromScratch(partition);
  Console::Info("Cost of partition is " + std::to_string(initial_cost));
  Console::Info("Number of partitions is " + std::to_string(num_parts_));
  Console::Info("Floorplan feasibility is " + std::to_string(success));
}

std::vector<int> ChipletPart::FindCrossbars(float &quantile) {
  std::vector<int> degrees(hypergraph_->GetNumVertices(), 0);
  for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
    degrees[i] = hypergraph_->GetNeighbors(i).size();
  }

  std::vector<int> sorted_degrees = degrees;
  std::sort(sorted_degrees.begin(), sorted_degrees.end());

  int threshold_index = static_cast<int>(quantile * sorted_degrees.size());
  int degree_threshold = sorted_degrees[threshold_index];

  std::vector<int> crossbars;
  for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
    if (degrees[i] >= degree_threshold) {
      crossbars.push_back(i);
    }
  }

  return crossbars;
}

#ifndef DISABLE_METIS
std::vector<int> ChipletPart::METISPart(int &num_parts) {
  // Initialize METIS arrays
  idx_t nvtxs = hypergraph_->GetNumVertices();  // Number of vertices
  idx_t ncon = hypergraph_->GetVertexDimensions();  // Number of balancing constraints (weights per vertex)
  if (ncon == 0) ncon = 1;  // Must be at least 1
  
  // Create arrays for the adjacency structure
  std::vector<idx_t> xadj(nvtxs + 1);  // CSR format pointer array
  std::vector<idx_t> adjncy;           // CSR format adjacency array
  
  // Build the adjacency structure from the hypergraph
  xadj[0] = 0;
  for (int i = 0; i < nvtxs; i++) {
    const std::vector<int>& neighbors = hypergraph_->GetNeighbors(i);
    for (int neighbor : neighbors) {
      adjncy.push_back(neighbor);
    }
    xadj[i + 1] = adjncy.size();
  }
  
  // Set up vertex weights if available
  std::vector<idx_t> vwgt;
  if (hypergraph_->GetVertexDimensions() > 0) {
    vwgt.resize(nvtxs * ncon);
    for (int i = 0; i < nvtxs; i++) {
      const auto& weights = hypergraph_->GetVertexWeights(i);
      for (int j = 0; j < ncon && j < static_cast<int>(weights.size()); j++) {
        vwgt[i*ncon + j] = static_cast<idx_t>(weights[j]);
      }
    }
  }
  
  // Set up edge weights
  std::vector<idx_t> adjwgt;
  size_t total_edges = adjncy.size();
  if (hypergraph_->GetHyperedgeDimensions() > 0 && total_edges > 0) {
    adjwgt.resize(total_edges);
    
    // Computing edge weights requires more complex mapping
    // For simplicity, set all edge weights to 1 initially
    std::fill(adjwgt.begin(), adjwgt.end(), 1);
    
    // A more sophisticated approach would map hyperedge weights to edge weights
    // but this is non-trivial due to the different data structures
  }
  
  // Set up arrays for the result
  idx_t objval;  // Stores the edge-cut or communication volume
  std::vector<idx_t> part(nvtxs);  // Stores the partition assignment
  
  // Set up METIS options
  idx_t options[METIS_NOPTIONS];
  METIS_SetDefaultOptions(options);
  options[METIS_OPTION_PTYPE] = METIS_PTYPE_KWAY;  // K-way partitioning
  options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;  // Edge-cut minimization
  
  // Convert num_parts to idx_t for METIS
  idx_t n_parts = static_cast<idx_t>(num_parts);
  
  // Call METIS partitioning function
  int ret = METIS_PartGraphKway(
      &nvtxs,                      // Number of vertices
      &ncon,                       // Number of balancing constraints
      xadj.data(),                 // Adjacency structure: pointers
      adjncy.data(),               // Adjacency structure: neighbors
      vwgt.empty() ? NULL : vwgt.data(),   // Vertex weights
      NULL,                        // Size of vertices (NULL = all 1)
      adjwgt.empty() ? NULL : adjwgt.data(), // Edge weights
      &n_parts,                    // Number of parts
      NULL,                        // Target partition weights (NULL = uniform)
      NULL,                        // Allowed load imbalance (NULL = 1.03)
      options,                     // Additional options
      &objval,                     // Output: Edge-cut or communication volume
      part.data()                  // Output: Partition vector
  );
  
  if (ret != METIS_OK) {
    std::cerr << "Error: METIS partitioning failed: " << MetisErrorString(ret) << std::endl;
    return std::vector<int>();
  }
  
  // Convert idx_t to int for the return value
  std::vector<int> partition(nvtxs);
  for (int i = 0; i < nvtxs; i++) {
    partition[i] = static_cast<int>(part[i]);
  }

  return partition;
}
#else
std::vector<int> ChipletPart::METISPart(int &num_parts) {
  // Fallback implementation when METIS is disabled
  std::cout << "METIS is disabled, using random partitioning instead." << std::endl;
  std::vector<int> partition(hypergraph_->GetNumVertices());
  for (int i = 0; i < hypergraph_->GetNumVertices(); i++) {
    partition[i] = i % num_parts;
  }
  return partition;
}
#endif

std::vector<int> ChipletPart::KWayCutsParallel(int &num_parts) {
  const int num_vertices = hypergraph_->GetNumVertices();
  
  // Early validation
  if (num_vertices == 0) {
    std::cerr << "[ERROR] Empty graph in KWayCutsParallel" << std::endl;
    return {};
  }
  
  if (num_parts <= 0) {
    std::cerr << "[WARNING] Invalid number of parts in KWayCutsParallel: " << num_parts << std::endl;
    num_parts = 1;
  }

  // Performance monitoring
  auto start_time = std::chrono::high_resolution_clock::now();
  std::cout << "[INFO] Starting parallel KWayCuts partitioning for " << num_vertices 
            << " vertices into " << num_parts << " parts" << std::endl;

  // Thread-safe random number generation
  // Use only the class seed for deterministic behavior
  std::seed_seq seq{static_cast<unsigned int>(seed_)};
  std::mt19937 rng(seq); // Create a thread-shared RNG
  
  // Initialize partition arrays with capacity
  std::vector<int> partition(num_vertices, 0);
  std::vector<std::atomic<int>> partition_sizes(num_parts);
  for (int i = 0; i < num_parts; i++) {
    partition_sizes[i] = 0; // Initialize all atomic counters to 0
  }
  
  // Target sizes for balanced partitioning
  const int ideal_size = num_vertices / num_parts;
  const int remainder = num_vertices % num_parts;
  
  // Set up target sizes for each partition with remainder distributed
  std::vector<int> target_sizes(num_parts, ideal_size);
  for (int i = 0; i < remainder; i++) {
    target_sizes[i]++;
  }
  
  // Add tolerance factor from class parameter
  std::vector<int> upper_bounds = target_sizes;
  for (int i = 0; i < num_parts; i++) {
    upper_bounds[i] = std::ceil(target_sizes[i] * ub_factor_);
  }
  
  // Random initial partitioning in parallel
  std::uniform_int_distribution<int> dist(0, num_parts - 1);
  
  // Pre-create a vector of thread-local RNGs with deterministic seeds
  int num_threads;
  #pragma omp parallel
  {
    #pragma omp single
    {
      num_threads = omp_utils::get_num_threads();
    }
  }
  
  std::vector<std::mt19937> thread_rngs(num_threads);
  for (int i = 0; i < num_threads; i++) {
    // Each thread gets a deterministic seed derived from the main seed
    thread_rngs[i].seed(seed_ + i);
  }
  
  // Create thread-local RNGs for parallel sections
  #pragma omp parallel
  {
    int thread_id = omp_utils::get_thread_num();
    std::mt19937& local_rng = thread_rngs[thread_id];
    
    #pragma omp for schedule(static, 1024)
    for (int i = 0; i < num_vertices; i++) {
      int random_part = dist(local_rng);
      partition[i] = random_part;
      
      // Use atomic update for partition sizes
      partition_sizes[random_part].fetch_add(1, std::memory_order_relaxed);
    }
  }
  
  // Convert atomic partition sizes to regular ints for future use
  std::vector<int> regular_partition_sizes(num_parts);
  for (int i = 0; i < num_parts; i++) {
    regular_partition_sizes[i] = partition_sizes[i].load(std::memory_order_relaxed);
  }
  
  // Compute initial balance metric
  double initial_imbalance = 0.0;
  int max_size = *std::max_element(regular_partition_sizes.begin(), regular_partition_sizes.end());
  int min_size = *std::min_element(regular_partition_sizes.begin(), regular_partition_sizes.end());
  if (min_size > 0) {
    initial_imbalance = static_cast<double>(max_size) / min_size;
  }
  
  std::cout << "[INFO] Initial random partitioning imbalance: " << initial_imbalance << std::endl;
  
  // Create a list of vertices that need to be moved from each partition
  std::vector<std::vector<int>> vertices_to_move(num_parts);
  
  // Calculate which vertices should be moved in parallel
  #pragma omp parallel
  {
    // Thread-local vectors for each partition
    std::vector<std::vector<int>> local_vertices_to_move(num_parts);
    
    // Local copy of regular_partition_sizes to avoid race conditions
    std::vector<int> local_partition_sizes = regular_partition_sizes;
    
    // Find vertices that need to be moved from overloaded partitions
    for (int p = 0; p < num_parts; p++) {
      if (local_partition_sizes[p] > upper_bounds[p]) {
        int excess = local_partition_sizes[p] - target_sizes[p];
        local_vertices_to_move[p].reserve(excess);
        
        // Collect candidates in parallel
        std::vector<std::pair<int, int>> candidates;
        
        #pragma omp for schedule(dynamic, 512) nowait
        for (int i = 0; i < num_vertices; i++) {
          if (partition[i] == p) {
            // Calculate how many neighbors are in different partitions
            int external_connections = 0;
            for (int neighbor : hypergraph_->GetNeighbors(i)) {
              if (partition[neighbor] != p) {
                external_connections++;
              }
            }
            
            #pragma omp critical(collect_candidates)
            {
              candidates.emplace_back(i, external_connections);
            }
          }
        }
        
        // Sort and process candidates only in one thread per partition
        #pragma omp single
        {
          // Sort by external connections (descending)
          std::sort(candidates.begin(), candidates.end(), 
                    [](const auto& a, const auto& b) { return a.second > b.second; });
                    
          // Take the top vertices to move
          for (int i = 0; i < std::min(excess, static_cast<int>(candidates.size())); i++) {
            local_vertices_to_move[p].push_back(candidates[i].first);
          }
        }
      }
    }
    
    // Merge local results
    #pragma omp critical(merge_vertices_to_move)
    {
      for (int p = 0; p < num_parts; p++) {
        if (!local_vertices_to_move[p].empty()) {
          vertices_to_move[p].insert(vertices_to_move[p].end(), 
                                  local_vertices_to_move[p].begin(), 
                                  local_vertices_to_move[p].end());
        }
      }
    }
  }
  
  // Now try to balance the partitions with parallel processing
  int iteration = 0;
  int max_iterations = 50; // Limit iterations to prevent infinite loops
  bool balanced = false;
  
  while (!balanced && iteration < max_iterations) {
    balanced = true;
    iteration++;
    
    // Calculate which partitions need more vertices
    std::vector<int> under_filled;
    for (int j = 0; j < num_parts; j++) {
      if (regular_partition_sizes[j] < target_sizes[j]) {
        under_filled.push_back(j);
      }
    }
    
    if (under_filled.empty()) {
      // All partitions are at or above target, check if any are over upper bound
      bool any_over_upper = false;
      for (int j = 0; j < num_parts; j++) {
        if (regular_partition_sizes[j] > upper_bounds[j]) {
          any_over_upper = true;
          break;
        }
      }
      
      if (!any_over_upper) {
        // We've achieved balance
        break;
      }
    }
    
    // Try to move vertices from over-filled to under-filled partitions
    int moves_made = 0;
    std::mutex moves_mutex; // To protect access to shared variables
    
    // Process partitions in parallel
    #pragma omp parallel for schedule(dynamic) reduction(+:moves_made)
    for (int from_part = 0; from_part < num_parts; from_part++) {
      if (regular_partition_sizes[from_part] <= target_sizes[from_part]) {
        continue; // Skip this partition, it's not overfilled
      }
      
      auto& vertices = vertices_to_move[from_part];
      if (vertices.empty()) {
        continue;
      }
      
      std::vector<int> local_under_filled;
      {
        std::lock_guard<std::mutex> lock(moves_mutex);
        local_under_filled = under_filled; // Make a thread-local copy
      }
      
      for (int to_part : local_under_filled) {
        if (regular_partition_sizes[to_part] >= target_sizes[to_part] || vertices.empty()) {
          continue;
        }
        
        // Move a vertex
        int vertex;
        {
          std::lock_guard<std::mutex> lock(moves_mutex);
          if (vertices.empty()) break; // Check again under lock
          vertex = vertices.back();
          vertices.pop_back();
        }
        
        // Update partition assignment
        partition[vertex] = to_part;
        
        // Update partition sizes atomically
        regular_partition_sizes[from_part]--;
        regular_partition_sizes[to_part]++;
        moves_made++;
        
        balanced = false; // We made a change, so we're not balanced yet
      }
    }
    
    // If we couldn't make any moves, but still have imbalance, try less constrained moves
    if (moves_made == 0 && !balanced) {
      // Use parallel processing for finding over-capacity partitions
      std::vector<int> over_capacity;
      for (int p = 0; p < num_parts; p++) {
        if (regular_partition_sizes[p] > upper_bounds[p]) {
          over_capacity.push_back(p);
        }
      }
      
      // Find partitions that have the most room
      std::vector<std::pair<int, int>> available_space;
      for (int p = 0; p < num_parts; p++) {
        int space = upper_bounds[p] - regular_partition_sizes[p];
        if (space > 0) {
          available_space.emplace_back(p, space);
        }
      }
      
      if (available_space.empty()) {
        // No space available in any partition, relax constraints
        continue;
      }
      
      // Sort by available space (descending)
      std::sort(available_space.begin(), available_space.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
      
      // Try to move random vertices from over-capacity partitions in parallel
      #pragma omp parallel for schedule(dynamic) reduction(+:moves_made)
      for (size_t i = 0; i < over_capacity.size(); i++) {
        int from_part = over_capacity[i];
        int needed_moves = regular_partition_sizes[from_part] - upper_bounds[from_part];
        
        if (needed_moves <= 0) {
          continue;
        }
        
        // Thread-local copy for thread safety
        std::vector<std::pair<int, int>> local_available_space;
        {
          std::lock_guard<std::mutex> lock(moves_mutex);
          local_available_space = available_space;
        }
        
        if (local_available_space.empty()) {
          continue;
        }
        
        // Thread-local RNG
        std::mt19937 local_rng(seq);
        local_rng.discard(omp_utils::get_thread_num() * 1000 + i * 100);
        
        // Build a list of movable vertices
        std::vector<int> movable_vertices;
        // Using a critical section instead of nested parallel for
        for (int v = 0; v < num_vertices; v++) {
          if (partition[v] == from_part) {
            #pragma omp critical(collect_movable)
            {
              movable_vertices.push_back(v);
            }
          }
        }
        
        // Shuffle the list
        std::shuffle(movable_vertices.begin(), movable_vertices.end(), local_rng);
        
        // Try to move vertices
        for (int v : movable_vertices) {
          if (needed_moves <= 0 || local_available_space.empty()) {
            break;
          }
          
          int to_part = local_available_space[0].first;
          
          // Update partition assignment
          partition[v] = to_part;
          
          // Update local tracking
          regular_partition_sizes[from_part]--;
          regular_partition_sizes[to_part]++;
          needed_moves--;
          
          // Update available space
          local_available_space[0].second--;
          if (local_available_space[0].second <= 0) {
            local_available_space.erase(local_available_space.begin());
          } else {
            // Re-sort if needed
            int idx = 0;
            while (idx < local_available_space.size() - 1 && 
                   local_available_space[idx].second < local_available_space[idx + 1].second) {
              std::swap(local_available_space[idx], local_available_space[idx + 1]);
              idx++;
            }
          }
          
          moves_made++;
          balanced = false; // We made a change
        }
        
        // Update the shared available_space with our local version
        {
          std::lock_guard<std::mutex> lock(moves_mutex);
          available_space = local_available_space;
        }
      }
    }
    
    // If we didn't make any changes but still have imbalance, we need to relax constraints
    if (balanced && iteration > 2) {
      // Check if we're actually balanced
      bool truly_balanced = true;
      for (int p = 0; p < num_parts; p++) {
        if (regular_partition_sizes[p] > upper_bounds[p]) {
          truly_balanced = false;
          break;
        }
      }
      
      if (!truly_balanced) {
        // Increase upper bounds slightly
        double relaxation = 1.0 + (0.05 * iteration); // Gradually relax bounds
        for (int p = 0; p < num_parts; p++) {
          upper_bounds[p] = std::ceil(target_sizes[p] * ub_factor_ * relaxation);
        }
        std::cout << "[INFO] Relaxing balance constraints in iteration " << iteration << std::endl;
        balanced = false; // Force another iteration
      }
    }
  }
  
  // Compute final balance metric
  double final_imbalance = 0.0;
  max_size = *std::max_element(regular_partition_sizes.begin(), regular_partition_sizes.end());
  min_size = *std::min_element(regular_partition_sizes.begin(), regular_partition_sizes.end());
  if (min_size > 0) {
    final_imbalance = static_cast<double>(max_size) / min_size;
  }
  
  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;
  
  std::cout << "[INFO] Parallel KWayCuts partitioning completed in " << elapsed.count() 
            << " seconds with imbalance factor: " << final_imbalance << std::endl;
  
  // Final partition size statistics
  std::cout << "[INFO] Final partition sizes:" << std::endl;
  for (int p = 0; p < num_parts; p++) {
    std::cout << "  Partition " << p << ": " << regular_partition_sizes[p] 
              << " vertices (target: " << target_sizes[p] 
              << ", upper bound: " << upper_bounds[p] << ")" << std::endl;
  }

  return partition;
}

std::vector<int> ChipletPart::KWayCuts(int &num_parts) {
  const int num_vertices = hypergraph_->GetNumVertices();
  
  // Early validation
  if (num_vertices == 0) {
    std::cerr << "[ERROR] Empty graph in KWayCuts" << std::endl;
    return {};
  }
  
  if (num_parts <= 0) {
    std::cerr << "[WARNING] Invalid number of parts in KWayCuts: " << num_parts << std::endl;
    num_parts = 1;
  }


  // Use modern C++ random number generation
  std::mt19937 rng(seed_); // Use the class's seed for reproducibility
  std::uniform_int_distribution<int> dist(0, num_parts - 1);
  
  // Initialize partition arrays with capacity
  std::vector<int> partition(num_vertices, 0);
  std::vector<int> partition_sizes(num_parts, 0);
  
  // Target sizes for balanced partitioning
  const int ideal_size = num_vertices / num_parts;
  const int remainder = num_vertices % num_parts;
  
  // Set up target sizes for each partition with remainder distributed
  std::vector<int> target_sizes(num_parts, ideal_size);
  for (int i = 0; i < remainder; i++) {
    target_sizes[i]++;
  }
  
  // Add tolerance factor from class parameter
  std::vector<int> upper_bounds = target_sizes;
  for (int i = 0; i < num_parts; i++) {
    upper_bounds[i] = std::ceil(target_sizes[i] * ub_factor_);
  }
  
  // Random initial partitioning
  for (int i = 0; i < num_vertices; i++) {
    int random_part = dist(rng);
    partition[i] = random_part;
    partition_sizes[random_part]++;
  }
  
  // Simple balancing algorithm - move vertices from over-filled to under-filled partitions
  bool balanced = false;
  int iteration = 0;
  const int max_iterations = 50;
  
  while (!balanced && iteration < max_iterations) {
    balanced = true;
    iteration++;
    
    // Find over-filled and under-filled partitions
    std::vector<int> over_filled;
    std::vector<int> under_filled;
    
    for (int p = 0; p < num_parts; p++) {
      if (partition_sizes[p] > upper_bounds[p]) {
        over_filled.push_back(p);
        balanced = false;
      } else if (partition_sizes[p] < target_sizes[p]) {
        under_filled.push_back(p);
      }
    }
    
    // No more imbalance to fix
    if (over_filled.empty()) {
      break;
    }
    
    // Move vertices from over-filled partitions to under-filled ones
    for (int from_part : over_filled) {
      // If we've balanced this partition, skip it
      if (partition_sizes[from_part] <= upper_bounds[from_part]) {
        continue;
      }
      
      // Find vertices to move
      std::vector<int> movable_vertices;
      for (int v = 0; v < num_vertices; v++) {
        if (partition[v] == from_part) {
          movable_vertices.push_back(v);
        }
      }
      
      // Shuffle to randomize which vertices we move
      std::shuffle(movable_vertices.begin(), movable_vertices.end(), rng);
      
      // Try to move vertices to under-filled partitions
      for (int v : movable_vertices) {
        // If we've balanced this partition, exit the loop
        if (partition_sizes[from_part] <= upper_bounds[from_part]) {
          break;
        }
        
        // Find a suitable destination partition
        for (int to_part : under_filled) {
          if (partition_sizes[to_part] < target_sizes[to_part]) {
            // Move the vertex
            partition[v] = to_part;
            partition_sizes[from_part]--;
            partition_sizes[to_part]++;
            break;
          }
        }
      }
    }
  }
  
  return partition;
}

void ChipletPart::ReadChipletGraphFromXML(std::string chiplet_io_file,
                                        std::string chiplet_netlist_file,
                                        std::string chiplet_blocks_file) {
  // First read the IO file to get the reach values for each IO type
  if (!chiplet_io_file.empty()) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(chiplet_io_file.c_str());
    if (!result) {
      std::cerr << "Error parsing IO file: " << result.description() << std::endl;
      return;
    }
    
    pugi::xml_node ios = doc.child("ios");
    for (pugi::xml_node io = ios.child("io"); io; io = io.next_sibling("io")) {
      // Read the 'type' attribute
      std::string type = io.attribute("type").as_string();

      // Read the 'reach' attribute
      double reach = io.attribute("reach").as_double();

      // Add to hash map
      io_map_[type] = reach;
    }
  } else {
    std::cerr << "Error: No IO file specified" << std::endl;
    return;
  }
  
  // Now convert the XML netlist file and block definitions file to a hypergraph
  ConvertXMLToHypergraph(chiplet_netlist_file, chiplet_blocks_file);
  
  // Create the hypergraph object from the data we've collected
  hypergraph_ = std::make_shared<Hypergraph>(
      vertex_dimensions_, hyperedge_dimensions_, hyperedges_, vertex_weights_,
      hyperedge_weights_, reach_, io_sizes_);
      
  std::cout << "[INFO] Number of IP blocks in chiplet graph: " << num_vertices_
            << std::endl;
  std::cout << "[INFO] Number of nets in chiplet graph: " << hyperedges_.size()
            << std::endl;
            
  // Write vertex mapping file for debugging/reference
  std::ofstream map_file("output.map");
  if (map_file.is_open()) {
    for (const auto& pair : vertex_index_to_name_) {
      map_file << pair.first + 1 << " " << pair.second << std::endl;
    }
    map_file.close();
    std::cout << "[INFO] Wrote vertex mapping to output.map" << std::endl;
  }
}

void ChipletPart::ConvertXMLToHypergraph(const std::string& netlist_file,
                                       const std::string& block_def_file) {
  // Clear existing hypergraph data
  hyperedges_.clear();
  hyperedge_weights_.clear();
  vertex_weights_.clear();
  reach_.clear();
  io_sizes_.clear();
  vertex_index_to_name_.clear();
  vertex_name_to_index_.clear();
  
  // Read the netlist file
  pugi::xml_document netlist_doc;
  pugi::xml_parse_result netlist_result = netlist_doc.load_file(netlist_file.c_str());
  if (!netlist_result) {
    std::cerr << "Error parsing netlist file: " << netlist_result.description() << std::endl;
    return;
  }
  
  std::cout << "[INFO] Converting netlist XML to hypergraph..." << std::endl;
  
  // Get the number of edges (nets) in the graph
  pugi::xml_node netlist_root = netlist_doc.child("netlist");
  int num_edges = 0;
  for (pugi::xml_node net = netlist_root.child("net"); net; net = net.next_sibling("net")) {
    num_edges++;
  }
  
  // Initialize variables for creating the hypergraph
  num_hyperedges_ = num_edges;
  num_vertices_ = 0;
  
  // Prepare collections for hypergraph construction
  std::vector<std::vector<int>> net_array;
  std::vector<float> net_weight_array;
  std::vector<float> reach_array;
  std::vector<float> io_size_array;
  
  // Process each net in the netlist
  for (pugi::xml_node net = netlist_root.child("net"); net; net = net.next_sibling("net")) {
    // Get the type of the net
    std::string type = net.attribute("type").as_string();
    
    // Find the reach of the net
    float reach = -1.0;
    if (io_map_.find(type) != io_map_.end()) {
      reach = io_map_[type];
    }
    reach_array.push_back(reach);
    
    // Default IO size to 1.0 for now
    io_size_array.push_back(1.0);
    
    // Process the blocks in this net
    std::string block0_name = net.attribute("block0").as_string();
    std::string block1_name = net.attribute("block1").as_string();
    
    // Add blocks to the vertex mapping if they don't exist
    int block0_index, block1_index;
    
    if (vertex_name_to_index_.find(block0_name) != vertex_name_to_index_.end()) {
      block0_index = vertex_name_to_index_[block0_name];
    } else {
      block0_index = num_vertices_;
      vertex_name_to_index_[block0_name] = num_vertices_;
      vertex_index_to_name_[num_vertices_] = block0_name;
      num_vertices_++;
    }
    
    if (vertex_name_to_index_.find(block1_name) != vertex_name_to_index_.end()) {
      block1_index = vertex_name_to_index_[block1_name];
    } else {
      block1_index = num_vertices_;
      vertex_name_to_index_[block1_name] = num_vertices_;
      vertex_index_to_name_[num_vertices_] = block1_name;
      num_vertices_++;
    }
    
    // Get the bandwidth as the edge weight
    float bandwidth = net.attribute("bandwidth").as_float(1.0); // Default to 1.0 if not specified
    net_weight_array.push_back(bandwidth);
    
    // Add the edge to the net array
    net_array.push_back({block0_index, block1_index});
  }
  
  // Read block definitions file to get vertex weights (area)
  std::unordered_map<std::string, float> vertex_area;
  std::ifstream block_file(block_def_file);
  if (block_file.is_open()) {
    std::string line;
    while (std::getline(block_file, line)) {
      std::istringstream iss(line);
      std::string vertex_name;
      float area, power;
      std::string tech;
      
      if (!(iss >> vertex_name >> area >> power >> tech)) {
        continue; // Skip malformed lines
      }
      
      vertex_area[vertex_name] = area;
    }
    block_file.close();
  } else {
    std::cerr << "Error: Could not open block definition file: " << block_def_file << std::endl;
    return;
  }
  
  // Initialize hyperedges and weights
  hyperedges_ = net_array;
  
  for (const auto& weight : net_weight_array) {
    hyperedge_weights_.push_back({weight});
  }
  
  // Set reach values
  reach_ = reach_array;
  io_sizes_ = io_size_array;
  
  // Initialize vertex weights from block definitions
  vertex_weights_.resize(num_vertices_);
  for (int i = 0; i < num_vertices_; i++) {
    const std::string& vertex_name = vertex_index_to_name_[i];
    float area = 1.0; // Default area
    
    if (vertex_area.find(vertex_name) != vertex_area.end()) {
      area = vertex_area[vertex_name];
    }
    
    vertex_weights_[i] = {area};
  }
  
  std::cout << "[INFO] Created hypergraph with " << num_vertices_ << " vertices and "
            << num_hyperedges_ << " hyperedges" << std::endl;
}

// Constructor to initialize rng_ with seed_
ChipletPart::ChipletPart() {
  // Initialize the random number generator with the default seed
  rng_.seed(seed_);
}

void ChipletPart::QuickTechPartition(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, 
    std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, 
    std::string chiplet_test_file,
    std::string chiplet_netlist_file, 
    std::string chiplet_blocks_file,
    float reach, 
    float separation, 
    std::vector<std::string> &tech_nodes,
    std::string output_prefix) {
  
  // Ensure RNG is correctly seeded
  rng_.seed(seed_);
  
  // Set number of parts based on technology assignment
  num_parts_ = tech_nodes.size();
  
  // Generate the hypergraph from XML files if not already done
  if (hypergraph_ == nullptr) {
    ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);
  }
  
  // Run METIS partitioning with the specified number of parts
  std::vector<int> partition = METISPart(num_parts_);
  
  // Setup for cost evaluation
  std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);
  
  // Create a ChipletRefiner with cost model files
  auto refiner = std::make_shared<chiplet::ChipletRefiner>(
      num_parts_,                   // num_parts
      2,                            // refiner_iters (fewer to be quick)
      static_cast<int>(hypergraph_->GetNumVertices() * 0.05),  // max_move
      reaches,                      // reaches
      true,                         // floorplanning
      chiplet_io_file,              // io_file
      chiplet_layer_file,           // layer_file
      chiplet_wafer_process_file,   // wafer_process_file
      chiplet_assembly_process_file, // assembly_process_file
      chiplet_test_file,            // test_file
      chiplet_netlist_file,         // netlist_file
      chiplet_blocks_file           // blocks_file
  );
  
  // Set the technology array
  refiner->SetTechArray(tech_nodes);
  
  // Run floorplanner to get locations
  auto floor_result = refiner->RunFloorplanner(
      partition, hypergraph_, 100, 100, 0.00001);
  
  std::vector<float> aspect_ratios = std::get<0>(floor_result);
  std::vector<float> x_locations = std::get<1>(floor_result);
  std::vector<float> y_locations = std::get<2>(floor_result);
  bool success = std::get<3>(floor_result);
  
  // Set results in refiner
  refiner->SetAspectRatios(aspect_ratios);
  refiner->SetXLocations(x_locations);
  refiner->SetYLocations(y_locations);
  
  // Compute the final cost
  float cost = refiner->GetCostFromScratch(partition);
  
  // Save results
  std::string partition_file = output_prefix + ".parts." + std::to_string(num_parts_);
  std::ofstream part_out(partition_file);
  for (int p : partition) {
    part_out << p << std::endl;
  }
  part_out.close();
  
  std::string tech_file = output_prefix + ".techs." + std::to_string(num_parts_);
  std::ofstream tech_out(tech_file);
  for (const auto& tech : tech_nodes) {
    tech_out << tech << std::endl;
  }
  tech_out.close();
  
  std::string summary_file = output_prefix + ".summary.txt";
  std::ofstream summary_out(summary_file);
  summary_out << "Quick Tech Partition Results" << std::endl;
  summary_out << "-------------------------" << std::endl;
  summary_out << "Number of Partitions: " << num_parts_ << std::endl;
  summary_out << "Cost: " << cost << std::endl;
  summary_out << "Success: " << (success ? "Yes" : "No") << std::endl;
  summary_out << std::endl << "Technology Assignments:" << std::endl;
  for (size_t i = 0; i < tech_nodes.size(); ++i) {
    summary_out << "Partition " << i << ": " << tech_nodes[i] << std::endl;
  }
  summary_out.close();
  
  // Store the resulting partition for later use
  solution_ = partition;
}

void ChipletPart::CanonicalGeneticTechPart(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, 
    std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, 
    std::string chiplet_test_file,
    std::string chiplet_netlist_file, 
    std::string chiplet_blocks_file,
    float reach, 
    float separation, 
    std::vector<std::string> &tech_nodes,
    int population_size,
    int num_generations,
    float mutation_rate,
    float crossover_rate,
    int min_partitions,
    int max_partitions,
    std::string output_prefix) {
  
  try {
    std::cout << "[INFO] Using full Canonical Genetic Algorithm implementation" << std::endl;
    
    // Start timing
    auto start_time_total = std::chrono::high_resolution_clock::now();
    
    // Ensure RNG is correctly seeded
    rng_.seed(seed_);
    
    // Ensure hypergraph is built
    if (hypergraph_ == nullptr) {
      std::cout << "[INFO] Building hypergraph from provided files..." << std::endl;
      auto start_time_hypergraph = std::chrono::high_resolution_clock::now();
      
      ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);
      
      auto end_time_hypergraph = std::chrono::high_resolution_clock::now();
      auto duration_hypergraph = std::chrono::duration_cast<std::chrono::milliseconds>(
          end_time_hypergraph - start_time_hypergraph).count();
      
      std::cout << "[INFO] Hypergraph construction time: " << duration_hypergraph << " ms" << std::endl;
      
      if (hypergraph_ == nullptr) {
        std::cout << "[ERROR] Failed to create hypergraph from input files!" << std::endl;
        return;
      }
    }
    
    // Create a safe copy of the tech nodes vector
    std::vector<std::string> safe_tech_nodes;
    if (!tech_nodes.empty()) {
      safe_tech_nodes.reserve(tech_nodes.size());
      for (const auto& tech : tech_nodes) {
        // Create a new string with proper memory allocation
        std::string tech_copy(tech.c_str());
        safe_tech_nodes.push_back(tech_copy);
      }
    } else {
      // Default technology nodes
      safe_tech_nodes.push_back(std::string("7nm"));
      safe_tech_nodes.push_back(std::string("14nm"));
      safe_tech_nodes.push_back(std::string("28nm"));
    }
    
    // Print GA configuration
    std::cout << "[INFO] Canonical GA Configuration:" << std::endl;
    std::cout << "[INFO] - Technology Nodes: ";
    for (const auto& tech : safe_tech_nodes) {
      std::cout << tech << " ";
    }
    std::cout << std::endl;
    std::cout << "[INFO] - Population Size: " << population_size << std::endl;
    std::cout << "[INFO] - Number of Generations: " << num_generations << std::endl;
    std::cout << "[INFO] - Mutation Rate: " << mutation_rate << std::endl;
    std::cout << "[INFO] - Crossover Rate: " << crossover_rate << std::endl;
    std::cout << "[INFO] - Min Partitions: " << min_partitions << std::endl;
    std::cout << "[INFO] - Max Partitions: " << max_partitions << std::endl;
    std::cout << "[INFO] - Output Prefix: " << output_prefix << std::endl;
    
    // Log file for timing information
    std::ofstream timing_log("ga_timing.log");
    if (!timing_log.is_open()) {
      std::cerr << "[WARNING] Could not open timing log file. Proceeding without logging." << std::endl;
    } else {
      timing_log << "Canonical GA Timing Log" << std::endl;
      timing_log << "----------------------" << std::endl;
      timing_log << "Configuration:" << std::endl;
      timing_log << "- Population Size: " << population_size << std::endl;
      timing_log << "- Number of Generations: " << num_generations << std::endl;
      timing_log << "- Mutation Rate: " << mutation_rate << std::endl;
      timing_log << "- Crossover Rate: " << crossover_rate << std::endl;
      timing_log << "- Min Partitions: " << min_partitions << std::endl;
      timing_log << "- Max Partitions: " << max_partitions << std::endl;
      timing_log << "- Random Seed: " << seed_ << std::endl;
      timing_log << "- Tech Nodes: ";
      for (const auto& tech : safe_tech_nodes) {
        timing_log << tech << " ";
      }
      timing_log << std::endl << std::endl;
      timing_log << "Timing Measurements:" << std::endl;
    }
    
    auto start_time_ga = std::chrono::high_resolution_clock::now();
    
    // Create the canonical GA instance
    CanonicalGA canonical_ga(
      safe_tech_nodes,  // Safe copy of tech nodes
      seed_,            // Random seed
      num_generations,  // Number of generations
      population_size,  // Population size
      mutation_rate,    // Mutation rate
      crossover_rate,   // Crossover rate
      min_partitions,   // Min partitions
      max_partitions    // Max partitions
    );
    
    auto end_time_ga_init = std::chrono::high_resolution_clock::now();
    auto duration_ga_init = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time_ga_init - start_time_ga).count();
        
    if (timing_log.is_open()) {
      timing_log << "- GA Initialization: " << duration_ga_init << " ms" << std::endl;
    }
    std::cout << "[INFO] GA initialization time: " << duration_ga_init << " ms" << std::endl;
    
    // Record start time for the GA run
    auto start_time_ga_run = std::chrono::high_resolution_clock::now();
    
    // Run the canonical GA
    CanonicalSolution solution = canonical_ga.Run(
      this,                        // Pointer to ChipletPart for evaluation
      chiplet_io_file,
      chiplet_layer_file,
      chiplet_wafer_process_file,
      chiplet_assembly_process_file,
      chiplet_test_file,
      chiplet_netlist_file,
      chiplet_blocks_file,
      reach,
      separation
    );
    
    // Record end time for the GA run
    auto end_time_ga_run = std::chrono::high_resolution_clock::now();
    auto duration_ga_run = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time_ga_run - start_time_ga_run).count();
    
    if (timing_log.is_open()) {
      timing_log << "- GA Run: " << duration_ga_run << " ms" << std::endl;
    }
    std::cout << "[INFO] GA run time: " << duration_ga_run << " ms" << std::endl;
    
    // Record start time for saving results
    auto start_time_save = std::chrono::high_resolution_clock::now();
    
    // Save the results
    canonical_ga.SaveResults(solution, output_prefix);
    
    // Record end time for saving results
    auto end_time_save = std::chrono::high_resolution_clock::now();
    auto duration_save = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time_save - start_time_save).count();
    
    if (timing_log.is_open()) {
      timing_log << "- Saving Results: " << duration_save << " ms" << std::endl;
    }
    std::cout << "[INFO] Results saving time: " << duration_save << " ms" << std::endl;
    
    // Store the solution in our class
    solution_ = solution.partition;
    num_parts_ = solution.tech_nodes.size();
    
    // Calculate total runtime
    auto end_time_total = std::chrono::high_resolution_clock::now();
    auto duration_total = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time_total - start_time_total).count();
    
    // Log total runtime
    if (timing_log.is_open()) {
      timing_log << "- Total Runtime: " << duration_total << " ms" << std::endl;
      timing_log << std::endl;
      timing_log << "Results:" << std::endl;
      timing_log << "- Best Cost: " << solution.cost << std::endl;
      timing_log << "- Number of Partitions: " << solution.tech_nodes.size() << std::endl;
      timing_log << "- Technology Assignments:" << std::endl;
      for (size_t i = 0; i < solution.tech_nodes.size(); i++) {
        timing_log << "  Partition " << i << ": " << solution.tech_nodes[i] << std::endl;
      }
      
      timing_log.close();
    }
    
    // Also create a summary file with timing information
    std::string timing_summary_file = output_prefix + ".timing.txt";
    std::ofstream timing_summary(timing_summary_file);
    if (timing_summary.is_open()) {
      timing_summary << "Canonical GA Timing Summary" << std::endl;
      timing_summary << "-------------------------" << std::endl;
      timing_summary << "GA Initialization: " << duration_ga_init << " ms" << std::endl;
      timing_summary << "GA Run: " << duration_ga_run << " ms" << std::endl;
      timing_summary << "Saving Results: " << duration_save << " ms" << std::endl;
      timing_summary << "Total Runtime: " << duration_total << " ms (" << 
                      (duration_total / 1000.0) << " seconds)" << std::endl;
      timing_summary.close();
      
      std::cout << "[SUCCESS] Timing summary saved to: " << timing_summary_file << std::endl;
    }
    
    std::cout << "[SUCCESS] Canonical Genetic Algorithm completed successfully" << std::endl;
    std::cout << "[INFO] Best cost: " << solution.cost << std::endl;
    std::cout << "[INFO] Number of partitions: " << solution.tech_nodes.size() << std::endl;
    std::cout << "[INFO] Technology assignments:" << std::endl;
    for (size_t i = 0; i < solution.tech_nodes.size(); i++) {
      std::cout << "[INFO]   Partition " << i << ": " << solution.tech_nodes[i] << std::endl;
    }
    std::cout << "[INFO] Total GA runtime: " << duration_total << " ms (" << 
              (duration_total / 1000.0) << " seconds)" << std::endl;
  } 
  catch (const std::exception& e) {
    std::cerr << "[ERROR] Exception in CanonicalGeneticTechPart: " << e.what() << std::endl;
  }
  catch (...) {
    std::cerr << "[ERROR] Unknown exception in CanonicalGeneticTechPart" << std::endl;
  }
}

// Evaluate a technology partition assignment for genetic algorithm
std::tuple<float, std::vector<int>> ChipletPart::EvaluateTechPartition(
    std::string chiplet_io_file,
    std::string chiplet_layer_file,
    std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file,
    std::string chiplet_test_file,
    std::string chiplet_netlist_file,
    std::string chiplet_blocks_file, 
    float reach,
    float separation, 
    const std::vector<std::string>& tech_assignment) {
    
    try {
        // Validation: Ensure we have at least one technology node
        if (tech_assignment.empty()) {
            Console::Error("EvaluateTechPartition: Empty technology assignment");
            return {std::numeric_limits<float>::max(), std::vector<int>()};
        }
        
        // Set the random seed for reproducibility
        rng_.seed(seed_);
        
        std::string tech_str = "";
        for (size_t i = 0; i < tech_assignment.size(); ++i) {
            tech_str += tech_assignment[i];
            if (i < tech_assignment.size() - 1) tech_str += ", ";
        }
        Console::Info("Evaluating technology partition with tech nodes: " + tech_str);
        
        // Generate the hypergraph from XML files if not already done
        if (!hypergraph_ || hypergraph_->GetNumVertices() == 0) {
            try {
                ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);
                if (!hypergraph_ || hypergraph_->GetNumVertices() == 0) {
                    Console::Error("Failed to create hypergraph");
                    return {std::numeric_limits<float>::max(), std::vector<int>()};
                }
            } catch (const std::exception& e) {
                Console::Error("Exception creating hypergraph: " + std::string(e.what()));
                return {std::numeric_limits<float>::max(), std::vector<int>()};
            }
        }
        
        // Get number of vertices
        int num_vertices = hypergraph_->GetNumVertices();
        
        // Store the original num_parts_ to restore it later
        int original_num_parts = num_parts_;
        
        // Set num_parts_ based on the technology assignment count
        // This is important as we want to create a partition with exactly
        // as many parts as we have technology nodes
        num_parts_ = tech_assignment.size();
        Console::Info("Setting number of partitions to " + std::to_string(num_parts_));
        
        // Special case for single partition - no need for complex partitioning
        if (num_parts_ == 1) {
            Console::Info("Single partition case - creating trivial partition");
            std::vector<int> single_partition(num_vertices, 0); // All vertices in partition 0
            
            // Create a refiner for evaluating partition cost
            std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);
            std::shared_ptr<chiplet::ChipletRefiner> refiner;
            
            try {
                refiner = std::make_shared<chiplet::ChipletRefiner>(
                    1,                            // num_parts (single partition)
                    1,                            // refiner_iters (minimum since no refinement needed)
                    1,                            // max_move (minimum since no refinement needed)
                    reaches,                      // reaches
                    true,                         // floorplanning
                    chiplet_io_file,              // io_file
                    chiplet_layer_file,           // layer_file
                    chiplet_wafer_process_file,   // wafer_process_file
                    chiplet_assembly_process_file, // assembly_process_file
                    chiplet_test_file,            // test_file
                    chiplet_netlist_file,         // netlist_file
                    chiplet_blocks_file           // blocks_file
                );
                
                // Set the tech array
                std::vector<std::string> tech_array(num_vertices, tech_assignment[0]);
                refiner->SetTechArray(tech_array);
                
                // Evaluate cost of single partition
                float cost = refiner->GetCostFromScratch(single_partition);
                
                // Store the partition in the solution_ member for later use
                solution_ = single_partition;
                
                // Restore original num_parts_
                num_parts_ = original_num_parts;
                
                Console::Success("Single partition evaluation complete, cost: " + std::to_string(cost));
                
                // Return the cost and partition
                return {cost, single_partition};
                
            } catch (const std::exception& e) {
                Console::Error("Exception evaluating single partition: " + std::string(e.what()));
                num_parts_ = original_num_parts; // Restore original num_parts_ before returning
                return {std::numeric_limits<float>::max(), std::vector<int>()};
            }
        }
        
        // For multiple partitions (num_parts_ >= 2), use METIS and spectral partitioning
        std::vector<std::vector<int>> init_partitions;
        
        // Run METIS partitioning
        try {
            Console::Info("Running METIS partitioning");
            std::vector<int> metis_partition = METISPart(num_parts_);
            
            if (!metis_partition.empty()) {
                init_partitions.push_back(metis_partition);
                Console::Info("METIS partitioning completed successfully");
            } else {
                Console::Warning("METIS partitioning failed to produce a valid partition");
            }
        } catch (const std::exception& e) {
            Console::Warning("Exception running METIS: " + std::string(e.what()));
            // Continue and try spectral partitioning
        }
        
        // Try spectral partitioning as well
        try {
            Console::Info("Running spectral partitioning");
            std::vector<int> spec_partition = SpectralPartition();
            
            if (!spec_partition.empty() && 
                std::find(spec_partition.begin(), spec_partition.end(), -1) == spec_partition.end()) {
                init_partitions.push_back(spec_partition);
                Console::Info("Spectral partitioning completed successfully");
            } else {
                Console::Warning("Spectral partitioning failed to produce a valid partition");
            }
        } catch (const std::exception& e) {
            Console::Warning("Exception running spectral partitioning: " + std::string(e.what()));
            // Continue with whatever partitions we have
        }
        
        // If both methods failed, create a simple initial partition
        if (init_partitions.empty()) {
            Console::Warning("No valid partitions generated, creating a fallback partition");
            std::vector<int> simple_partition(num_vertices, 0);
            
            // Create a reasonably balanced simple partition
            int vertices_per_part = num_vertices / num_parts_;
            int remainder = num_vertices % num_parts_;
            
            int current_idx = 0;
            for (int part = 0; part < num_parts_; part++) {
                int part_size = vertices_per_part + (part < remainder ? 1 : 0);
                for (int j = 0; j < part_size && current_idx < num_vertices; j++) {
                    simple_partition[current_idx++] = part;
                }
            }
            
            init_partitions.push_back(simple_partition);
            Console::Info("Created fallback partition");
        }
        
        Console::Info("Generated " + std::to_string(init_partitions.size()) + " initial partitions");
        
        // Create a refiner for evaluating and refining the partitions
        std::vector<int> reaches(hypergraph_->GetNumHyperedges(), reach);
        std::shared_ptr<chiplet::ChipletRefiner> refiner;
        
        try {
            refiner = std::make_shared<chiplet::ChipletRefiner>(
                num_parts_,                   // num_parts
                refine_iters_,                // refiner_iters
                max_moves_,                   // max_move
                reaches,                      // reaches
                true,                         // floorplanning
                chiplet_io_file,              // io_file
                chiplet_layer_file,           // layer_file
                chiplet_wafer_process_file,   // wafer_process_file
                chiplet_assembly_process_file, // assembly_process_file
                chiplet_test_file,            // test_file
                chiplet_netlist_file,         // netlist_file
                chiplet_blocks_file           // blocks_file
            );
            
            // Set up initial aspect ratios, locations, etc.
            std::vector<float> aspect_ratios(num_vertices, 1.0);
            refiner->SetAspectRatios(aspect_ratios);
            
            std::vector<float> x_locations(num_vertices, 0.0);
            std::vector<float> y_locations(num_vertices, 0.0);
            refiner->SetXLocations(x_locations);
            refiner->SetYLocations(y_locations);
        } catch (const std::exception& e) {
            Console::Error("Exception creating refiner: " + std::string(e.what()));
            // Restore original num_parts_ before returning
            num_parts_ = original_num_parts;
            return {std::numeric_limits<float>::max(), std::vector<int>()};
        }
        
        // Prepare for refinement
        const int num_dimensions = hypergraph_->GetVertexDimensions();
        std::vector<float> total_weights = hypergraph_->GetTotalVertexWeights();
        
        // Create upper_block_balance with balanced allocation per partition
        Matrix<float> ub_balance(num_parts_, std::vector<float>(num_dimensions, 0.0f));
        
        for (int i = 0; i < num_parts_; i++) {
            for (int j = 0; j < num_dimensions && j < total_weights.size(); j++) {
                ub_balance[i][j] = total_weights[j] / num_parts_ * ub_factor_;
            }
        }
        
        // Create lower_block_balance with zeros
        Matrix<float> lb_balance(
            num_parts_, std::vector<float>(num_dimensions, 0.0f));
        
        // Find the best partition by evaluating all candidates
        float best_cost = std::numeric_limits<float>::max();
        std::vector<int> best_partition;
        
        for (size_t i = 0; i < init_partitions.size(); i++) {
            try {
                std::string method_name = (i == 0 && !init_partitions.empty() ? "METIS" : 
                                          (i == 1 ? "Spectral" : "Fallback"));
                Console::Info("Evaluating " + method_name + " partition");
                
                // Get a copy of this partition
                std::vector<int> partition_copy = init_partitions[i];
                
                // Make sure partition has the correct size
                if (partition_copy.size() != static_cast<size_t>(num_vertices)) {
                    Console::Warning("Partition size mismatch, resizing");
                    partition_copy.resize(num_vertices, 0);
                }
                
                // Validate partition - ensure all values are in bounds
                for (int& part_id : partition_copy) {
                    if (part_id < 0 || part_id >= num_parts_) {
                        part_id = 0; // Set to first partition if out of bounds
                    }
                }
                
                // Create a tech_array that maps each vertex to a technology
                std::vector<std::string> tech_array(num_vertices);
                for (int v = 0; v < num_vertices; v++) {
                    int part_id = partition_copy[v];
                    
                    // Assign technology based on partition ID
                    if (part_id >= 0 && part_id < static_cast<int>(tech_assignment.size())) {
                        tech_array[v] = tech_assignment[part_id];
                    } else {
                        // If part_id is out of range, use the first technology
                        tech_array[v] = tech_assignment[0];
                    }
                }
                
                // Set the tech array in the refiner
                refiner->SetTechArray(tech_array);
                
                // Get initial cost of this partition
                float initial_cost = refiner->GetCostFromScratch(partition_copy);
                
                // Refine this partition
                try {
                    refiner->Refine(hypergraph_, ub_balance, lb_balance, partition_copy);
                } catch (const std::exception& e) {
                    Console::Warning("Exception during refinement: " + std::string(e.what()));
                    // Continue without refinement
                }
                
                // Update tech array after refinement
                for (int v = 0; v < num_vertices; v++) {
                    int part_id = partition_copy[v];
                    if (part_id >= 0 && part_id < static_cast<int>(tech_assignment.size())) {
                        tech_array[v] = tech_assignment[part_id];
                    } else {
                        tech_array[v] = tech_assignment[0];
                    }
                }
                
                // Update the refiner with the new tech array
                refiner->SetTechArray(tech_array);
                
                // Get final cost after refinement
                float final_cost = refiner->GetCostFromScratch(partition_copy);
                
                Console::Info(method_name + " partition cost: " + 
                             std::to_string(initial_cost) + " -> " + std::to_string(final_cost));
                
                // Track the best partition
                if (final_cost < best_cost) {
                    best_cost = final_cost;
                    best_partition = partition_copy;
                    Console::Info("New best partition found from " + method_name + 
                                 " with cost: " + std::to_string(best_cost));
                }
            } catch (const std::exception& e) {
                Console::Warning("Exception evaluating partition " + std::to_string(i) + 
                                ": " + std::string(e.what()));
                // Continue with next partition
            }
        }
        
        // Make sure we found a valid partition
        if (best_partition.empty()) {
            Console::Error("Failed to find a valid partition!");
            num_parts_ = original_num_parts;
            return {std::numeric_limits<float>::max(), std::vector<int>()};
        }
        
        // Store the best partition in the solution_ member for later use
        solution_ = best_partition;
        
        // Restore original num_parts_
        num_parts_ = original_num_parts;
        
        // Create a tech array for the best partition for summary
        std::vector<std::string> best_tech_array(num_vertices);
        for (int v = 0; v < num_vertices; v++) {
            int part_id = best_partition[v];
            if (part_id >= 0 && part_id < static_cast<int>(tech_assignment.size())) {
                best_tech_array[v] = tech_assignment[part_id];
            } else {
                best_tech_array[v] = tech_assignment[0];
            }
        }
        
        // Print technology assignment summary
        std::unordered_map<std::string, int> tech_counts;
        for (const auto& tech : best_tech_array) {
            tech_counts[tech]++;
        }
        
        Console::Info("Technology assignment summary:");
        for (const auto& tech_pair : tech_counts) {
            Console::Info("  " + tech_pair.first + ": " + 
                         std::to_string(tech_pair.second) + " vertices (" + 
                         std::to_string(100.0 * tech_pair.second / num_vertices) + "%)");
        }
        
        Console::Success("Technology partition evaluation complete, best cost: " + 
                         std::to_string(best_cost));
        
        // Return the cost and best partition
        return {best_cost, best_partition};
    } catch (const std::exception& e) {
        Console::Error("Unhandled exception in EvaluateTechPartition: " + std::string(e.what()));
        return {std::numeric_limits<float>::max(), std::vector<int>()};
    }
}

// Method to run the canonical GA for technology assignment

std::tuple<float, std::vector<int>, std::vector<std::string>> ChipletPart::EnumerateTechAssignments(
    std::string chiplet_io_file,
    std::string chiplet_layer_file, 
    std::string chiplet_wafer_process_file,
    std::string chiplet_assembly_process_file, 
    std::string chiplet_test_file,
    std::string chiplet_netlist_file, 
    std::string chiplet_blocks_file,
    float reach, 
    float separation, 
    const std::vector<std::string>& available_tech_nodes,
    int max_partitions,
    bool detailed_output) {

    Console::Header("Technology Enumeration Analysis");
    Console::Info("Available technology nodes: " + std::to_string(available_tech_nodes.size()));
    for (const auto& tech : available_tech_nodes) {
        Console::Info("  - " + tech);
    }
    Console::Info("Maximum number of partitions: " + std::to_string(max_partitions));
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Ensure the hypergraph is loaded
    if (!hypergraph_ || hypergraph_->GetNumVertices() == 0) {
        try {
            ReadChipletGraphFromXML(chiplet_io_file, chiplet_netlist_file, chiplet_blocks_file);
            if (!hypergraph_ || hypergraph_->GetNumVertices() == 0) {
                Console::Error("Failed to create hypergraph");
                return {std::numeric_limits<float>::max(), {}, {}};
            }
        } catch (const std::exception& e) {
            Console::Error("Exception creating hypergraph: " + std::string(e.what()));
            return {std::numeric_limits<float>::max(), {}, {}};
        }
    }
    
    int num_vertices = hypergraph_->GetNumVertices();
    
    // Store original num_parts_ to restore it later
    int original_num_parts = num_parts_;
    
    // Best solution tracking
    float best_cost = std::numeric_limits<float>::max();
    std::vector<int> best_partition;
    std::vector<std::string> best_tech_assignment;
    int best_num_parts = 0;
    
    // Multi-sets for counting equivalent assignments
    std::unordered_set<std::string> canonical_assignments_seen;
    
    // Generate all possible technology assignments from 1 to max_partitions
    int total_assignments = 0;
    int pruned_assignments = 0;
    int evaluated_assignments = 0;
    
    // Performance tracking
    std::map<std::string, std::vector<int>> eval_times_by_canonical;
    std::map<int, std::vector<int>> eval_times_by_parts;
    int64_t total_eval_time_ms = 0;
    
    // Helper function to canonicalize a tech assignment (returns a canonical string representation)
    auto canonicalize_assignment = [&](const std::vector<std::string>& assignment) -> std::string {
        // Count occurrences of each technology node
        std::map<std::string, int> tech_counts;
        for (const auto& tech : assignment) {
            tech_counts[tech]++;
        }
        
        // Create a canonical representation: sort by frequency (descending), then by tech name
        std::vector<std::pair<std::string, int>> frequency_pairs;
        for (const auto& pair : tech_counts) {
            frequency_pairs.push_back({pair.first, pair.second});
        }
        
        // Sort by frequency (descending) and then by tech node name for tie-breaking
        std::sort(frequency_pairs.begin(), frequency_pairs.end(), 
            [](const auto& a, const auto& b) {
                return a.second > b.second || (a.second == b.second && a.first < b.first);
            });
        
        // Build canonical string representation
        std::stringstream ss;
        ss << frequency_pairs.size() << ":";
        for (const auto& pair : frequency_pairs) {
            ss << pair.first << ":" << pair.second << ":";
        }
        return ss.str();
    };
    
    // Create a progress tracking table
    std::vector<std::string> progress_columns = {"Parts", "Possible", "Unique", "Pruned", "Evaluated", "Best Cost"};
    std::vector<int> progress_widths = {8, 12, 12, 12, 12, 15};
    Console::TableHeader(progress_columns, progress_widths);
    
    // Create a results table if detailed output is requested
    std::vector<std::vector<std::string>> detailed_results;
    if (detailed_output) {
        detailed_results.push_back({"Parts", "Tech Assignment", "Cost", "Canonical Form"});
    }
    
    // Map to store information about each canonical form
    std::map<std::string, std::vector<std::string>> canonical_forms;
    int canonical_form_count = 0;
    
    // Explore partitions from 1 to max_partitions
    for (int k = 1; k <= max_partitions; k++) {
        Console::Info("Enumerating technology assignments for " + std::to_string(k) + " partitions");
        
        // Count possible assignments for this k (with replacement)
        int64_t possible_assignments = 1;
        for (int i = 0; i < k; i++) {
            possible_assignments *= available_tech_nodes.size();
        }
        
        // Generate technology assignments recursively with a lambda function
        std::vector<std::vector<std::string>> unique_assignments;
        std::function<void(std::vector<std::string>&, int)> generate_assignments = 
            [&](std::vector<std::string>& current, int depth) {
                if (depth == k) {
                    // We have a complete assignment
                    total_assignments++;
                    
                    // Canonicalize to check if we've seen this pattern before
                    std::string canonical = canonicalize_assignment(current);
                    
                    if (canonical_assignments_seen.find(canonical) == canonical_assignments_seen.end()) {
                        canonical_assignments_seen.insert(canonical);
                        unique_assignments.push_back(current);
                        
                        // Log canonical form information
                        canonical_form_count++;
                        
                        // Convert current assignment to string representation
                        std::stringstream ss;
                        ss << "[";
                        for (size_t i = 0; i < current.size(); i++) {
                            ss << current[i];
                            if (i < current.size() - 1) ss << ", ";
                        }
                        ss << "]";
                        
                        // Store for later analysis
                        canonical_forms[canonical].push_back(ss.str());
                        
                        // Log detailed info if in verbose mode
                        if (detailed_output) {
                            Console::Info("Found new canonical form #" + std::to_string(canonical_form_count) + 
                                         ": " + canonical + " - Example: " + ss.str());
                        }
                    } else {
                        pruned_assignments++;
                        
                        // Convert current assignment to string representation for logging
                        if (detailed_output) {
                            std::stringstream ss;
                            ss << "[";
                            for (size_t i = 0; i < current.size(); i++) {
                                ss << current[i];
                                if (i < current.size() - 1) ss << ", ";
                            }
                            ss << "]";
                            
                            // Add to the list of examples for this canonical form
                            canonical_forms[canonical].push_back(ss.str());
                            
                            Console::Info("Pruned redundant assignment: " + ss.str() + 
                                         " (matches canonical form: " + canonical + ")");
                        }
                    }
                    return;
                }
                
                // Try each available technology at this position
                for (const auto& tech : available_tech_nodes) {
                    current.push_back(tech);
                    generate_assignments(current, depth + 1);
                    current.pop_back();
                }
            };
        
        // Generate assignments for this k
        std::vector<std::string> current;
        generate_assignments(current, 0);
        
        Console::Info("Generated " + std::to_string(unique_assignments.size()) + 
                     " unique assignments for k=" + std::to_string(k));
        
        // Evaluate each unique assignment
        for (const auto& assignment : unique_assignments) {
            evaluated_assignments++;
            
            // Convert assignment to string for display
            std::string assignment_str = "[";
            for (size_t i = 0; i < assignment.size(); i++) {
                assignment_str += assignment[i];
                if (i < assignment.size() - 1) assignment_str += ", ";
            }
            assignment_str += "]";
            
            Console::Info("Evaluating tech assignment: " + assignment_str);
            
            // Start timing this evaluation
            auto eval_start_time = std::chrono::high_resolution_clock::now();
            
            // Evaluate this assignment
            auto [cost, partition] = EvaluateTechPartition(
                chiplet_io_file, chiplet_layer_file, chiplet_wafer_process_file,
                chiplet_assembly_process_file, chiplet_test_file, chiplet_netlist_file,
                chiplet_blocks_file, reach, separation, assignment);
            
            // End timing
            auto eval_end_time = std::chrono::high_resolution_clock::now();
            auto eval_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                eval_end_time - eval_start_time).count();
            
            // Record timing metrics for performance analysis
            total_eval_time_ms += eval_duration;
            eval_times_by_canonical[canonicalize_assignment(assignment)].push_back(eval_duration);
            eval_times_by_parts[k].push_back(eval_duration);
            
            // Get canonical form for this assignment
            std::string canonical_form = canonicalize_assignment(assignment);
            
            // Record result if detailed output is requested
            if (detailed_output) {
                detailed_results.push_back({
                    std::to_string(k),
                    assignment_str,
                    std::to_string(cost),
                    canonical_form
                });
            }
            
            Console::Info("Evaluation completed in " + std::to_string(eval_duration) + " ms, cost: " + std::to_string(cost));
            
            // Store timing information for this canonical form
            if (canonical_forms.find(canonical_form) != canonical_forms.end()) {
                canonical_forms[canonical_form].push_back(assignment_str + " (cost=" + 
                                                      std::to_string(cost) + ", time=" + 
                                                      std::to_string(eval_duration) + "ms)");
            }
            
            // Update best solution if this is better
            if (cost < best_cost) {
                best_cost = cost;
                best_partition = partition;
                best_tech_assignment = assignment;
                best_num_parts = k;
                
                Console::Success("New best solution found: " + 
                               std::to_string(k) + " parts, cost = " + 
                               std::to_string(best_cost) + 
                               " (canonical form: " + canonical_form + ")");
            }
        }
        
        // Print progress for this k
        Console::TableRow({
            std::to_string(k),
            std::to_string(possible_assignments),
            std::to_string(unique_assignments.size()),
            std::to_string(pruned_assignments),
            std::to_string(evaluated_assignments),
            std::to_string(best_cost)
        }, progress_widths);
    }
    
    // Restore original num_parts_
    num_parts_ = original_num_parts;
    
    // Calculate elapsed time
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time).count();
    
    // Print final summary
    Console::Header("Technology Enumeration Results");
    
    // Create a summary table
    std::vector<std::string> summary_columns = {"Metric", "Value"};
    std::vector<int> summary_widths = {30, 50};
    Console::TableHeader(summary_columns, summary_widths);
    
    // Format the best technology assignment as a string
    std::string best_tech_str = "[";
    for (size_t i = 0; i < best_tech_assignment.size(); i++) {
        best_tech_str += best_tech_assignment[i];
        if (i < best_tech_assignment.size() - 1) best_tech_str += ", ";
    }
    best_tech_str += "]";
    
    Console::TableRow({"Total Assignments Considered", std::to_string(total_assignments)}, summary_widths);
    Console::TableRow({"Unique Canonical Assignments", std::to_string(canonical_assignments_seen.size())}, summary_widths);
    Console::TableRow({"Pruned Assignments", std::to_string(pruned_assignments)}, summary_widths);
    Console::TableRow({"Evaluated Assignments", std::to_string(evaluated_assignments)}, summary_widths);
    Console::TableRow({"Best Number of Partitions", std::to_string(best_num_parts)}, summary_widths);
    Console::TableRow({"Best Technology Assignment", best_tech_str}, summary_widths);
    Console::TableRow({"Best Cost", std::to_string(best_cost)}, summary_widths);
    Console::TableRow({"Time Elapsed", std::to_string(duration) + " seconds"}, summary_widths);
    
    // If detailed output was requested, print detailed results
    if (detailed_output) {
        Console::Header("Detailed Results for All Evaluated Assignments");
        
        // Print table header
        std::vector<std::string> detail_columns = {"Parts", "Tech Assignment", "Cost", "Canonical Form"};
        std::vector<int> detail_widths = {8, 35, 15, 30};
        Console::TableHeader(detail_columns, detail_widths);
        
        // Skip the header row
        for (size_t i = 1; i < detailed_results.size(); i++) {
            Console::TableRow(detailed_results[i], detail_widths);
        }
        
        // Log canonical form statistics
        Console::Header("Canonical Form Analysis");
        std::vector<std::string> form_columns = {"ID", "Canonical Form", "Count", "Examples"};
        std::vector<int> form_widths = {5, 35, 8, 40};
        Console::TableHeader(form_columns, form_widths);
        
        int form_id = 1;
        for (const auto& [canonical, examples] : canonical_forms) {
            // Display at most 2 examples to keep the output manageable
            std::string example_str = examples[0];
            if (examples.size() > 1) {
                example_str += ", " + examples[1];
            }
            if (examples.size() > 2) {
                example_str += ", ...";
            }
            
            Console::TableRow({
                std::to_string(form_id++),
                canonical,
                std::to_string(examples.size()),
                example_str
            }, form_widths);
        }
    }
    
    // Save the best solution to files
    std::string assignment_file = chiplet_netlist_file + ".best_tech_assignment.txt";
    std::ofstream tech_out(assignment_file);
    if (tech_out.is_open()) {
        tech_out << "# Best technology assignment found by enumeration" << std::endl;
        tech_out << "# Number of partitions: " << best_num_parts << std::endl;
        tech_out << "# Cost: " << best_cost << std::endl;
        tech_out << "# Canonical form: " << canonicalize_assignment(best_tech_assignment) << std::endl;
        tech_out << "# Total assignments considered: " << total_assignments << std::endl;
        tech_out << "# Unique canonical assignments: " << canonical_assignments_seen.size() << std::endl;
        tech_out << "# Pruned assignments: " << pruned_assignments << std::endl;
        tech_out << std::endl;
        
        for (const auto& tech : best_tech_assignment) {
            tech_out << tech << std::endl;
        }
        tech_out.close();
        Console::Success("Best technology assignment saved to " + assignment_file);
    }
    
    // Write canonical forms log file
    std::string canonical_log_file = chiplet_netlist_file + ".canonical_forms.log";
    std::ofstream canonical_log(canonical_log_file);
    if (canonical_log.is_open()) {
        canonical_log << "# Canonical Forms Analysis" << std::endl;
        canonical_log << "# ========================" << std::endl;
        canonical_log << "# Total assignments considered: " << total_assignments << std::endl;
        canonical_log << "# Unique canonical assignments: " << canonical_assignments_seen.size() << std::endl;
        canonical_log << "# Pruned assignments: " << pruned_assignments << std::endl;
        canonical_log << "# Available technology nodes: ";
        for (size_t i = 0; i < available_tech_nodes.size(); i++) {
            canonical_log << available_tech_nodes[i];
            if (i < available_tech_nodes.size() - 1) canonical_log << ", ";
        }
        canonical_log << std::endl << std::endl;
        
        // Header for the detailed form analysis
        canonical_log << "# ID\tCanonical Form\tCount\tPartitions\tExamples" << std::endl;
        canonical_log << "# --\t--------------\t-----\t----------\t--------" << std::endl;
        
        // Sort canonical forms by number of parts for better organization
        std::map<int, std::vector<std::pair<std::string, std::vector<std::string>>>> forms_by_parts;
        
        for (const auto& [canonical, examples] : canonical_forms) {
            // Parse the number of parts from the canonical form (first value before the first colon)
            int num_parts = std::stoi(canonical.substr(0, canonical.find(":")));
            forms_by_parts[num_parts].push_back({canonical, examples});
        }
        
        // Write forms by number of parts, then by example count (descending)
        int form_id = 1;
        for (const auto& [num_parts, forms] : forms_by_parts) {
            // Sort forms for this number of parts by count (descending)
            std::vector<std::pair<std::string, std::vector<std::string>>> sorted_forms = forms;
            std::sort(sorted_forms.begin(), sorted_forms.end(), 
                [](const auto& a, const auto& b) { return a.second.size() > b.second.size(); });
            
            for (const auto& [canonical, examples] : sorted_forms) {
                canonical_log << form_id++ << "\t" << canonical << "\t" << examples.size() << "\t" << num_parts << "\t";
                
                // Write up to 3 examples
                int max_examples = std::min(3, static_cast<int>(examples.size()));
                for (int i = 0; i < max_examples; i++) {
                    canonical_log << examples[i];
                    if (i < max_examples - 1) canonical_log << "; ";
                }
                if (examples.size() > max_examples) {
                    canonical_log << "; ... (" << (examples.size() - max_examples) << " more)";
                }
                canonical_log << std::endl;
            }
        }
        
        // Add a summary by partition count
        canonical_log << std::endl << "# Summary by Partition Count" << std::endl;
        canonical_log << "# Partitions\tUnique Forms\tTotal Assignments\tPruned" << std::endl;
        
        for (const auto& [num_parts, forms] : forms_by_parts) {
            // Count total assignments for this partition count
            int total_assign = 0;
            for (const auto& [_, examples] : forms) {
                total_assign += examples.size();
            }
            int unique_forms = forms.size();
            int pruned_assign = total_assign - unique_forms;
            
            canonical_log << num_parts << "\t" << unique_forms << "\t" 
                         << total_assign << "\t" << pruned_assign << std::endl;
        }
        
        // Add performance analytics
        canonical_log << std::endl << "# Performance Analytics" << std::endl;
        canonical_log << "# Total evaluation time: " << total_eval_time_ms << " ms" << std::endl;
        if (evaluated_assignments > 0) {
            canonical_log << "# Average time per evaluation: " 
                         << (static_cast<double>(total_eval_time_ms) / evaluated_assignments) 
                         << " ms" << std::endl;
        }
        
        // Performance by partition count
        canonical_log << std::endl << "# Performance by Partition Count" << std::endl;
        canonical_log << "# Partitions\tEvaluations\tTotal Time (ms)\tAvg Time (ms)\tMin Time (ms)\tMax Time (ms)" << std::endl;
        
        for (const auto& [num_parts, times] : eval_times_by_parts) {
            if (times.empty()) continue;
            
            int64_t total_time = 0;
            int min_time = std::numeric_limits<int>::max();
            int max_time = 0;
            
            for (int time : times) {
                total_time += time;
                min_time = std::min(min_time, time);
                max_time = std::max(max_time, time);
            }
            
            double avg_time = static_cast<double>(total_time) / times.size();
            
            canonical_log << num_parts << "\t" 
                         << times.size() << "\t"
                         << total_time << "\t"
                         << avg_time << "\t"
                         << min_time << "\t"
                         << max_time << std::endl;
        }
        
        // Performance by canonical form (top 10 most expensive)
        canonical_log << std::endl << "# Performance by Canonical Form (Top 10 Most Expensive)" << std::endl;
        canonical_log << "# Canonical Form\tEvaluations\tTotal Time (ms)\tAvg Time (ms)" << std::endl;
        
        // Collect and sort forms by average evaluation time
        std::vector<std::pair<std::string, double>> forms_by_avg_time;
        for (const auto& [form, times] : eval_times_by_canonical) {
            if (times.empty()) continue;
            
            int64_t total_time = 0;
            for (int time : times) {
                total_time += time;
            }
            
            double avg_time = static_cast<double>(total_time) / times.size();
            forms_by_avg_time.push_back({form, avg_time});
        }
        
        // Sort by average time (descending)
        std::sort(forms_by_avg_time.begin(), forms_by_avg_time.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Print top 10 or all if fewer
        int top_n = std::min(10, static_cast<int>(forms_by_avg_time.size()));
        for (int i = 0; i < top_n; i++) {
            const auto& [form, avg_time] = forms_by_avg_time[i];
            const auto& times = eval_times_by_canonical[form];
            
            int64_t total_time = 0;
            for (int time : times) {
                total_time += time;
            }
            
            canonical_log << form << "\t" 
                         << times.size() << "\t"
                         << total_time << "\t"
                         << avg_time << std::endl;
        }
        
        canonical_log.close();
        Console::Success("Canonical forms analysis saved to " + canonical_log_file);
    }
    
    std::string partition_file = chiplet_netlist_file + ".best_partition.txt";
    std::ofstream part_out(partition_file);
    if (part_out.is_open()) {
        part_out << "# Best partition found by enumeration" << std::endl;
        part_out << "# Number of partitions: " << best_num_parts << std::endl;
        part_out << "# Cost: " << best_cost << std::endl;
        part_out << std::endl;
        
        for (int part_id : best_partition) {
            part_out << part_id << std::endl;
        }
        part_out.close();
        Console::Success("Best partition saved to " + partition_file);
    }
    
    // Return the best solution found
    return {best_cost, best_partition, best_tech_assignment};
}

} // namespace chiplet