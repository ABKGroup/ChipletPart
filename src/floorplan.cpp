///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2024, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "floorplan.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <vector>

namespace chiplet {

// Class SACore
SACore::SACore(
    std::vector<Chiplet> chiplets,
    std::vector<BundledNet> bundled_nets,
    // penalty parameters
    float area_penalty_weight,
    float package_penalty_weight,
    float net_penalty_weight,
    // operation parameters
    float pos_swap_prob,
    float neg_swap_prob,
    float double_swap_prob,
    float resize_prob,
    float expand_prob,
    // SA parameters
    int max_num_step,
    int num_perturb_per_step,
    float cooling_rate,
    unsigned seed,
    std::vector<int>& pos_seq,
    std::vector<int>& neg_seq)
{
  // penalty parameters
  area_penalty_weight_ = area_penalty_weight;
  package_penalty_weight_ = package_penalty_weight;
  net_penalty_weight_ = net_penalty_weight;
  
  // operation parameters
  pos_swap_prob_ = pos_swap_prob;
  neg_swap_prob_ = neg_swap_prob;
  double_swap_prob_ = double_swap_prob;
  resize_prob_ = resize_prob;
  expand_prob_ = expand_prob;
  
  // SA parameters
  max_num_step_ = max_num_step;
  num_perturb_per_step_ = num_perturb_per_step;
  cooling_rate_ = cooling_rate;
  seed_ = seed;
  
  // Initialize random number generator
  generator_ = std::mt19937(seed);
  distribution_ = std::uniform_real_distribution<float>(0.0f, 1.0f);
  
  // Reserve space for efficiency
  const size_t chiplet_count = chiplets.size();
  macros_.reserve(chiplet_count);

  if (pos_seq.empty()) {
    pos_seq_.reserve(chiplet_count);
    neg_seq_.reserve(chiplet_count);
    pre_pos_seq_.reserve(chiplet_count);
    pre_neg_seq_.reserve(chiplet_count);
    
    for (size_t i = 0; i < chiplet_count; i++) {
      pos_seq_.push_back(static_cast<int>(i));
      neg_seq_.push_back(static_cast<int>(i));
      pre_pos_seq_.push_back(static_cast<int>(i));
      pre_neg_seq_.push_back(static_cast<int>(i));
      macros_.push_back(chiplets[i]);
    }
  } else {
    pos_seq_ = pos_seq;
    neg_seq_ = neg_seq;
    pre_pos_seq_ = pos_seq;
    pre_neg_seq_ = neg_seq;
    macros_ = std::move(chiplets);
  }
  
  nets_ = std::move(bundled_nets);
}

void SACore::packFloorplan() {
  const size_t macro_count = macros_.size();
  
  // Reset all positions to origin
  for (auto& macro : macros_) {
    macro.setX(0.0f);
    macro.setY(0.0f);
  }

  // Calculate X position
  // Store the position of each macro in the pos_seq_ and neg_seq_
  std::vector<std::pair<int, int>> match(macro_count);
  for (size_t i = 0; i < macro_count; i++) {
    match[pos_seq_[i]].first = static_cast<int>(i);
    match[neg_seq_[i]].second = static_cast<int>(i);
  }
  
  // Initialize current length
  std::vector<float> length(macro_count, 0.0f);
  
  // Position macros in X direction
  for (size_t i = 0; i < macro_count; i++) {
    const int b = pos_seq_[i];  // macro_id
    
    // Skip fixed terminals or invalid dimensions
    if (macros_[b].getWidth() <= 0.0f || macros_[b].getHeight() <= 0.0f) {
      continue;
    }
    
    const int p = match[b].second;  // the position of current macro in neg_seq_
    macros_[b].setX(length[p]);
    const float right_edge = macros_[b].getX() + macros_[b].getWidth();
    
    // Update lengths efficiently
    for (size_t j = p; j < macro_count; j++) {
      if (right_edge > length[j]) {
        length[j] = right_edge;
      } else {
        break; // Early termination when no more updates needed
      }
    }
  }
  
  // Update width_ of current floorplan
  width_ = length[macro_count - 1];

  // Calculate Y position (reversed order for pos_seq)
  std::vector<int> pos_seq(macro_count);
  for (size_t i = 0; i < macro_count; i++) {
    pos_seq[i] = pos_seq_[macro_count - 1 - i];
  }
  
  // Reset match and length vectors
  for (size_t i = 0; i < macro_count; i++) {
    match[pos_seq[i]].first = static_cast<int>(i);
    match[neg_seq_[i]].second = static_cast<int>(i);
    length[i] = 0.0f;  // Reset length for Y positioning
  }
  
  // Position macros in Y direction
  for (size_t i = 0; i < macro_count; i++) {
    const int b = pos_seq[i];  // macro_id
    
    // Skip fixed terminals or invalid dimensions
    if (macros_[b].getHeight() <= 0.0f || macros_[b].getWidth() <= 0.0f) {
      continue;
    }
    
    const int p = match[b].second;  // the position in neg_seq_
    macros_[b].setY(length[p]);
    const float top_edge = macros_[b].getY() + macros_[b].getHeight();
    
    // Update lengths efficiently
    for (size_t j = p; j < macro_count; j++) {
      if (top_edge > length[j]) {
        length[j] = top_edge;
      } else {
        break; // Early termination
      }
    }
  }
  
  // Update height_ of current floorplan
  height_ = length[macro_count - 1];
}

void SACore::singleSeqSwap(bool pos) {
  const size_t macro_count = macros_.size();
  if (macro_count <= 1) {
    return;
  }

  // Generate two distinct random indices
  const int index1 = static_cast<int>(std::floor(distribution_(generator_) * macro_count));
  int index2;
  do {
    index2 = static_cast<int>(std::floor(distribution_(generator_) * macro_count));
  } while (index1 == index2);

  // Swap elements in the appropriate sequence
  if (pos) {
    std::swap(pos_seq_[index1], pos_seq_[index2]);
  } else {
    std::swap(neg_seq_[index1], neg_seq_[index2]);
  }
}

void SACore::doubleSeqSwap() {
  const size_t macro_count = macros_.size();
  if (macro_count <= 1) {
    return;
  }

  // Generate two distinct random indices
  const int index1 = static_cast<int>(std::floor(distribution_(generator_) * macro_count));
  int index2;
  do {
    index2 = static_cast<int>(std::floor(distribution_(generator_) * macro_count));
  } while (index1 == index2);

  // Swap elements in both sequences
  std::swap(pos_seq_[index1], pos_seq_[index2]);
  std::swap(neg_seq_[index1], neg_seq_[index2]);
}

void SACore::resizeOneCluster() {
  const size_t macro_count = pos_seq_.size();
  const int idx = static_cast<int>(std::floor(distribution_(generator_) * macro_count));
  macro_id_ = idx;
  Chiplet& src_macro = macros_[idx];

  const float lx = src_macro.getX();
  const float ly = src_macro.getY();
  const float ux = lx + src_macro.getWidth();
  const float uy = ly + src_macro.getHeight();

  // Special case: randomly resize with aspect ratio
  if (distribution_(generator_) < 0.2f) {
    float aspect_ratio = distribution_(generator_);
    aspect_ratio = std::clamp(aspect_ratio, 0.2f, 5.0f);
    src_macro.resizeRandomly(aspect_ratio);
    return;
  }

  // Determine the resize operation based on random value
  const float option = distribution_(generator_);
  
  if (option <= 0.25f) {
    // Change the width of soft block to Rb = e.x2 - b.x1
    float e_x2 = width_;
    for (const auto& macro : macros_) {
      const float cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 > ux && cur_x2 < e_x2) {
        e_x2 = cur_x2;
      }
    }
    src_macro.setWidth(e_x2 - lx);
    
  } else if (option <= 0.5f) {
    // Change the width based on closest left edge
    float d_x2 = lx;
    for (const auto& macro : macros_) {
      const float cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 < ux && cur_x2 > d_x2) {
        d_x2 = cur_x2;
      }
    }
    if (d_x2 <= lx) {
      return;
    }
    src_macro.setWidth(d_x2 - lx);
    
  } else if (option <= 0.75f) {
    // Change the height of soft block to Tb = a.y2 - b.y1
    float a_y2 = height_;
    for (const auto& macro : macros_) {
      const float cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 > uy && cur_y2 < a_y2) {
        a_y2 = cur_y2;
      }
    }
    src_macro.setHeight(a_y2 - ly);
    
  } else {
    // Change the height based on closest bottom edge
    float c_y2 = ly;
    for (const auto& macro : macros_) {
      const float cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 < uy && cur_y2 > c_y2) {
        c_y2 = cur_y2;
      }
    }
    if (c_y2 <= ly) {
      return;
    }
    src_macro.setHeight(c_y2 - ly);
  }
}

