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

#include "Hypergraph.h"

#include <fstream>
#include <iostream>
#include <string>
#include <numeric>
#include <algorithm>
#include <stdexcept>

#include "Utilities.h"

namespace chiplet {

Hypergraph::Hypergraph(int vertex_dimensions, int hyperedge_dimensions,
                       const std::vector<std::vector<int>>& hyperedges,
                       const std::vector<std::vector<float>>& vertex_weights,
                       const std::vector<std::vector<float>>& hyperedge_weights,
                       const std::vector<float>& reaches,
                       const std::vector<float>& io_sizes)
    : num_vertices_(static_cast<int>(vertex_weights.size())),
      num_hyperedges_(static_cast<int>(hyperedge_weights.size())),
      vertex_dimensions_(vertex_dimensions),
      hyperedge_dimensions_(hyperedge_dimensions),
      vertex_weights_(vertex_weights), hyperedge_weights_(hyperedge_weights),
      reaches_(reaches), io_sizes_(io_sizes) {
      
  // Validate input sizes
  if (static_cast<int>(hyperedges.size()) != num_hyperedges_) {
    throw std::invalid_argument("Number of hyperedges doesn't match hyperedge_weights size");
  }
  if (static_cast<int>(reaches.size()) != num_hyperedges_) {
    throw std::invalid_argument("Number of reaches doesn't match number of hyperedges");
  }
  if (static_cast<int>(io_sizes.size()) != num_hyperedges_) {
    throw std::invalid_argument("Number of IO sizes doesn't match number of hyperedges");
  }
  
  // Setup hyperedge CSR format: each hyperedge is a set of vertices
  eptr_.reserve(num_hyperedges_ + 1);
  eptr_.push_back(0);
  
  // Precompute eind_ size to avoid reallocations
  size_t total_vertex_refs = 0;
  for (const auto& hyperedge : hyperedges) {
    total_vertex_refs += hyperedge.size();
  }
  eind_.reserve(total_vertex_refs);
  
  for (const auto& hyperedge : hyperedges) {
    eind_.insert(eind_.end(), hyperedge.begin(), hyperedge.end());
    eptr_.push_back(static_cast<int>(eind_.size()));
  }

  // Setup vertex CSR format: each vertex is in a set of hyperedges
  // Create temporary vector of hyperedges for each vertex
  std::vector<std::vector<int>> vertices(num_vertices_);
  
  // Reserve space based on average density to reduce reallocations
  if (num_vertices_ > 0) {
    size_t avg_edges_per_vertex = total_vertex_refs / num_vertices_;
    for (auto& vertex_edges : vertices) {
      vertex_edges.reserve(avg_edges_per_vertex);
    }
  }
  
  for (int e = 0; e < num_hyperedges_; e++) {
    for (auto v : hyperedges[e]) {
      if (v < 0 || v >= num_vertices_) {
        throw std::out_of_range("Vertex index out of range in hyperedge");
      }
      vertices[v].push_back(e);
    }
  }

  // Convert to CSR format
  vptr_.reserve(num_vertices_ + 1);
  vptr_.push_back(0);
  
  // Precompute vind_ size
  total_vertex_refs = 0;
  for (const auto& vertex : vertices) {
    total_vertex_refs += vertex.size();
  }
  vind_.reserve(total_vertex_refs);
  
  for (const auto& vertex : vertices) {
    vind_.insert(vind_.end(), vertex.begin(), vertex.end());
    vptr_.push_back(static_cast<int>(vind_.size()));
  }
}

void Hypergraph::validateVertexId(int vertex_id, const char* func_name) const {
  if (vertex_id < 0 || vertex_id >= num_vertices_) {
    throw std::out_of_range(std::string(func_name) + ": Vertex ID out of range");
  }
}

void Hypergraph::validateEdgeId(int edge_id, const char* func_name) const {
  if (edge_id < 0 || edge_id >= num_hyperedges_) {
    throw std::out_of_range(std::string(func_name) + ": Hyperedge ID out of range");
  }
}

void Hypergraph::SetReach(const std::vector<float>& reaches) {
  if (static_cast<int>(reaches.size()) != num_hyperedges_) {
    throw std::invalid_argument("SetReach: Size of reaches doesn't match number of hyperedges");
  }
  reaches_ = reaches;
}

void Hypergraph::SetReach(int hyperedge_id, float val) {
  validateEdgeId(hyperedge_id, "SetReach");
  reaches_[hyperedge_id] = val;
}

float Hypergraph::GetReach(int hyperedge_id) const {
  validateEdgeId(hyperedge_id, "GetReach");
  return reaches_[hyperedge_id];
}

void Hypergraph::SetIoSize(int hyperedge_id, float val) {
  validateEdgeId(hyperedge_id, "SetIoSize");
  io_sizes_[hyperedge_id] = val;
}

float Hypergraph::GetIoSize(int hyperedge_id) const {
  validateEdgeId(hyperedge_id, "GetIoSize");
  return io_sizes_[hyperedge_id];
}

const std::vector<float>& Hypergraph::GetVertexWeights(int vertex_id) const {
  validateVertexId(vertex_id, "GetVertexWeights");
  return vertex_weights_[vertex_id];
}

const std::vector<float>& Hypergraph::GetHyperedgeWeights(int edge_id) const {
  validateEdgeId(edge_id, "GetHyperedgeWeights");
  return hyperedge_weights_[edge_id];
}

boost::iterator_range<std::vector<int>::const_iterator> Hypergraph::Vertices(int edge_id) const {
  validateEdgeId(edge_id, "Vertices");
  auto begin_iter = eind_.cbegin();
  return boost::make_iterator_range(begin_iter + eptr_[edge_id],
                                    begin_iter + eptr_[edge_id + 1]);
}

boost::iterator_range<std::vector<int>::const_iterator> Hypergraph::Edges(int node_id) const {
  validateVertexId(node_id, "Edges");
  auto begin_iter = vind_.cbegin();
  return boost::make_iterator_range(begin_iter + vptr_[node_id],
                                    begin_iter + vptr_[node_id + 1]);
}

std::vector<float> Hypergraph::GetTotalVertexWeights() const {
  if (num_vertices_ == 0) {
    return std::vector<float>(vertex_dimensions_, 0.0f);
  }
  
  // Initialize with first vertex weight
  std::vector<float> total_weight(vertex_weights_[0]);
  
  // Add remaining vertex weights
  for (int i = 1; i < num_vertices_; ++i) {
    for (size_t j = 0; j < total_weight.size(); ++j) {
      total_weight[j] += vertex_weights_[i][j];
    }
  }
  
  return total_weight;
}

Matrix<float> Hypergraph::GetUpperVertexBalance(int num_parts, float ub_factor,
                                              std::vector<float> base_balance) const {
  // Create a copy of base_balance to avoid modifying the input
  std::vector<float> adjusted_balance = base_balance;
  for (auto& value : adjusted_balance) {
    value += ub_factor * 0.01f;
  }
  
  // Get total vertex weights
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  
  // Create and fill upper balance matrix
  Matrix<float> upper_block_balance(num_parts, vertex_balance);
  for (int i = 0; i < num_parts; i++) {
    if (i < static_cast<int>(adjusted_balance.size())) {
      upper_block_balance[i] = MultiplyFactor(upper_block_balance[i], adjusted_balance[i]);
    }
  }
  
  return upper_block_balance;
}

Matrix<float> Hypergraph::GetLowerVertexBalance(int num_parts, float ub_factor,
                                              std::vector<float> base_balance) const {
  // Create a copy of base_balance to avoid modifying the input
  std::vector<float> adjusted_balance = base_balance;
  for (auto& value : adjusted_balance) {
    value -= ub_factor * 0.01f;
    if (value <= 0.0f) {
      value = 0.0f;
    }
  }
  
  // Get total vertex weights
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  
  // Create and fill lower balance matrix
  Matrix<float> lower_block_balance(num_parts, vertex_balance);
  for (int i = 0; i < num_parts; i++) {
    if (i < static_cast<int>(adjusted_balance.size())) {
      lower_block_balance[i] = MultiplyFactor(lower_block_balance[i], adjusted_balance[i]);
    }
  }
  
  return lower_block_balance;
}

std::vector<int> Hypergraph::GetNeighbors(int vertex_id) const {
  validateVertexId(vertex_id, "GetNeighbors");
  
  // Use a set for efficient duplicate elimination
  std::set<int> neighbors;
  
  // Iterate through hyperedges connected to this vertex
  for (const auto& hyperedge_id : Edges(vertex_id)) {
    // For each hyperedge, get all vertices it connects
    for (const auto& v : Vertices(hyperedge_id)) {
      // Skip the original vertex
      if (v != vertex_id) {
        neighbors.insert(v);
      }
    }
  }
  
  return std::vector<int>(neighbors.begin(), neighbors.end());
}

void Hypergraph::WriteChipletNetlist(const std::string& file_name) const {
  std::ofstream file_output(file_name);
  if (!file_output.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + file_name);
  }
  
  // Write header with counts
  file_output << GetNumHyperedges() << "  " << GetNumVertices() << " 11"
              << std::endl;

  // Write hyperedges and their vertices
  for (int i = 0; i < GetNumHyperedges(); i++) {
    file_output << "196 2000 "; // Default values for reach and bandwidth
    for (const int vertex : Vertices(i)) {
      file_output << vertex + 1 << " "; // +1 because file format uses 1-based indexing
    }
    file_output << std::endl;
  }

  // Write vertex weights
  for (int v = 0; v < GetNumVertices(); v++) {
    const auto& weights = GetVertexWeights(v);
    if (!weights.empty()) {
      file_output << weights[0] << std::endl;
    } else {
      file_output << "0" << std::endl; // Default value if no weights
    }
  }
  
  file_output.close();
}

} // namespace chiplet