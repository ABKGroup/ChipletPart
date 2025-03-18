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
#pragma once

#include <vector>
#include <memory>
#include <limits>
#include <cmath>
#include "Hypergraph.h"

namespace chiplet {

/**
 * @brief Represents the gain of moving a vertex between partitions.
 * 
 * VertexGain is the basic element of the Fiduccia-Mattheyses algorithm.
 * It stores information about a potential vertex move between partitions
 * and the associated gain (benefit) of making that move.
 */
class VertexGain
{
 public:
  // Default constructor
  VertexGain() = default;
  
  /**
   * @brief Construct a new Vertex Gain object
   * 
   * @param vertex Vertex ID
   * @param src_block_id Source partition ID
   * @param destination_block_id Destination partition ID
   * @param gain Gain value associated with moving the vertex
   */
  VertexGain(int vertex,
             int src_block_id,
             int destination_block_id,
             float gain) : 
    vertex_(vertex),
    source_part_(src_block_id),
    destination_part_(destination_block_id),
    gain_(gain) {}

  // Accessor methods
  [[nodiscard]] int GetVertex() const noexcept { return vertex_; }
  void SetVertex(int vertex) noexcept { vertex_ = vertex; }

  [[nodiscard]] float GetGain() const noexcept { return gain_; }
  void SetGain(float gain) noexcept { gain_ = gain; }

  [[nodiscard]] int GetSourcePart() const noexcept { return source_part_; }
  [[nodiscard]] int GetDestinationPart() const noexcept { return destination_part_; }

 private:
  int vertex_ = -1;            // vertex id
  int source_part_ = -1;       // the source block id
  int destination_part_ = -1;  // the destination block id
  float gain_ = -std::numeric_limits<float>::max();  // gain value of moving this vertex
};

// ------------------------------------------------------------
// Priority-queue based gain bucket (Only for VertexGain)
// Actually we implement the priority queue with Max Heap
// We did not use the STL priority queue becuase we need
// to record the location of each element (vertex gain)
// -------------------------------------------------------------
class PriorityQueue
{
 public:
  /**
   * @brief Construct a new Priority Queue object
   * 
   * @param total_elements Number of elements the queue can hold
   * @param maximum_traverse_level Maximum levels to traverse when finding candidates
   * @param hypergraph Pointer to the hypergraph
   */
  PriorityQueue(int total_elements,
                int maximum_traverse_level,
                HGraphPtr hypergraph);

  /**
   * @brief Insert a new element into the priority queue
   * 
   * @param element The VertexGain element to insert
   */
  void InsertIntoPQ(const std::shared_ptr<VertexGain>& element);

  /**
   * @brief Extract the element with the highest gain
   * 
   * @return std::shared_ptr<VertexGain> The highest gain element
   */
  std::shared_ptr<VertexGain> ExtractMax();

  /**
   * @brief Get the maximum element without removing it
   * 
   * @return std::shared_ptr<VertexGain> The highest gain element
   */
  [[nodiscard]] std::shared_ptr<VertexGain> GetMax() const noexcept { 
    return vertices_.empty() ? nullptr : vertices_.front(); 
  }

  /**
   * @brief Find the best candidate that satisfies balance constraints
   * 
   * @param curr_block_balance Current balance of each block
   * @param upper_block_balance Upper balance constraints
   * @param lower_block_balance Lower balance constraints
   * @param hgraph Pointer to the hypergraph
   * @return std::shared_ptr<VertexGain> The best candidate
   */
  std::shared_ptr<VertexGain> GetBestCandidate(
      const Matrix<float>& curr_block_balance,
      const Matrix<float>& upper_block_balance,
      const Matrix<float>& lower_block_balance,
      const HGraphPtr& hgraph);

  /**
   * @brief Update the priority (gain) for a specific vertex
   * 
   * @param vertex_id Vertex ID to update
   * @param new_element New VertexGain object
   */
  void ChangePriority(int vertex_id,
                      const std::shared_ptr<VertexGain>& new_element);

  /**
   * @brief Remove a vertex from the queue
   * 
   * @param vertex_id Vertex ID to remove
   */
  void Remove(int vertex_id);

  // Basic accessors
  [[nodiscard]] bool CheckIfEmpty() const noexcept { return vertices_.empty(); }
  [[nodiscard]] int GetTotalElements() const noexcept { return total_elements_; }
  [[nodiscard]] int GetSizeOfMap() const noexcept { return vertices_map_.size(); }
  [[nodiscard]] bool CheckIfVertexExists(int v) const noexcept { return v >= 0 && v < static_cast<int>(vertices_map_.size()) && vertices_map_[v] > -1; }
  void SetActive() noexcept { active_ = true; }
  void SetDeactive() noexcept { active_ = false; }
  [[nodiscard]] bool GetStatus() const noexcept { return active_; }
  void Clear();
  [[nodiscard]] int GetSize() const noexcept { return static_cast<int>(vertices_.size()); }

 private:
  // The max heap (priority queue) is organized as a binary tree
  // Helper methods for heap operations
  [[nodiscard]] inline int Parent(int element) const noexcept { 
    return (element - 1) / 2; 
  }

  [[nodiscard]] inline int LeftChild(int element) const noexcept { 
    return 2 * element + 1; 
  }

  [[nodiscard]] inline int RightChild(int element) const noexcept { 
    return 2 * element + 2; 
  }

  // Heap maintenance operations
  void HeapifyUp(int index);
  void HeapifyDown(int index);

  /**
   * @brief Compare two elements in the heap
   * 
   * @param index_a First element index
   * @param index_b Second element index
   * @return true if element at index_a has higher priority
   */
  [[nodiscard]] bool CompareElementLargeThan(int index_a, int index_b) const;

  // Member variables
  bool active_ = false;
  HGraphPtr hypergraph_;
  std::vector<std::shared_ptr<VertexGain>> vertices_;  // Heap elements
  std::vector<int> vertices_map_;  // Maps vertex IDs to heap positions
  int total_elements_ = 0;         // Number of elements in the queue
  int maximum_traverse_level_ = 25;  // Maximum level to traverse when finding candidates
};

}  // namespace chiplet