void SACore::calSegmentLoc(float seg_start,
                          float seg_end,
                          int& start_id,
                          int& end_id,
                          const std::vector<float>& grid) {
  start_id = -1;
  end_id = -1;
  
  // Find the grid cell containing the segment start
  for (size_t i = 0; i < grid.size() - 1; i++) {
    if ((grid[i] <= seg_start) && (grid[i + 1] > seg_start)) {
      start_id = static_cast<int>(i);
    }
    if ((grid[i] <= seg_end) && (grid[i + 1] > seg_end)) {
      end_id = static_cast<int>(i);
    }
  }
  
  // Handle edge case where segment end is beyond the last grid point
  if (end_id == -1) {
    end_id = static_cast<int>(grid.size() - 1);
  }
}

void SACore::expandClusters() {
  // Step 1: Divide the entire floorplan into grids by collecting unique coordinate points
  std::set<float> x_point;
  std::set<float> y_point;
  
  for (auto& macro_id : pos_seq_) {
    x_point.insert(macros_[macro_id].getX());
    x_point.insert(macros_[macro_id].getX() + macros_[macro_id].getWidth());
    y_point.insert(macros_[macro_id].getY());
    y_point.insert(macros_[macro_id].getY() + macros_[macro_id].getHeight());
  }
  
  // Convert sets to vectors for grid representation
  std::vector<float> x_grid(x_point.begin(), x_point.end());
  std::vector<float> y_grid(y_point.begin(), y_point.end());
  
  // Create grid in a row-based manner to track macro occupancy
  const int num_x = static_cast<int>(x_grid.size()) - 1;
  const int num_y = static_cast<int>(y_grid.size()) - 1;
  std::vector<std::vector<int>> grids(num_y, std::vector<int>(num_x, -1));

  // Find the macro with lowest net violation to prioritize expansion
  std::vector<float> netViolationVec(macros_.size(), 0.0f);
  std::vector<float> netExpandVec;
  netExpandVec.reserve(nets_.size());
  
  // Calculate net violations for all nets
  for (const auto& net : nets_) {
    const float penalty = calNetViolation(&net);
    netExpandVec.push_back(penalty / net.weight);    
    netViolationVec[net.terminals.first] += penalty;
    netViolationVec[net.terminals.second] += penalty;
  }
  
  // Find the macro with minimum net violation
  auto min_element_iter = std::min_element(netViolationVec.begin(), netViolationVec.end());
  int src_macro = static_cast<int>(min_element_iter - netViolationVec.begin());

  // Calculate maximum expansion in each direction based on connected nets
  float left_expand_max = 0.0f;
  float right_expand_max = 0.0f;
  float top_expand_max = 0.0f;
  float down_expand_max = 0.0f;  

  // Source macro boundaries
  const float src_lx = macros_[src_macro].getX();
  const float src_ly = macros_[src_macro].getY();
  const float src_ux = src_lx + macros_[src_macro].getWidth();
  const float src_uy = src_ly + macros_[src_macro].getHeight();

  // Analyze all nets to determine expansion limits
  for (size_t i = 0; i < nets_.size(); i++) {
    if (nets_[i].terminals.first != src_macro && nets_[i].terminals.second != src_macro) {
      continue; // Skip nets not connected to source macro
    }
    
    // Get the other terminal's coordinates
    float sink_lx, sink_ly, sink_ux, sink_uy;
    
    if (nets_[i].terminals.first == src_macro) {
      const int sink_idx = nets_[i].terminals.second;
      sink_lx = macros_[sink_idx].getX();
      sink_ly = macros_[sink_idx].getY();
      sink_ux = sink_lx + macros_[sink_idx].getWidth();
      sink_uy = sink_ly + macros_[sink_idx].getHeight();
    } else {
      const int sink_idx = nets_[i].terminals.first;
      sink_lx = macros_[sink_idx].getX();
      sink_ly = macros_[sink_idx].getY();
      sink_ux = sink_lx + macros_[sink_idx].getWidth();
      sink_uy = sink_ly + macros_[sink_idx].getHeight();
    }

    // Determine expansion limits based on relative positions
    if (src_lx > sink_ux) {
      left_expand_max = std::max(left_expand_max, netExpandVec[i]);
    }

    if (src_ux < sink_lx) {
      right_expand_max = std::max(right_expand_max, netExpandVec[i]);
    }

    if (src_ly > sink_uy) {
      down_expand_max = std::max(down_expand_max, netExpandVec[i]);
    }

    if (src_uy < sink_ly) {
      top_expand_max = std::max(top_expand_max, netExpandVec[i]);
    }
  }

  // First mark the source macro in the grid
  for (int macro_id = src_macro; macro_id <= src_macro; macro_id++) {
    int x_start = 0, x_end = 0;
    calSegmentLoc(macros_[macro_id].getX(),
                  macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                  x_start, x_end, x_grid);
                  
    int y_start = 0, y_end = 0;
    calSegmentLoc(macros_[macro_id].getY(),
                  macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                  y_start, y_end, y_grid);
                  
    // Mark grid cells occupied by this macro
    for (int j = y_start; j < y_end; j++) {
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }
  }
  
  // Process macros in sequence order for expansion
  for (int order = 0; order <= 1; order++) {
    const std::vector<int>& macro_ids = (order == 0) ? pos_seq_ : neg_seq_;
    
    for (int macro_id : macro_ids) {
      // Find grid cells occupied by this macro
      int x_start = 0, x_end = 0;
      calSegmentLoc(macros_[macro_id].getX(),
                    macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                    x_start, x_end, x_grid);
                    
      int y_start = 0, y_end = 0;
      calSegmentLoc(macros_[macro_id].getY(),
                    macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                    y_start, y_end, y_grid);
                    
      int x_start_new = x_start;
      int x_end_new = x_end;
      int y_start_new = y_start;
      int y_end_new = y_end;
      
      // Try to expand left
      for (int i = x_start - 1; i >= 0; i--) {
        bool can_expand = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            can_expand = false;  // Cell already occupied
            break;
          }
        }
        
        if (!can_expand) {
          break;
        }
        
        // Expand left and mark cells
        x_start_new--;
        for (int j = y_start; j < y_end; j++) {
          grids[j][i] = macro_id;
        }
      }
      x_start = x_start_new;
      
      // Try to expand top
      for (int j = y_end; j < num_y; j++) {
        bool can_expand = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            can_expand = false;
            break;
          }
        }
        
        if (!can_expand) {
          break;
        }
        
        // Expand top and mark cells
        y_end_new++;
        for (int i = x_start; i < x_end; i++) {
          grids[j][i] = macro_id;
        }
      }
      y_end = y_end_new;
      
      // Try to expand right
      for (int i = x_end; i < num_x; i++) {
        bool can_expand = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            can_expand = false;
            break;
          }
        }
        
        if (!can_expand) {
          break;
        }
        
        // Expand right and mark cells
        x_end_new++;
        for (int j = y_start; j < y_end; j++) {
          grids[j][i] = macro_id;
        }
      }
      x_end = x_end_new;
      
      // Try to expand down
      for (int j = y_start - 1; j >= 0; j--) {
        bool can_expand = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            can_expand = false;
            break;
          }
        }
        
        if (!can_expand) {
          break;
        }
        
        // Expand down and mark cells
        y_start_new--;
        for (int i = x_start; i < x_end; i++) {
          grids[j][i] = macro_id;
        }
      }
      y_start = y_start_new;
      
      // Update the macro dimensions with expansion limits considered
      float left_start = std::max(x_grid[x_start], macros_[macro_id].getX() - left_expand_max);
      float down_start = std::max(y_grid[y_start], macros_[macro_id].getY() - down_expand_max);
      float right_end = std::min(x_grid[x_end], macros_[macro_id].getX() + macros_[macro_id].getWidth() + right_expand_max);
      float top_end = std::min(y_grid[y_end], macros_[macro_id].getY() + macros_[macro_id].getHeight() + top_expand_max);
      
      // Set new position and shape
      macros_[macro_id].setX(left_start);
      macros_[macro_id].setY(down_start);
      macros_[macro_id].setShape(right_end - macros_[macro_id].getX(),
                                 top_end - macros_[macro_id].getY());
    }
  }
}

