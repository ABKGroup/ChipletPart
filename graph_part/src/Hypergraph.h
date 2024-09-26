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

namespace chiplet {

class Hypergraph;
using HGraphPtr = std::shared_ptr<Hypergraph>;

class Hypergraph {
public:
  Hypergraph(int vertex_dimensions, int hyperedge_dimensions,
             const std::vector<std::vector<int>> &hyperedges,
             const std::vector<std::vector<float>> &vertex_weights,
             const std::vector<std::vector<float>> &hyperedge_weights,
             const std::vector<float> &reaches,
             const std::vector<float> &io_sizes);

  int GetNumVertices() const { return num_vertices_; }
  int GetNumHyperedges() const { return num_hyperedges_; }
  int GetVertexDimensions() const { return vertex_dimensions_; }
  int GetHyperedgeDimensions() const { return hyperedge_dimensions_; }
  void SetReach(const std::vector<float> &reaches) { reaches_ = reaches; }

  void SetReach(int hyperedge_id, float val) { reaches_[hyperedge_id] = val; }

  float GetReach(const int hyperedge_id) const {
    return reaches_[hyperedge_id];
  }

  void SetIoSize(int hyperedge_id, float val) { io_sizes_[hyperedge_id] = val; }

  float GetIoSize(const int hyperedge_id) const {
    return io_sizes_[hyperedge_id];
  }

  std::vector<float> GetTotalVertexWeights() const;

  const std::vector<float> &GetVertexWeights(const int vertex_id) const {
    return vertex_weights_[vertex_id];
  }
  const Matrix<float> &GetVertexWeights() const { return vertex_weights_; }

  const std::vector<float> &GetHyperedgeWeights(const int edge_id) const {
    return hyperedge_weights_[edge_id];
  } // Returns the vertex ids connected by the hyper edge

  auto Vertices(const int edge_id) const {
    auto begin_iter = eind_.cbegin();
    return boost::make_iterator_range(begin_iter + eptr_[edge_id],
                                      begin_iter + eptr_[edge_id + 1]);
  }

  // Returns the hyperedge ids connected by the node
  auto Edges(const int node_id) const {
    auto begin_iter = vind_.cbegin();
    return boost::make_iterator_range(begin_iter + vptr_[node_id],
                                      begin_iter + vptr_[node_id + 1]);
  }

  void WriteChipletNetlist(const std::string &file_name) const;
  // get balance constraints
  std::vector<std::vector<float>>
  GetUpperVertexBalance(int num_parts, float ub_factor,
                        std::vector<float> base_balance) const;

  std::vector<std::vector<float>>
  GetLowerVertexBalance(int num_parts, float ub_factor,
                        std::vector<float> base_balance) const;

  std::vector<int> GetNeighbors(int vertex_id) const {
    std::set<int> neighbors;
    for (const auto &hyperedge_id : Edges(vertex_id)) {
      for (const auto &v : Vertices(hyperedge_id)) {
        if (v != vertex_id) {
          neighbors.insert(v);
        }
      }
    }
    return std::vector<int>(neighbors.begin(), neighbors.end());
  }

private:
  // basic hypergraph
  const int num_vertices_ = 0;
  const int num_hyperedges_ = 0;
  const int vertex_dimensions_ = 1;
  const int hyperedge_dimensions_ = 1;

  const Matrix<float> vertex_weights_;
  const Matrix<float> hyperedge_weights_; // weights can be negative
  // hyperedges: each hyperedge is a set of vertices
  std::vector<int> eind_;
  std::vector<int> eptr_;

  // vertices: each vertex is a set of hyperedges
  std::vector<int> vind_;
  std::vector<int> vptr_;
  // store reach here
  std::vector<float> reaches_;
  std::vector<float> io_sizes_;
};

} // namespace chiplet
