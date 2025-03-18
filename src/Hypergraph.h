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
// High-level description
// This file includes the basic data structure for hypergraph,
// vertex, hyperedge and timing paths. We also explain our basic
// conventions.
// Rule1 : num_vertices, num_hyperedges, vertex_dimensions,
//         hyperedge_dimensions, placement_dimension,
//         cluster_id (c), vertex_id (v), hyperedge_id (e)
//         are all in int type.
// Rule2 : Each hyperedge can include a vertex at most once.
////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Utilities.h"
#include <boost/range/iterator_range.hpp>
#include <functional>
#include <memory>
#include <set>
#include <vector>
#include <string>

namespace chiplet {

/**
 * @brief Class representing a hypergraph data structure
 * 
 * A hypergraph is a generalization of a graph where edges (hyperedges)
 * can connect any number of vertices. This implementation uses a
 * Compressed Sparse Row (CSR) representation for memory efficiency.
 */
class Hypergraph;
using HGraphPtr = std::shared_ptr<Hypergraph>;

class Hypergraph {
public:
  /**
   * @brief Construct a new Hypergraph object
   * 
   * @param vertex_dimensions Number of dimensions for vertex weights
   * @param hyperedge_dimensions Number of dimensions for hyperedge weights
   * @param hyperedges Vector of hyperedges, where each hyperedge is a vector of vertex indices
   * @param vertex_weights Vector of vertex weights, where each vertex has a vector of weight values
   * @param hyperedge_weights Vector of hyperedge weights, where each hyperedge has a vector of weight values
   * @param reaches Vector of reach values for each hyperedge
   * @param io_sizes Vector of I/O sizes for each hyperedge
   */
  Hypergraph(int vertex_dimensions, int hyperedge_dimensions,
             const std::vector<std::vector<int>>& hyperedges,
             const std::vector<std::vector<float>>& vertex_weights,
             const std::vector<std::vector<float>>& hyperedge_weights,
             const std::vector<float>& reaches,
             const std::vector<float>& io_sizes);

  /**
   * @brief Get the number of vertices in the hypergraph
   * @return Number of vertices
   */
  int GetNumVertices() const noexcept { return num_vertices_; }
  
  /**
   * @brief Get the number of hyperedges in the hypergraph
   * @return Number of hyperedges
   */
  int GetNumHyperedges() const noexcept { return num_hyperedges_; }
  
  /**
   * @brief Get the number of dimensions for vertex weights
   * @return Number of vertex weight dimensions
   */
  int GetVertexDimensions() const noexcept { return vertex_dimensions_; }
  
  /**
   * @brief Get the number of dimensions for hyperedge weights
   * @return Number of hyperedge weight dimensions
   */
  int GetHyperedgeDimensions() const noexcept { return hyperedge_dimensions_; }
  
  /**
   * @brief Set reach values for all hyperedges
   * @param reaches Vector of reach values
   * @throws std::invalid_argument If reaches size doesn't match number of hyperedges
   */
  void SetReach(const std::vector<float>& reaches);

  /**
   * @brief Set reach value for a specific hyperedge
   * @param hyperedge_id Hyperedge ID
   * @param val Reach value
   * @throws std::out_of_range If hyperedge_id is invalid
   */
  void SetReach(int hyperedge_id, float val);

  /**
   * @brief Get reach value for a specific hyperedge
   * @param hyperedge_id Hyperedge ID
   * @return Reach value
   * @throws std::out_of_range If hyperedge_id is invalid
   */
  float GetReach(int hyperedge_id) const;

  /**
   * @brief Set I/O size for a specific hyperedge
   * @param hyperedge_id Hyperedge ID
   * @param val I/O size value
   * @throws std::out_of_range If hyperedge_id is invalid
   */
  void SetIoSize(int hyperedge_id, float val);

  /**
   * @brief Get I/O size for a specific hyperedge
   * @param hyperedge_id Hyperedge ID
   * @return I/O size value
   * @throws std::out_of_range If hyperedge_id is invalid
   */
  float GetIoSize(int hyperedge_id) const;

  /**
   * @brief Get the sum of all vertex weights
   * @return Vector of summed weights (one per dimension)
   */
  std::vector<float> GetTotalVertexWeights() const;