float SACore::calNetViolation(const BundledNet* net) const {
  const int src = net->terminals.first;
  const int sink = net->terminals.second;
  
  // Calculate chip boundaries
  const float lx_a = macros_[src].getRealX();
  const float ly_a = macros_[src].getRealY();
  const float ux_a = lx_a + macros_[src].getRealWidth();
  const float uy_a = ly_a + macros_[src].getRealHeight();

  const float lx_b = macros_[sink].getRealX();
  const float ly_b = macros_[sink].getRealY();
  const float ux_b = lx_b + macros_[sink].getRealWidth();
  const float uy_b = ly_b + macros_[sink].getRealHeight();

  float length = 0.0f;
  
  // Calculate length based on overlap between terminals
  if (std::min(uy_a, uy_b) > std::max(ly_a, ly_b)) {
    // Vertical overlap case
    const float w = std::min(uy_a, uy_b) - std::max(ly_a, ly_b);
    const float h = std::max(lx_a, lx_b) - std::min(ux_a, ux_b);
    length = h + 2.0f * (std::sqrt(w * w + 2.0f * net->io_area) - w);
  } else if (std::min(ux_a, ux_b) > std::max(lx_a, lx_b)) {
    // Horizontal overlap case
    const float w = std::min(ux_a, ux_b) - std::max(lx_a, lx_b);
    const float h = std::max(ly_a, ly_b) - std::min(uy_a, uy_b);
    length = h + 2.0f * (std::sqrt(w * w + 2.0f * net->io_area) - w);    
  } else {
    // No overlap case - use center-to-center distance
    const float cx_a = (lx_a + ux_a) * 0.5f;
    const float cy_a = (ly_a + uy_a) * 0.5f;
    const float cx_b = (lx_b + ux_b) * 0.5f;
    const float cy_b = (ly_b + uy_b) * 0.5f;

    const float width = (ux_a - lx_a + ux_b - lx_b) * 0.5f;
    const float height = (uy_a - ly_a + uy_b - ly_b) * 0.5f;

    length = std::abs(cx_a - cx_b) + std::abs(cy_a - cy_b) - width - height;
    length += 2.0f * std::sqrt(2.0f * net->io_area);
  }

  // Calculate penalty as weighted violation
  return net->weight * std::max(0.0f, length - net->reach); 
}

