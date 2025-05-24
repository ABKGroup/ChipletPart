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

#include "PriorityQueue.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <utility>
#include <limits>
#include "Hypergraph.h"

namespace chiplet {

PriorityQueue::PriorityQueue(const int total_elements,
                             const int maximum_traverse_level,
                             HGraphPtr hypergraph)
    : maximum_traverse_level_(maximum_traverse_level),
      hypergraph_(std::move(hypergraph))
{
  // Pre-allocate memory for vertices vector to avoid frequent reallocations
  vertices_.reserve(total_elements);
  
  // Initialize vertices_map_ with -1 values (indicating vertex not in heap)
  vertices_map_.resize(total_elements, -1);
}

void PriorityQueue::Clear()
{
  active_ = false;
  vertices_.clear();
  total_elements_ = 0;
  std::fill(vertices_map_.begin(), vertices_map_.end(), -1);
}

void PriorityQueue::InsertIntoPQ(const std::shared_ptr<VertexGain>& element)
{
  // Validate input element
  assert(element && "Cannot insert null element into priority queue");
  const int vertex_id = element->GetVertex();
  assert(vertex_id >= 0 && vertex_id < static_cast<int>(vertices_map_.size()) && 
         "Vertex ID out of bounds");
  
  // Add element to the heap
  vertices_.push_back(element);
  vertices_map_[vertex_id] = total_elements_;
  total_elements_++;
  
  // Restore heap property
  HeapifyUp(total_elements_ - 1);
}

std::shared_ptr<VertexGain> PriorityQueue::ExtractMax()
{
  if (total_elements_ == 0) {
    return nullptr;
  }
  
  // Get top element
  auto max_element = vertices_.front();
  const int vertex_id = max_element->GetVertex();
  
  // Replace top with the last element
  if (total_elements_ > 1) {
    vertices_[0] = vertices_[total_elements_ - 1];
    vertices_map_[vertices_[0]->GetVertex()] = 0;
  }
  
  // Remove the last element (which is now at position 0)
  vertices_.pop_back();
  total_elements_--;
  
  // Mark vertex as no longer in the heap
  vertices_map_[vertex_id] = -1;
  
  // Restore heap property if we have elements left
  if (total_elements_ > 0) {
    HeapifyDown(0);
  }
  
  return max_element;
}

std::shared_ptr<VertexGain> PriorityQueue::GetBestCandidate(
    const Matrix<float>& curr_block_balance,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance,
    const HGraphPtr& hgraph)
{
  if (total_elements_ <= 0) {
    return std::make_shared<VertexGain>();  // Return a dummy cell
  }
  
  // Lambda to check balance constraints
  auto CheckBalance = [&](int index) -> bool {
    const int vertex_id = vertices_[index]->GetVertex();
    const int to_pid = vertices_[index]->GetDestinationPart();
    const int from_pid = vertices_[index]->GetSourcePart();
    
    // Get vertex weights and current block balances
    const std::vector<float>& vertex_weights = hgraph->GetVertexWeights(vertex_id);
    const std::vector<float>& to_block_balance = curr_block_balance[to_pid];
    const std::vector<float>& from_block_balance = curr_block_balance[from_pid];
    const std::vector<float>& upper_to_balance = upper_block_balance[to_pid];
    const std::vector<float>& lower_from_balance = lower_block_balance[from_pid];
    
    // Check vector sizes
    if (vertex_weights.size() != to_block_balance.size() || 
        vertex_weights.size() != from_block_balance.size() ||
        to_block_balance.size() != upper_to_balance.size() || 
        from_block_balance.size() != lower_from_balance.size()) {
      // Size mismatch, return false
      std::cerr << "Vector size mismatch in PriorityQueue::GetMax(): "
                << "vertex_weights.size()=" << vertex_weights.size() 
                << ", to_block_balance.size()=" << to_block_balance.size()
                << ", from_block_balance.size()=" << from_block_balance.size()
                << ", upper_to_balance.size()=" << upper_to_balance.size()
                << ", lower_from_balance.size()=" << lower_from_balance.size()
                << std::endl;
      return false;
    }
    
    // Check constraints element-wise
    bool satisfies_upper = true;
    bool satisfies_lower = true;
    
    for (size_t i = 0; i < vertex_weights.size(); ++i) {
      if (to_block_balance[i] + vertex_weights[i] >= upper_to_balance[i]) {
        satisfies_upper = false;
        break;
      }
    }
    
    for (size_t i = 0; i < vertex_weights.size(); ++i) {
      if (from_block_balance[i] - vertex_weights[i] <= lower_from_balance[i]) {
        satisfies_lower = false;
        break;
      }
    }
    
    return satisfies_upper && satisfies_lower;
  };
  
  // Check the root node first (highest gain element)
  if (CheckBalance(0)) {
    return vertices_[0];
  }
  
  // If root doesn't satisfy balance constraints, search through the heap
  int pass = 0;
  int candidate_index = -1;
  int index = 0;
  
  // Breadth-first traversal of the heap to find a valid candidate
  while (pass < maximum_traverse_level_) {
    pass++;
    
    // Check left child
    const int left_child = LeftChild(index);
    if (left_child < total_elements_ && CheckBalance(left_child)) {
      candidate_index = left_child;
    }
    
    // Check right child
    const int right_child = RightChild(index);
    if (right_child < total_elements_ && CheckBalance(right_child) &&
        (candidate_index == -1 || CompareElementLargeThan(right_child, candidate_index))) {
      candidate_index = right_child;
    }
    
    // If we found a valid candidate, return it
    if (candidate_index > -1) {
      return vertices_[candidate_index];
    }
    
    // If we can't go deeper in the tree, return a dummy cell
    if (left_child >= total_elements_ || right_child >= total_elements_) {
      return std::make_shared<VertexGain>();
    }
    
    // Continue traversal with the child that has higher gain
    index = CompareElementLargeThan(right_child, left_child) ? right_child : left_child;
  }
  
  // If we've reached the maximum traversal level without finding a candidate
  return std::make_shared<VertexGain>();
}

void PriorityQueue::Remove(int vertex_id)
{
  // Validate vertex exists in the heap
  if (vertex_id < 0 || vertex_id >= static_cast<int>(vertices_map_.size())) {
    return;
  }
  
  const int index = vertices_map_[vertex_id];
  if (index == -1) {
    return;  // Vertex not in the heap
  }
  
  // Move the vertex to the top by setting a very high gain
  if (GetMax() != nullptr) {
    vertices_[index]->SetGain(GetMax()->GetGain() + 1.0f);
    HeapifyUp(index);
    
    // Now it should be at the top, so extract it
    ExtractMax();
  }
  
  // Update active status if heap becomes empty
  if (total_elements_ <= 0) {
    active_ = false;
  }
}

void PriorityQueue::ChangePriority(
    int vertex_id,
    const std::shared_ptr<VertexGain>& new_element)
{
  // Validate vertex exists in the heap
  if (vertex_id < 0 || vertex_id >= static_cast<int>(vertices_map_.size())) {
    return;
  }
  
  const int index = vertices_map_[vertex_id];
  if (index == -1) {
    return;  // Vertex not in the heap
  }
  
  // Store old gain for comparison
  const float old_gain = vertices_[index]->GetGain();
  
  // Update the element
  vertices_[index] = new_element;
  
  // Restore heap property based on whether gain increased or decreased
  if (new_element->GetGain() > old_gain) {
    HeapifyUp(index);
  } else if (new_element->GetGain() < old_gain) {
    HeapifyDown(index);
  }
}

bool PriorityQueue::CompareElementLargeThan(int index_a, int index_b) const
{
  // Return true if element at index_a has higher priority than element at index_b
  
  // First compare by gain value
  const float gain_a = vertices_[index_a]->GetGain();
  const float gain_b = vertices_[index_b]->GetGain();
  
  if (gain_a > gain_b) {
    return true;
  }
  
  // If gains are equal, compare by vertex weight (smaller is better)
  if (std::abs(gain_a - gain_b) < 1e-6) {
    return hypergraph_->GetVertexWeights(vertices_[index_a]->GetVertex()) < 
           hypergraph_->GetVertexWeights(vertices_[index_b]->GetVertex());
  }
  
  return false;
}

void PriorityQueue::HeapifyUp(int index)
{
  while (index > 0) {
    const int parent_index = Parent(index);
    
    // If element has higher priority than parent, swap them
    if (CompareElementLargeThan(index, parent_index)) {
      // Update the vertices_map_ to reflect the swap
      const int parent_vertex = vertices_[parent_index]->GetVertex();
      const int child_vertex = vertices_[index]->GetVertex();
      
      vertices_map_[parent_vertex] = index;
      vertices_map_[child_vertex] = parent_index;
      
      // Swap the elements
      std::swap(vertices_[parent_index], vertices_[index]);
      
      // Move up in the tree
      index = parent_index;
    } else {
      // Heap property restored
      break;
    }
  }
}

void PriorityQueue::HeapifyDown(int index)
{
  while (true) {
    int largest_index = index;
    const int left_child = LeftChild(index);
    const int right_child = RightChild(index);
    
    // Check if left child has higher priority
    if (left_child < total_elements_ && CompareElementLargeThan(left_child, largest_index)) {
      largest_index = left_child;
    }
    
    // Check if right child has higher priority
    if (right_child < total_elements_ && CompareElementLargeThan(right_child, largest_index)) {
      largest_index = right_child;
    }
    
    // If no swap needed, heap property is restored
    if (largest_index == index) {
      break;
    }
    
    // Update the vertices_map_ to reflect the swap
    const int current_vertex = vertices_[index]->GetVertex();
    const int largest_vertex = vertices_[largest_index]->GetVertex();
    
    vertices_map_[current_vertex] = largest_index;
    vertices_map_[largest_vertex] = index;
    
    // Swap the elements
    std::swap(vertices_[index], vertices_[largest_index]);
    
    // Continue down the tree
    index = largest_index;
  }
}

}  // namespace chiplet
