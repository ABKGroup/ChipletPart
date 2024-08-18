#include "Hypergraph.h"

#include <fstream>
#include <iostream>
#include <string>

#include "Utilities.h"

namespace chiplet {

Hypergraph::Hypergraph(int vertex_dimensions, int hyperedge_dimensions,
                       const std::vector<std::vector<int>> &hyperedges,
                       const std::vector<std::vector<float>> &vertex_weights,
                       const std::vector<std::vector<float>> &hyperedge_weights)
    : num_vertices_(static_cast<int>(vertex_weights.size())),
      num_hyperedges_(static_cast<int>(hyperedge_weights.size())),
      vertex_dimensions_(vertex_dimensions),
      hyperedge_dimensions_(hyperedge_dimensions),
      vertex_weights_(vertex_weights), hyperedge_weights_(hyperedge_weights) {
  // add hyperedge
  // hyperedges: each hyperedge is a set of vertices
  eptr_.push_back(0);
  for (const auto &hyperedge : hyperedges) {
    eind_.insert(eind_.end(), hyperedge.begin(), hyperedge.end());
    eptr_.push_back(static_cast<int>(eind_.size()));
  }

  // add vertex
  // create vertices from hyperedges
  std::vector<std::vector<int>> vertices(num_vertices_);
  for (int e = 0; e < num_hyperedges_; e++) {
    for (auto v : hyperedges[e]) {
      vertices[v].push_back(e); // e is the hyperedge id
    }
  }

  vptr_.push_back(0);
  for (const auto &vertex : vertices) {
    vind_.insert(vind_.end(), vertex.begin(), vertex.end());
    vptr_.push_back(static_cast<int>(vind_.size()));
  }
}

std::vector<float> Hypergraph::GetTotalVertexWeights() const {
  std::vector<float> total_weight(vertex_dimensions_, 0.0);
  for (auto &weight : vertex_weights_) {
    total_weight = total_weight + weight;
  }
  return total_weight;
}

std::vector<std::vector<float>>
Hypergraph::GetUpperVertexBalance(int num_parts, float ub_factor,
                                  std::vector<float> base_balance) const {
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  for (auto &value : base_balance) {
    value += ub_factor * 0.01;
  }
  std::vector<std::vector<float>> upper_block_balance(num_parts,
                                                      vertex_balance);
  for (int i = 0; i < num_parts; i++) {
    upper_block_balance[i] =
        MultiplyFactor(upper_block_balance[i], base_balance[i]);
  }
  return upper_block_balance;
}

std::vector<std::vector<float>>
Hypergraph::GetLowerVertexBalance(int num_parts, float ub_factor,
                                  std::vector<float> base_balance) const {
  std::vector<float> vertex_balance = GetTotalVertexWeights();
  for (auto &value : base_balance) {
    value -= ub_factor * 0.01;
    if (value <= 0.0) {
      value = 0.0;
    }
  }
  std::vector<std::vector<float>> lower_block_balance(num_parts,
                                                      vertex_balance);
  for (int i = 0; i < num_parts; i++) {
    lower_block_balance[i] =
        MultiplyFactor(lower_block_balance[i], base_balance[i]);
  }
  return lower_block_balance;
}

void Hypergraph::WriteChipletNetlist(const std::string &file_name) const {
  std::ofstream file_output;
  file_output.open(file_name);
  file_output << GetNumHyperedges() << "  " << GetNumVertices() << " 11"
              << std::endl;

  for (int i = 0; i < GetNumHyperedges(); i++) {
    file_output
        << "196 2000 "; // this needs to change based on reach and bandwidth
    for (const int vertex : Vertices(i)) {
      file_output << vertex + 1 << " ";
    }
    file_output << std::endl;
  }

  // write vertex weight
  for (int v = 0; v < GetNumVertices(); v++) {
    file_output << GetVertexWeights(v)[0] << std::endl;
  }
  // close the file
  file_output.close();
}

} // namespace chiplet