float SACore::calNetPenalty() const {
  float net_penalty = 0.0f;
  
  // Accumulate penalties from all nets
  for (const auto& net : nets_) {
    net_penalty += calNetViolation(&net);
  }

  return net_penalty;
}

void SACore::checkViolation() {
  for (const auto& net : nets_) {
    const float penalty = calNetViolation(&net);
    
    if (penalty > 0.0f) {
      // Violation found - uncomment if detailed logging is needed
      /*
      std::cout << "Violation:  net.src = " << net.terminals.first << "  "
                << "net.sink = " << net.terminals.second << "  "
                << "net_length = " << penalty / net.weight << "  "
                << "reach = " << net.reach << "  "
                << std::endl;
      */
    }        
  }
}

void SACore::perturb() {
  if (macros_.empty()) {
    return;
  }

  // Backup current state
  pre_pos_seq_ = pos_seq_;
  pre_neg_seq_ = neg_seq_;
  pre_width_ = width_;
  pre_height_ = height_;
  pre_area_penalty_ = area_penalty_;
  pre_package_penalty_ = package_penalty_;
  pre_net_penalty_ = net_penalty_;
  
  // Determine action based on random selection and probability thresholds
  const float op = distribution_(generator_);
  const float action_prob_1 = pos_swap_prob_;
  const float action_prob_2 = action_prob_1 + neg_swap_prob_;
  const float action_prob_3 = action_prob_2 + double_swap_prob_;
  const float action_prob_4 = action_prob_3 + resize_prob_;
  
  if (op <= action_prob_1) {
    action_id_ = 1;
    singleSeqSwap(true);  // Swap in pos_seq_
  } else if (op <= action_prob_2) {
    action_id_ = 2;
    singleSeqSwap(false);  // Swap in neg_seq_
  } else if (op <= action_prob_3) {
    action_id_ = 3;
    doubleSeqSwap();  // Swap in both sequences
  } else if (op <= action_prob_4) {
    action_id_ = 4;
    pre_macros_ = macros_;
    resizeOneCluster();  // Resize a single cluster
  } else {
    action_id_ = 5;
    pre_macros_ = macros_;
    expandClusters();  // Expand clusters to fill deadspace
  }
  
  // Update macro locations based on sequence pair
  packFloorplan();
}

