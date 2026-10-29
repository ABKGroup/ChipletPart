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
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <string>
#include <numeric>
#include <chrono>

// Class SACore
SACore::SACore(
    int worker_id,
    std::vector<Chiplet> chiplets,
    std::vector<BundledNet> nets,
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
    unsigned seed)
{
  worker_id_ = worker_id;
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
  temperature_ = init_temperature_;
  // generate random
  std::mt19937 rand_gen(seed);
  generator_ = rand_gen;
  std::uniform_real_distribution<float> distribution(0.0, 1.0);
  distribution_ = distribution;
  macros_.reserve(chiplets.size());
  for (unsigned int i = 0; i < chiplets.size(); i++) {
    pos_seq_.push_back(i);
    neg_seq_.push_back(i);
    pre_pos_seq_.push_back(i);
    pre_neg_seq_.push_back(i);
    macros_.push_back(chiplets[i]);
  }
  nets_ = nets;
}


void SACore::packFloorplan()
{
  for (auto& macro : macros_) {
    macro.setX(0.0);
    macro.setY(0.0);
  }

  // calculate X position
  // store the position of each macro in the pos_seq_ and neg_seq_
  std::vector<std::pair<int, int>> match(macros_.size());
  for (int i = 0; i < macros_.size(); i++) {
    match[pos_seq_[i]].first = i;
    match[neg_seq_[i]].second = i;
  }
  // Initialize current length
  std::vector<float> length(macros_.size(), 0.0);
  for (int i = 0; i < pos_seq_.size(); i++) {
    const int b = pos_seq_[i];  // macro_id
    // add the continue syntax to handle fixed terminals
    if (macros_[b].getWidth() <= 0 || macros_[b].getHeight() <= 0) {
      continue;
    }
    const int p = match[b].second;  // the position of current macro in neg_seq_
    macros_[b].setX(length[p]);
    const float t = macros_[b].getX() + macros_[b].getWidth();
    for (int j = p; j < neg_seq_.size(); j++) {
      if (t > length[j]) {
        length[j] = t;
      } else {
        break;
      }
    }
  }
  // update width_ of current floorplan
  width_ = length[macros_.size() - 1];

  // calulate Y position
  std::vector<int> pos_seq(pos_seq_.size());
  for (int i = 0; i < macros_.size(); i++) {
    pos_seq[i] = pos_seq_[macros_.size() - 1 - i];
  }
  // store the position of each macro in the pos_seq_ and neg_seq_
  for (int i = 0; i < macros_.size(); i++) {
    match[pos_seq[i]].first = i;
    match[neg_seq_[i]].second = i;
    length[i] = 0.0;  // initialize the length
  }
  for (int i = 0; i < pos_seq_.size(); i++) {
    const int b = pos_seq[i];  // macro_id
    // add continue syntax to handle fixed terminals
    if (macros_[b].getHeight() <= 0 || macros_[b].getWidth() <= 0.0) {
      continue;
    }
    const int p = match[b].second;  // the position of current macro in neg_seq_
    macros_[b].setY(length[p]);
    const float t = macros_[b].getY() + macros_[b].getHeight();
    for (int j = p; j < neg_seq_.size(); j++) {
      if (t > length[j]) {
        length[j] = t;
      } else {
        break;
      }
    }
  }
  // update width_ of current floorplan
  height_ = length[macros_.size() - 1];
}


// SingleSeqSwap
void SACore::singleSeqSwap(bool pos)
{
  if (macros_.size() <= 1) {
    return;
  }

  const int n = static_cast<int>(macros_.size());
  const int index1 = randIndex(n);
  int index2 = randIndex(n);
  while (index1 == index2) {
    index2 = randIndex(n);
  }

  if (pos) {
    std::swap(pos_seq_[index1], pos_seq_[index2]);
  } else {
    std::swap(neg_seq_[index1], neg_seq_[index2]);
  }
}

void SACore::doubleSeqSwap()
{
  if (macros_.size() <= 1) {
    return;
  }

  const int n = static_cast<int>(macros_.size());
  const int index1 = randIndex(n);
  int index2 = randIndex(n);
  while (index1 == index2) {
    index2 = randIndex(n);
  }

  std::swap(pos_seq_[index1], pos_seq_[index2]);
  std::swap(neg_seq_[index1], neg_seq_[index2]);
}