  /**
   * @brief Get weights for a specific vertex
   * @param vertex_id Vertex ID
   * @return Vector of weight values
   * @throws std::out_of_range If vertex_id is invalid
   */
  const std::vector<float>& GetVertexWeights(int vertex_id) const;
  
  /**
   * @brief Get all vertex weights
   * @return Matrix of all vertex weights
   */
  const Matrix<float>& GetVertexWeights() const noexcept { return vertex_weights_; }

  /**
   * @brief Get weights for a specific hyperedge
   * @param edge_id Hyperedge ID
   * @return Vector of weight values
   * @throws std::out_of_range If edge_id is invalid
   */
  const std::vector<float>& GetHyperedgeWeights(int edge_id) const;
  
  /**
   * @brief Get all vertices connected by a hyperedge
   * @param edge_id Hyperedge ID
   * @return Range of vertex IDs
   * @throws std::out_of_range If edge_id is invalid
   */
  boost::iterator_range<std::vector<int>::const_iterator> Vertices(int edge_id) const;

  /**
   * @brief Get all hyperedges connected to a vertex
   * @param node_id Vertex ID
   * @return Range of hyperedge IDs
   * @throws std::out_of_range If node_id is invalid
   */
  boost::iterator_range<std::vector<int>::const_iterator> Edges(int node_id) const;

  /**
   * @brief Write hypergraph to a netlist file
   * @param file_name Output file name
   * @throws std::runtime_error If file cannot be opened
   */
  void WriteChipletNetlist(const std::string& file_name) const;
  
  /**
   * @brief Get upper balance constraints for vertices
   * @param num_parts Number of partitions
   * @param ub_factor Upper bound factor
   * @param base_balance Base balance values
   * @return Matrix of upper balance constraints
   */
  Matrix<float> GetUpperVertexBalance(int num_parts, float ub_factor,
                                      std::vector<float> base_balance) const;

  /**
   * @brief Get lower balance constraints for vertices
   * @param num_parts Number of partitions
   * @param ub_factor Upper bound factor
   * @param base_balance Base balance values
   * @return Matrix of lower balance constraints
   */
  Matrix<float> GetLowerVertexBalance(int num_parts, float ub_factor,
                                      std::vector<float> base_balance) const;

  /**
   * @brief Get all neighbors of a vertex
   * @param vertex_id Vertex ID
   * @return Vector of neighbor vertex IDs
   * @throws std::out_of_range If vertex_id is invalid
   */
  std::vector<int> GetNeighbors(int vertex_id) const;

private:
  /**
   * @brief Validate a vertex ID
   * @param vertex_id Vertex ID to validate
   * @param func_name Function name for error message
   * @throws std::out_of_range If vertex_id is invalid
   */
  void validateVertexId(int vertex_id, const char* func_name) const;
  
  /**
   * @brief Validate a hyperedge ID
   * @param edge_id Hyperedge ID to validate
   * @param func_name Function name for error message
   * @throws std::out_of_range If edge_id is invalid
   */
  void validateEdgeId(int edge_id, const char* func_name) const;

  // Basic hypergraph properties
  const int num_vertices_ = 0;       ///< Number of vertices
  const int num_hyperedges_ = 0;     ///< Number of hyperedges
  const int vertex_dimensions_ = 1;  ///< Number of dimensions for vertex weights
  const int hyperedge_dimensions_ = 1; ///< Number of dimensions for hyperedge weights

  // Weight data
  const Matrix<float> vertex_weights_;     ///< Weights for vertices
  const Matrix<float> hyperedge_weights_;  ///< Weights for hyperedges (can be negative)
  
  // CSR representation for hyperedges (each hyperedge is a set of vertices)
  std::vector<int> eind_;  ///< Concatenated lists of vertices for each hyperedge
  std::vector<int> eptr_;  ///< Pointers to where each hyperedge's vertices begin in eind_

  // CSR representation for vertices (each vertex is in a set of hyperedges)
  std::vector<int> vind_;  ///< Concatenated lists of hyperedges for each vertex
  std::vector<int> vptr_;  ///< Pointers to where each vertex's hyperedges begin in vind_
  
  // Additional hyperedge properties
  std::vector<float> reaches_;   ///< Reach values for hyperedges
  std::vector<float> io_sizes_;  ///< I/O sizes for hyperedges
};

} // namespace chiplet