void SACore::restore() {
  if (macros_.empty()) {
    return;
  }

  // Restore previous state based on action type
  switch (action_id_) {
    case 1:
      pos_seq_ = pre_pos_seq_;
      break;
    case 2:
      neg_seq_ = pre_neg_seq_;
      break;
    case 3:
      pos_seq_ = pre_pos_seq_;
      neg_seq_ = pre_neg_seq_;
      break;
    case 4:
      macros_[macro_id_] = pre_macros_[macro_id_];
      break;
    case 5:
      macros_ = pre_macros_;
      break;
    default:
      break;
  }

  // Restore metrics
  width_ = pre_width_;
  height_ = pre_height_;
  area_penalty_ = pre_area_penalty_;
  package_penalty_ = pre_package_penalty_;
  net_penalty_ = pre_net_penalty_;
}

bool SACore::isValid() const {
  return (calNetPenalty() <= net_reach_penalty_acc_);
}

float SACore::calAreaPenalty() const {
  if (area_penalty_weight_ <= 0.0f) {
    return 0.0f;
  }

  float area_penalty = 0.0f;
  for (const auto& macro : macros_) {
    area_penalty += std::max(0.0f, macro.getArea() - macro.getMinArea());
  }

  return area_penalty; 
}

float SACore::calPackagePenalty() const {
  if (package_penalty_weight_ <= 0.0f) {
    return 0.0f;
  }

  return width_ * height_;
}