void SACore::resizeOneCluster()
{
  if (macros_.empty()) {
    return;
  }
  const int idx = randIndex(static_cast<int>(macros_.size()));
  macro_id_ = idx;
  Chiplet& src_macro = macros_[idx];

  const float lx = src_macro.getX();
  const float ly = src_macro.getY();
  const float ux = lx + src_macro.getWidth();
  const float uy = ly + src_macro.getHeight();

  if (distribution_(generator_) < 0.2) {
    // Draw a random aspect ratio inside the chiplet's allowed range
    // [min_ar_, max_ar_]. (The previous min/max pair was inverted and always
    // produced 5.0.) Log-uniform so that 0.5 and 2.0 are equally likely.
    const float lo = std::log(src_macro.min_ar_);
    const float hi = std::log(src_macro.max_ar_);
    const float aspect_ratio =
        std::exp(lo + distribution_(generator_) * (hi - lo));
    src_macro.resizeRandomly(aspect_ratio);
    return;
  }


  const float option = distribution_(generator_);
  if (option <= 0.25) {
    // Change the width of soft block to Rb = e.x2 - b.x1
    float e_x2 = width_;
    for (const auto& macro : macros_) {
      const float cur_x2 = macro.getX() + macro.getWidth();
      if (cur_x2 > ux && cur_x2 < e_x2) {
        e_x2 = cur_x2;
      }
    }
    src_macro.setWidth(e_x2 - lx);
  } else if (option <= 0.5) {
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
  } else if (option <= 0.75) {
    // change the height of soft block to Tb = a.y2 - b.y1
    float a_y2 = height_;
    for (const auto& macro : macros_) {
      const float cur_y2 = macro.getY() + macro.getHeight();
      if (cur_y2 > uy && cur_y2 < a_y2) {
        a_y2 = cur_y2;
      }
    }
    src_macro.setHeight(a_y2 - ly);
  } else {
    // Change the height of soft block to Bb = c.y2 - b.y1
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


// A utility function for FillDeadSpace.
// It's used for calculate the start point and end point for a segment in a grid
void SACore::calSegmentLoc(float seg_start,
                           float seg_end,
                           int& start_id,
                           int& end_id,
                           std::vector<float>& grid)
{
  start_id = -1;
  end_id = -1;
  if (grid.size() < 2) {
    start_id = 0;
    end_id = 0;
    return;
  }
  for (int i = 0; i < static_cast<int>(grid.size()) - 1; i++) {
    if ((grid[i] <= seg_start) && (grid[i + 1] > seg_start)) {
      start_id = i;
    }
    if ((grid[i] <= seg_end) && (grid[i + 1] > seg_end)) {
      end_id = i;
    }
  }
  if (end_id == -1) {
    end_id = grid.size() - 1;
  }
}




void SACore::expandClusters()
{
  // Expand the chiplet with the largest reach violation into the dead space
  // around it, growing only towards the sinks it violates and by at most the
  // excess wirelength of those nets. Growing a chiplet shortens the gap to
  // its neighbours (reach) at the price of area, which the area/package
  // penalties and the SA acceptance test trade off.
  if (macros_.empty() || nets_.empty()) {
    return;
  }

  // Step 1: per-net excess length and per-macro accumulated violation.
  std::vector<float> netViolationVec(macros_.size(), 0.0);
  std::vector<float> netExpandVec(nets_.size(), 0.0);
  float total_violation = 0.0f;
  for (size_t n = 0; n < nets_.size(); ++n) {
    const BundledNet& net = nets_[n];
    const float penalty = calNetViolation(&net);
    netExpandVec[n] = (net.weight > 0.0f) ? penalty / net.weight : 0.0f;
    netViolationVec[net.terminals.first] += penalty;
    netViolationVec[net.terminals.second] += penalty;
    total_violation += penalty;
  }
  if (total_violation <= 0.0f) {
    return;  // every reach is met; expansion could only add area
  }
  const int src_macro = static_cast<int>(
      std::max_element(netViolationVec.begin(), netViolationVec.end())
      - netViolationVec.begin());

  // Step 2: how far may src_macro grow in each direction?
  float left_expand_max = 0.0;
  float right_expand_max = 0.0;
  float top_expand_max = 0.0;
  float down_expand_max = 0.0;

  const float src_lx = macros_[src_macro].getX();
  const float src_ly = macros_[src_macro].getY();
  const float src_ux = src_lx + macros_[src_macro].getWidth();
  const float src_uy = src_ly + macros_[src_macro].getHeight();

  for (size_t i = 0; i < nets_.size(); i++) {
    if (nets_[i].terminals.first != src_macro && nets_[i].terminals.second != src_macro) {
      continue;
    }
    if (netExpandVec[i] <= 0.0f) {
      continue;
    }
    const int sink = (nets_[i].terminals.first == src_macro)
                         ? nets_[i].terminals.second
                         : nets_[i].terminals.first;
    const float sink_lx = macros_[sink].getX();
    const float sink_ly = macros_[sink].getY();
    const float sink_ux = sink_lx + macros_[sink].getWidth();
    const float sink_uy = sink_ly + macros_[sink].getHeight();

    // Only growth towards +x / +y is meaningful: the sequence-pair packing
    // anchors every macro at its lower-left corner, so a macro enlarged to
    // the left or bottom would simply be re-packed and grow to the right/top
    // instead, pushing the wrong neighbours away.
    (void)sink_ux;
    (void)sink_uy;
    if (src_ux < sink_lx) {
      right_expand_max = std::max(right_expand_max, netExpandVec[i]);
    }
    if (src_uy < sink_ly) {
      top_expand_max = std::max(top_expand_max, netExpandVec[i]);
    }
  }
  if (left_expand_max <= 0.0f && right_expand_max <= 0.0f &&
      top_expand_max <= 0.0f && down_expand_max <= 0.0f) {
    return;
  }

  // Step 3: grid of all macro boundaries, with EVERY macro marked, so that a
  // free cell really is dead space.
  std::set<float> x_point;
  std::set<float> y_point;
  for (const auto& macro : macros_) {
    x_point.insert(macro.getX());
    x_point.insert(macro.getX() + macro.getWidth());
    y_point.insert(macro.getY());
    y_point.insert(macro.getY() + macro.getHeight());
  }
  std::vector<float> x_grid(x_point.begin(), x_point.end());
  std::vector<float> y_grid(y_point.begin(), y_point.end());
  const int num_x = static_cast<int>(x_grid.size()) - 1;
  const int num_y = static_cast<int>(y_grid.size()) - 1;
  if (num_x <= 0 || num_y <= 0) {
    return;  // degenerate floorplan (all macros zero-sized)
  }
  std::vector<std::vector<int>> grids(num_y, std::vector<int>(num_x, -1));

  auto macro_cells = [&](int macro_id, int& x_start, int& x_end, int& y_start, int& y_end) {
    calSegmentLoc(macros_[macro_id].getX(),
                  macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                  x_start, x_end, x_grid);
    calSegmentLoc(macros_[macro_id].getY(),
                  macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                  y_start, y_end, y_grid);
    x_start = std::max(0, x_start);
    y_start = std::max(0, y_start);
    x_end = std::min(num_x, x_end);
    y_end = std::min(num_y, y_end);
  };

  for (int macro_id = 0; macro_id < static_cast<int>(macros_.size()); ++macro_id) {
    if (macros_[macro_id].getWidth() <= 0 || macros_[macro_id].getHeight() <= 0) {
      continue;
    }
    int x_start, x_end, y_start, y_end;
    macro_cells(macro_id, x_start, x_end, y_start, y_end);
    for (int j = y_start; j < y_end; j++) {
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }
  }

  // Step 4: propagate src_macro into adjacent free cells (left, top, right,
  // down), each direction only as far as its expand budget allows.
  int x_start, x_end, y_start, y_end;
  macro_cells(src_macro, x_start, x_end, y_start, y_end);
  if (x_start >= x_end || y_start >= y_end) {
    return;
  }

  auto row_free = [&](int j, int i0, int i1) {
    for (int i = i0; i < i1; i++) {
      if (grids[j][i] != -1) return false;
    }
    return true;
  };
  auto col_free = [&](int i, int j0, int j1) {
    for (int j = j0; j < j1; j++) {
      if (grids[j][i] != -1) return false;
    }
    return true;
  };

  const float min_x = src_lx - left_expand_max;
  const float max_x = src_ux + right_expand_max;
  const float min_y = src_ly - down_expand_max;
  const float max_y = src_uy + top_expand_max;

  // left
  for (int i = x_start - 1; i >= 0 && left_expand_max > 0.0f; i--) {
    if (x_grid[i] < min_x || !col_free(i, y_start, y_end)) break;
    x_start = i;
  }
  // top
  for (int j = y_end; j < num_y && top_expand_max > 0.0f; j++) {
    if (y_grid[j + 1] > max_y || !row_free(j, x_start, x_end)) break;
    y_end = j + 1;
  }
  // right
  for (int i = x_end; i < num_x && right_expand_max > 0.0f; i++) {
    if (x_grid[i + 1] > max_x || !col_free(i, y_start, y_end)) break;
    x_end = i + 1;
  }
  // down
  for (int j = y_start - 1; j >= 0 && down_expand_max > 0.0f; j--) {
    if (y_grid[j] < min_y || !row_free(j, x_start, x_end)) break;
    y_start = j;
  }

  const float new_lx = x_grid[x_start];
  const float new_ly = y_grid[y_start];
  const float new_ux = x_grid[x_end];
  const float new_uy = y_grid[y_end];
  if (new_ux - new_lx <= 0.0f || new_uy - new_ly <= 0.0f) {
    return;
  }
  macro_id_ = src_macro;
  macros_[src_macro].setX(new_lx);
  macros_[src_macro].setY(new_ly);
  macros_[src_macro].setShape(new_ux - new_lx, new_uy - new_ly);
}

/*
void SACore::expandClusters()
{
  // Step1 : Divide the entire floorplan into grids
  std::set<float> x_point;
  std::set<float> y_point;
  for (auto& macro_id : pos_seq_) {
    x_point.insert(macros_[macro_id].getX());
    x_point.insert(macros_[macro_id].getX() + macros_[macro_id].getWidth());
    y_point.insert(macros_[macro_id].getY());
    y_point.insert(macros_[macro_id].getY() + macros_[macro_id].getHeight());
  }
  // create grid
  std::vector<float> x_grid(x_point.begin(), x_point.end());
  std::vector<float> y_grid(y_point.begin(), y_point.end());
  // create grid in a row-based manner
  std::vector<std::vector<int>> grids;  // store the macro id
  const int num_x = x_grid.size() - 1;
  const int num_y = y_grid.size() - 1;
  for (int j = 0; j < num_y; j++) {
    std::vector<int> macro_ids(num_x, -1);
    grids.push_back(macro_ids);
  }

  for (int macro_id = 0; macro_id < pos_seq_.size(); macro_id++) {
    int x_start = 0;
    int x_end = 0;
    calSegmentLoc(macros_[macro_id].getX(),
                  macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                  x_start,
                  x_end,
                  x_grid);
    int y_start = 0;
    int y_end = 0;
    calSegmentLoc(macros_[macro_id].getY(),
                  macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                  y_start,
                  y_end,
                  y_grid);
    for (int j = y_start; j < y_end; j++) {
      for (int i = x_start; i < x_end; i++) {
        grids[j][i] = macro_id;
      }
    }
  }
  for (int order = 0; order <= 1; order++) {
    std::vector<int> macro_ids;

    if (order == 0) {
      macro_ids = pos_seq_;
    } else {
      macro_ids = neg_seq_;
    }
    
    for (int macro_id : macro_ids) {
      int x_start = 0;
      int x_end = 0;
      calSegmentLoc(macros_[macro_id].getX(),
                    macros_[macro_id].getX() + macros_[macro_id].getWidth(),
                    x_start,
                    x_end,
                    x_grid);
      int y_start = 0;
      int y_end = 0;
      calSegmentLoc(macros_[macro_id].getY(),
                    macros_[macro_id].getY() + macros_[macro_id].getHeight(),
                    y_start,
                    y_end,
                    y_grid);
      int x_start_new = x_start;
      int x_end_new = x_end;
      int y_start_new = y_start;
      int y_end_new = y_end;
      // propagate left first
      for (int i = x_start - 1; i >= 0; i--) {
        bool flag = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }           // end if
        }             // end y
        if (!flag) {  // extension done
          break;
        }
        x_start_new--;  // extend left
        for (int j = y_start; j < y_end; j++) {
          grids[j][i] = macro_id;
        }
      }  // end left
      x_start = x_start_new;
      // propagate top second
      for (int j = y_end; j < num_y; j++) {
        bool flag = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }           // end if
        }             // end y
        if (!flag) {  // extension done
          break;
        }
        y_end_new++;  // extend top
        for (int i = x_start; i < x_end; i++) {
          grids[j][i] = macro_id;
        }
      }  // end top
      y_end = y_end_new;
      // propagate right third
      for (int i = x_end; i < num_x; i++) {
        bool flag = true;
        for (int j = y_start; j < y_end; j++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }           // end if
        }             // end y
        if (!flag) {  // extension done
          break;
        }
        x_end_new++;  // extend right
        for (int j = y_start; j < y_end; j++) {
          grids[j][i] = macro_id;
        }
      }  // end right
      x_end = x_end_new;
      // propagate down second
      for (int j = y_start - 1; j >= 0; j--) {
        bool flag = true;
        for (int i = x_start; i < x_end; i++) {
          if (grids[j][i] != -1) {
            flag = false;  // we cannot extend the current cluster
            break;
          }           // end if
        }             // end y
        if (!flag) {  // extension done
          break;
        }
        y_start_new--;  // extend down
        for (int i = x_start; i < x_end; i++) {
          grids[j][i] = macro_id;
        }
      }  // end down
      y_start = y_start_new;
      // update the location of cluster
      macros_[macro_id].setX(x_grid[x_start]);
      macros_[macro_id].setY(y_grid[y_start]);
      macros_[macro_id].setShape(x_grid[x_end] - x_grid[x_start],
                                 y_grid[y_end] - y_grid[y_start]);
    }
  }
}
*/


float SACore::calNetViolation(const BundledNet* net) const
{
  auto src = net->terminals.first;
  auto sink = net->terminals.second;
  
  const float lx_a = macros_[src].getRealX();
  const float ly_a = macros_[src].getRealY();
  const float ux_a = lx_a + macros_[src].getRealWidth();
  const float uy_a = ly_a + macros_[src].getRealHeight();

  const float lx_b = macros_[sink].getRealX();
  const float ly_b = macros_[sink].getRealY();
  const float ux_b = lx_b + macros_[sink].getRealWidth();
  const float uy_b = ly_b + macros_[sink].getRealHeight();

  float length = 0.0;
  // vertical overlap
  if(std::min(uy_a, uy_b) > std::max(ly_a, ly_b)) {
    float w = std::min(uy_a, uy_b) - std::max(ly_a, ly_b);
    float h = std::max(lx_a, lx_b) - std::min(ux_a, ux_b);
    length = h + 2 * (std::sqrt(w * w + 2 * net->io_area) - w);
  } else if (std::min(ux_a, ux_b) > std::max(lx_a, lx_b)) {
    // horizontal overlap
    float w = std::min(ux_a, ux_b) - std::max(lx_a, lx_b);
    float h = std::max(ly_a, ly_b) - std::min(uy_a, uy_b);
    length = h + 2 * (std::sqrt(w * w + 2 * net->io_area) - w);    
  } else {
    const float cx_a = (lx_a + ux_a) / 2.0;
    const float cy_a = (ly_a + uy_a) / 2.0;
    const float cx_b = (lx_b + ux_b) / 2.0;
    const float cy_b = (ly_b + uy_b) / 2.0;

    const float width = (ux_a - lx_a + ux_b - lx_b) / 2.0;
    const float height = (uy_a - ly_a + uy_b - ly_b) / 2.0;

    length = std::abs(cx_a - cx_b) + std::abs(cy_a - cy_b) - width - height;
    length += 2 * std::sqrt(2 * net->io_area);
  }

  const float penalty = net->weight * std::max(0.0f, length - net->reach); 
  return penalty;
}


// We use the maximum HPWL as the cost
float SACore::calNetPenalty() const
{
  float net_penalty = 0.0;
  
  /*
  for (const auto& net : nets_) {
    const float lx_a = macros_[net.terminals.first].getRealX();
    const float ly_a = macros_[net.terminals.first].getRealY();
    const float ux_a = lx_a + macros_[net.terminals.first].getRealWidth();
    const float uy_a = ly_a + macros_[net.terminals.first].getRealHeight();

    const float lx_b = macros_[net.terminals.second].getRealX();
    const float ly_b = macros_[net.terminals.second].getRealY();
    const float ux_b = lx_b + macros_[net.terminals.second].getRealWidth();
    const float uy_b = ly_b + macros_[net.terminals.second].getRealHeight();

    float HPWL = 0.0;
    // vertical overlap
    if(std::min(uy_a, uy_b) > std::max(ly_a, ly_b)) {
      HPWL = std::max(lx_a, lx_b) - std::min(ux_a, ux_b);
    } else if (std::min(ux_a, ux_b) > std::max(lx_a, lx_b)) {
      // horizontal overlap
      HPWL = std::max(ly_a, ly_b) - std::min(uy_a, uy_b);
    } else {
      const float cx_a = (lx_a + ux_a) / 2.0;
      const float cy_a = (ly_a + uy_a) / 2.0;
      const float cx_b = (lx_b + ux_b) / 2.0;
      const float cy_b = (ly_b + uy_b) / 2.0;

      const float width = (ux_a - lx_a + ux_b - lx_b) / 2.0;
      const float height = (uy_a - ly_a + uy_b - ly_b) / 2.0;

      HPWL = std::abs(cx_a - cx_b) + std::abs(cy_a - cy_b) - width - height;
    }

    const float penalty = net.weight * std::max(0.0f, HPWL - net.reach);
    net_penalty = std::max(net_penalty, penalty);
  }
  */

  for (const auto& net : nets_) {
    net_penalty += calNetViolation(&net);
  }

  return net_penalty;
}


bool SACore::checkViolation()
{
  for (const auto& net : nets_) {
    /*
    const float lx_a = macros_[net.terminals.first].getRealX();
    const float ly_a = macros_[net.terminals.first].getRealY();
    const float ux_a = lx_a + macros_[net.terminals.first].getRealWidth();
    const float uy_a = ly_a + macros_[net.terminals.first].getRealHeight();

    const float lx_b = macros_[net.terminals.second].getRealX();
    const float ly_b = macros_[net.terminals.second].getRealY();
    const float ux_b = lx_b + macros_[net.terminals.second].getRealWidth();
    const float uy_b = ly_b + macros_[net.terminals.second].getRealHeight();

    float HPWL = 0.0;
    // vertical overlap
    if(std::min(uy_a, uy_b) > std::max(ly_a, ly_b)) {
      HPWL = std::max(lx_a, lx_b) - std::min(ux_a, ux_b);
    } else if (std::min(ux_a, ux_b) > std::max(lx_a, lx_b)) {
      // horizontal overlap
      HPWL = std::max(ly_a, ly_b) - std::min(uy_a, uy_b);
    } else {
      const float cx_a = (lx_a + ux_a) / 2.0;
      const float cy_a = (ly_a + uy_a) / 2.0;
      const float cx_b = (lx_b + ux_b) / 2.0;
      const float cy_b = (ly_b + uy_b) / 2.0;

      const float width = (ux_a - lx_a + ux_b - lx_b) / 2.0;
      const float height = (uy_a - ly_a + uy_b - ly_b) / 2.0;

      HPWL = std::abs(cx_a - cx_b) + std::abs(cy_a - cy_b) - width - height;
    }

    const float penalty = net.weight * std::max(0.0f, HPWL - net.reach);
    */

    const float penalty = calNetViolation(&net);
    
    if (penalty > 0.0) {
      std::cout << "Violation:  net.src = " << net.terminals.first << "  "
                << "net.sink = " << net.terminals.second << "  "
                << "excess_length = " << (net.weight > 0.0f ? penalty / net.weight : penalty) << "  "
                << "reach = " << net.reach << "  "
                << std::endl;
      return true;
    }  
  }

  return false;
}


int SACore::reportViolations(std::ostream& os, int max_lines) const
{
  int num_violations = 0;
  float worst_excess = 0.0f;
  for (const auto& net : nets_) {
    const float penalty = calNetViolation(&net);
    if (penalty <= 0.0f) continue;
    const float excess = (net.weight > 0.0f) ? penalty / net.weight : penalty;
    worst_excess = std::max(worst_excess, excess);
    if (num_violations < max_lines) {
      const Chiplet& a = macros_[net.terminals.first];
      const Chiplet& b = macros_[net.terminals.second];
      os << "  net " << net.terminals.first << "->" << net.terminals.second
         << " reach=" << net.reach << " excess=" << excess
         << " io_area=" << net.io_area
         << " | A: " << a.getRealWidth() << "x" << a.getRealHeight()
         << " @(" << a.getRealX() << "," << a.getRealY() << ")"
         << " | B: " << b.getRealWidth() << "x" << b.getRealHeight()
         << " @(" << b.getRealX() << "," << b.getRealY() << ")\n";
    }
    ++num_violations;
  }
  if (num_violations > 0) {
    os << "  " << num_violations << "/" << nets_.size()
       << " nets violate reach; worst excess = " << worst_excess
       << "; package = " << width_ << "x" << height_ << "\n";
  }
  return num_violations;
}

void SACore::perturb()
{
  if (macros_.empty()) {
    return;
  }

  // Keep back up
  pre_pos_seq_ = pos_seq_;
  pre_neg_seq_ = neg_seq_;
  pre_width_ = width_;
  pre_height_ = height_;
  pre_area_penalty_ = area_penalty_;
  pre_package_penalty_ = package_penalty_;
  pre_net_penalty_ = net_penalty_;
  
  // generate random number (0 - 1) to determine actions
  const float op = distribution_(generator_);
  const float action_prob_1 = pos_swap_prob_;
  const float action_prob_2 = action_prob_1 + neg_swap_prob_;
  const float action_prob_3 = action_prob_2 + double_swap_prob_;
  const float action_prob_4 = action_prob_3 + resize_prob_;
  if (op <= action_prob_1) {
    action_id_ = 1;
    singleSeqSwap(true);  // Swap two macros in pos_seq_
  } else if (op <= action_prob_2) {
    action_id_ = 2;
    singleSeqSwap(false);  // Swap two macros in neg_seq_;
  } else if (op <= action_prob_3) {
    action_id_ = 3;
    doubleSeqSwap();  // Swap two macros in pos_seq_ and
                      // other two macros in neg_seq_
  } else if (op <= action_prob_4) {
    action_id_ = 4;
    pre_macros_ = macros_;
    resizeOneCluster();  // resize one cluster
  } else {
    action_id_ = 5;
    pre_macros_ = macros_;
    expandClusters();  // expand clusters to fill all the deadspace
  }
  // update the macro locations based on Sequence Pair
  packFloorplan();
}


void SACore::restore()
{
  if (macros_.empty()) {
    return;
  }

  // To reduce the runtime, here we do not call PackFloorplan
  // again. So when we need to generate the final floorplan out,
  // we need to call PackFloorplan again at the end of SA process
  if (action_id_ == 1) {
    pos_seq_ = pre_pos_seq_;
  } else if (action_id_ == 2) {
    neg_seq_ = pre_neg_seq_;
  } else if (action_id_ == 3) {
    pos_seq_ = pre_pos_seq_;
    neg_seq_ = pre_neg_seq_;
  } else if (action_id_ == 4) {
    macros_[macro_id_] = pre_macros_[macro_id_];
  } else {
    macros_ = pre_macros_;
  } 

  width_ = pre_width_;
  height_ = pre_height_;
  area_penalty_ = pre_area_penalty_;
  package_penalty_ = pre_package_penalty_;
  net_penalty_ = pre_net_penalty_;
}


bool SACore::isValid() const
{
  return (calNetPenalty() <= net_reach_penalty_acc_);
}

float SACore::getPackageSize() const
{
  return width_ * height_;
}


void SACore::getMacros(std::vector<Chiplet>& macros) const
{
  macros = macros_;
}

float SACore::calAreaPenalty() const
{
  float area_penalty = 0.0;
  if (area_penalty_weight_ <= 0.0) {
    return 0.0;
  }

  for (const auto& macro : macros_) {
    area_penalty += std::max(0.0f, macro.getArea() - macro.getMinArea());
  }

  return area_penalty; 
}


float SACore::calPackagePenalty() const
{
  float package_penalty = 0.0;
  if (package_penalty_weight_ <= 0.0) {
    return 0.0;
  }

  package_penalty = width_ * height_;
  return package_penalty;
}


void SACore::calPenalty()
{
  area_penalty_ =  calAreaPenalty();
  package_penalty_ =  calPackagePenalty();
  net_penalty_ = calNetPenalty();
}


void SACore::initialize()
{
  std::vector<float> area_penalty_list;
  std::vector<float> package_penalty_list;
  std::vector<float> net_penalty_list;

  perturb();
  calPenalty();

  norm_area_penalty_ = width_ * height_;
  norm_package_penalty_ = width_ * height_;
  norm_net_penalty_ = width_ + height_;

  /*
  for (int i = 0; i < 10; i++) {
    perturb();
    calPenalty();
    std::cout << "perturb step = " << i << "  "
              << "width = " << width_ << "  "
              << "height = " << height_ << "  "
              << std::endl;
    area_penalty_list.push_back(area_penalty_);
    package_penalty_list.push_back(package_penalty_);
    net_penalty_list.push_back(net_penalty_);
  }
  
  exit(1);
  
  norm_area_penalty_ = std::accumulate(area_penalty_list.begin(), area_penalty_list.end(), 0.0);
  norm_package_penalty_ = std::accumulate(package_penalty_list.begin(), package_penalty_list.end(), 0.0);
  norm_net_penalty_ = std::accumulate(net_penalty_list.begin(), net_penalty_list.end(), 0.0);
  norm_area_penalty_ /= area_penalty_list.size();
  norm_package_penalty_ /= package_penalty_list.size();
  norm_net_penalty_ /= net_penalty_list.size();
  */
}



float SACore::calNormCost()
{
  float norm_cost = 0.0;
  calPenalty();
  norm_cost += area_penalty_weight_ * area_penalty_ / norm_area_penalty_;
  norm_cost += package_penalty_weight_ * package_penalty_ / norm_package_penalty_;
  norm_cost += net_penalty_weight_ * net_penalty_ / norm_net_penalty_;
  return norm_cost;
}


void SACore::run()
{
  runSteps(max_num_step_);
}

void SACore::evaluateCurrent()
{
  packFloorplan();
  calPenalty();
  if (norm_area_penalty_ <= 0.0) norm_area_penalty_ = std::max(1e-6f, width_ * height_);
  if (norm_package_penalty_ <= 0.0) norm_package_penalty_ = std::max(1e-6f, width_ * height_);
  if (norm_net_penalty_ <= 0.0) norm_net_penalty_ = std::max(1e-6f, width_ + height_);
  trackViolation();
}

void SACore::trackViolation()
{
  if (net_penalty_ < min_net_penalty_seen_) {
    min_net_penalty_seen_ = net_penalty_;
    min_pos_seq_ = pos_seq_;
    min_neg_seq_ = neg_seq_;
    min_macros_ = macros_;
  }
  if (net_penalty_ <= net_reach_penalty_acc_) {
    seen_valid_ = true;
    valid_pos_seq_ = pos_seq_;
    valid_neg_seq_ = neg_seq_;
    valid_macros_ = macros_;
  }
}

static void fillSeedShapes(FloorplanSeed& seed, const std::vector<Chiplet>& macros)
{
  seed.aspect_ratios.clear();
  seed.widths.clear();
  seed.heights.clear();
  seed.aspect_ratios.reserve(macros.size());
  seed.widths.reserve(macros.size());
  seed.heights.reserve(macros.size());
  for (const auto& m : macros) {
    seed.aspect_ratios.push_back(m.getRealWidth() > 0.0f
                                     ? m.getRealHeight() / m.getRealWidth()
                                     : 1.0f);
    seed.widths.push_back(m.getRealWidth());
    seed.heights.push_back(m.getRealHeight());
  }
}

void SACore::getMinSeed(FloorplanSeed& seed) const
{
  seed.pos_seq = min_pos_seq_;
  seed.neg_seq = min_neg_seq_;
  fillSeedShapes(seed, min_macros_);
}

void SACore::getSeed(FloorplanSeed& seed) const
{
  seed.pos_seq = pos_seq_;
  seed.neg_seq = neg_seq_;
  fillSeedShapes(seed, macros_);
}

void SACore::applySeed(const FloorplanSeed& seed, bool exact_shapes)
{
  if (seed.pos_seq.size() != macros_.size() || seed.neg_seq.size() != macros_.size()) {
    return;
  }
  pos_seq_ = seed.pos_seq;
  neg_seq_ = seed.neg_seq;
  const bool exact = exact_shapes &&
                     seed.widths.size() == macros_.size() &&
                     seed.heights.size() == macros_.size();
  for (size_t i = 0; i < macros_.size(); ++i) {
    Chiplet& m = macros_[i];
    if (exact && seed.widths[i] > 0.0f && seed.heights[i] > 0.0f &&
        seed.widths[i] * seed.heights[i] >= m.getMinArea() * (1.0f - 1e-4f)) {
      m.width = seed.widths[i];
      m.height = seed.heights[i];
    } else if (seed.aspect_ratios.size() == macros_.size()) {
      const float ar = std::clamp(seed.aspect_ratios[i], m.min_ar_, m.max_ar_);
      m.resizeRandomly(ar);
    }
  }
}

void SACore::restoreMin()
{
  if (min_pos_seq_.empty()) {
    return;
  }
  pos_seq_ = min_pos_seq_;
  neg_seq_ = min_neg_seq_;
  macros_ = min_macros_;
  packFloorplan();
  calPenalty();
}

void SACore::quench(int num_perturbs)
{
  if (macros_.size() <= 1) {
    return;
  }
  packFloorplan();
  calPenalty();
  trackViolation();
  for (int i = 0; i < num_perturbs && net_penalty_ > net_reach_penalty_acc_; ++i) {
    perturb();
    calPenalty();
    if (net_penalty_ <= pre_net_penalty_) {
      trackViolation();
    } else {
      restore();
    }
  }
}

void SACore::getAspectRatios(std::vector<float>& ratios) const
{
  ratios.clear();
  ratios.reserve(macros_.size());
  for (const auto& m : macros_) {
    ratios.push_back(m.getRealHeight() > 0.0f
                         ? m.getRealWidth() / m.getRealHeight()
                         : 1.0f);
  }
}

float SACore::refineShapes(const std::function<float(const std::vector<float>&)>& cost_fn,
                           const std::vector<float>& candidates,
                           int max_rounds,
                           int* evals)
{
  packFloorplan();
  calPenalty();
  std::vector<float> ratios;
  getAspectRatios(ratios);
  if (macros_.empty() || net_penalty_ > net_reach_penalty_acc_) {
    return cost_fn(ratios);
  }
  float best_cost = cost_fn(ratios);
  if (evals) ++*evals;

  const int n = static_cast<int>(macros_.size());
  for (int round = 0; round < max_rounds; ++round) {
    bool improved = false;
    for (int i = 0; i < n; ++i) {
      Chiplet& m = macros_[i];
      // Candidates are width/height; the chiplet's range is height/width.
      const float lo = 1.0f / m.max_ar_;
      const float hi = 1.0f / m.min_ar_;
      const std::vector<Chiplet> saved = macros_;
      const float saved_ratio = ratios[i];
      std::vector<Chiplet> best_macros = macros_;
      float best_ratio = saved_ratio;
      bool found = false;
      for (float c : candidates) {
        const float r = std::clamp(c, lo, hi);
        if (std::fabs(r - saved_ratio) <= 1e-4f * saved_ratio) continue;
        macros_ = saved;
        macros_[i].resizeRandomly(1.0f / r);  // argument is height/width
        packFloorplan();
        calPenalty();
        if (net_penalty_ > net_reach_penalty_acc_) continue;
        ratios[i] = r;
        const float cost = cost_fn(ratios);
        if (evals) ++*evals;
        if (cost < best_cost - 1e-6f) {
          best_cost = cost;
          best_ratio = r;
          best_macros = macros_;
          found = true;
        }
      }
      macros_ = found ? best_macros : saved;
      ratios[i] = found ? best_ratio : saved_ratio;
      improved = improved || found;
    }
    if (!improved) break;
  }
  packFloorplan();
  calPenalty();
  trackViolation();
  return best_cost;
}

bool SACore::restoreValid()
{
  if (!seen_valid_) {
    return false;
  }
  if (isValid()) {
    return true;
  }
  pos_seq_ = valid_pos_seq_;
  neg_seq_ = valid_neg_seq_;
  macros_ = valid_macros_;
  packFloorplan();
  calPenalty();
  return isValid();
}

void SACore::runSteps(int steps)
{
  packFloorplan();

  if (norm_area_penalty_ <= 0.0) {
    norm_area_penalty_ = 1.0;
  }

  if (norm_package_penalty_ <= 0.0) {
    norm_package_penalty_ = 1.0;
  }

  if (norm_net_penalty_ <= 0.0) {
    norm_net_penalty_ = 1.0;
  }

  // record the previous status
  float cost = calNormCost();
  float pre_cost = cost;
  float delta_cost = 0.0;

  // The schedule continues from the current temperature (so successive
  // runSteps() calls form one annealing run) and never drops below
  // min_temperature_, which keeps exp(-delta / T) finite.
  for (int step = 0; step < steps; ++step) {
    const float T = std::max(temperature_, min_temperature_);
    for (int i = 0; i < num_perturb_per_step_; i++) {
      perturb();
      cost = calNormCost();
      delta_cost = cost - pre_cost;
      const float num = distribution_(generator_);
      const float prob
          = (delta_cost > 0.0) ? std::exp(-delta_cost / T) : 1.0f;
      if (num < prob) {
        pre_cost = cost;
        trackViolation();
      } else {
        restore();
      }
    }
    temperature_ = std::max(temperature_ * cooling_rate_, min_temperature_);
    cost_list_.push_back(pre_cost);
    T_list_.push_back(temperature_);
  }

  // update the final results
  packFloorplan();
  calPenalty();
}