void SACore::calPenalty() {
  area_penalty_ = calAreaPenalty();
  package_penalty_ = calPackagePenalty();
  net_penalty_ = calNetPenalty();
}

void SACore::initialize() {
  // Initial floorplan calculation and penalty evaluation
  perturb();
  calPenalty();

  // Set normalization factors based on initial floorplan
  norm_area_penalty_ = width_ * height_;
  norm_package_penalty_ = width_ * height_;
  norm_net_penalty_ = width_ + height_;
}

float SACore::calNormCost() {
  calPenalty();
  
  // Calculate normalized weighted cost
  float norm_cost = 0.0f;
  
  if (norm_area_penalty_ > 0.0f) {
    norm_cost += area_penalty_weight_ * area_penalty_ / norm_area_penalty_;
  }
  
  if (norm_package_penalty_ > 0.0f) {
    norm_cost += package_penalty_weight_ * package_penalty_ / norm_package_penalty_;
  }
  
  if (norm_net_penalty_ > 0.0f) {
    norm_cost += net_penalty_weight_ * net_penalty_ / norm_net_penalty_;
  }
  
  return norm_cost;
}

void SACore::run(float cooling_acceleration_factor) {
  // Initialize floorplan
  packFloorplan();

  // Ensure normalization factors are positive
  if (norm_area_penalty_ <= 0.0f) {
    norm_area_penalty_ = 1.0f;
  }

  if (norm_package_penalty_ <= 0.0f) {
    norm_package_penalty_ = 1.0f;
  }

  if (norm_net_penalty_ <= 0.0f) {
    norm_net_penalty_ = 1.0f;
  }

  // Initialize SA parameters
  float cost = calNormCost();
  float pre_cost = cost;
  float temperature = init_temperature_;
  
  // Calculate temperature reduction factor
  const float t_factor = std::exp(std::log(min_temperature_ / init_temperature_) 
                                 / (max_num_step_ * cooling_acceleration_factor));
  
  // Start timing
  auto startTimeStamp = std::chrono::high_resolution_clock::now();
  
  // Main SA loop
  for (int step = 1; step <= max_num_step_; step++) {
    // Perform perturbations at current temperature
    for (int i = 0; i < num_perturb_per_step_; i++) {
      perturb();
      cost = calNormCost();
      const float delta_cost = cost - pre_cost;
      
      // Metropolis acceptance criterion
      const float random_val = distribution_(generator_);
      const float acceptance_prob = (delta_cost > 0.0f) 
                                  ? std::exp(-delta_cost / temperature) 
                                  : 1.0f;
                                  
      if (random_val < acceptance_prob) {
        pre_cost = cost; // Accept new solution
      } else {
        restore();       // Reject and restore
      }
    }
    
    // Cool down temperature
    temperature *= t_factor;
    
    // Record history
    cost_list_.push_back(pre_cost);
    T_list_.push_back(temperature);
    
    /* Uncomment for debug output
    if (step % 100 == 0) {
      std::cout << "Step: " << step 
                << "  Width: " << width_ 
                << "  Height: " << height_
                << "  Cost: " << pre_cost 
                << "  Temperature: " << temperature 
                << std::endl;
    }
    */
  }
  
  // Calculate runtime statistics
  auto endTimeStamp = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeStamp - startTimeStamp);
  
  /*  Uncomment for runtime reporting
  std::cout << "SA duration: " << duration.count() << " ms" << std::endl;
  std::cout << "Average runtime per perturbation: " 
            << duration.count() * 1.0 / (max_num_step_ * num_perturb_per_step_) 
            << " ms" << std::endl;
  */
  
  // Ensure final solution is properly evaluated
  packFloorplan();
  calPenalty();
}

} // namespace chiplet
