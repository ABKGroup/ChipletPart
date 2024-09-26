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

#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <vector>

namespace chiplet {

// Class SACore
SACore::SACore(std::vector<Chiplet> chiplets,
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
  // generate random
  std::mt19937 rand_gen(seed);
  generator_ = rand_gen;
  std::uniform_real_distribution<float> distribution(0.0, 1.0);
  distribution_ = distribution;
  macros_.reserve(chiplets.size());
  if (pos_seq.empty()) {
    for (unsigned int i = 0; i < chiplets.size(); i++) {
      pos_seq_.push_back(i);
      neg_seq_.push_back(i);
      pre_pos_seq_.push_back(i);
      pre_neg_seq_.push_back(i);
      macros_.push_back(chiplets[i]);
    }
  } else {
    pos_seq_ = pos_seq;
    neg_seq_ = neg_seq;
    pre_pos_seq_ = pos_seq;
    pre_neg_seq_ = neg_seq;
    for (unsigned int i = 0; i < chiplets.size(); i++) {
      macros_.push_back(chiplets[i]);
    }
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

  const int index1
      = (int) (std::floor(distribution_(generator_) * macros_.size()));
  int index2 = (int) (std::floor(distribution_(generator_) * macros_.size()));
  while (index1 == index2) {
    index2 = (int) (std::floor(distribution_(generator_) * macros_.size()));
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

  const int index1
      = (int) (std::floor(distribution_(generator_) * macros_.size()));
  int index2 = (int) (std::floor(distribution_(generator_) * macros_.size()));
  while (index1 == index2) {
    index2 = (int) (std::floor(distribution_(generator_) * macros_.size()));
  }

  std::swap(pos_seq_[index1], pos_seq_[index2]);
  std::swap(neg_seq_[index1], neg_seq_[index2]);
}

void SACore::resizeOneCluster()
{
  const int idx = static_cast<int>(
      std::floor(distribution_(generator_) * pos_seq_.size()));
  macro_id_ = idx;
  Chiplet& src_macro = macros_[idx];

  const float lx = src_macro.getX();
  const float ly = src_macro.getY();
  const float ux = lx + src_macro.getWidth();
  const float uy = ly + src_macro.getHeight();

  if (distribution_(generator_) < 0.2) {
    const float aspect_ratio = distribution_(generator_);
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
  for (int i = 0; i < grid.size() - 1; i++) {
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

// We use the maximum HPWL as the cost
float SACore::calNetPenalty() const
{
  float net_penalty = 0.0;
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
    if (std::min(uy_a, uy_b) > std::max(ly_a, ly_b)) {
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

  return net_penalty;
}

void SACore::checkViolation()
{
  int num_violations = 0;
  float avg_violation = 0.0;
  float total_penalty = 0.0;

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
    if (std::min(uy_a, uy_b) > std::max(ly_a, ly_b)) {
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
    if (penalty > 0.0) {
      num_violations++;
      avg_violation += HPWL - net.reach;
      total_penalty += penalty;
      /*std::cout << "Violation:  net.src = " << net.terminals.first << "  "
                << "net.sink = " << net.terminals.second << "  "
                << "net_length = " << HPWL << "  "
                << "reach = " << net.reach << "  " << std::endl;*/
    }
  }

  /*if (num_violations > 0) {
    std::cout << "Number of violations: " << num_violations << " / "
              << nets_.size() << "  "
              << "Average violation: " << avg_violation / num_violations << "  "
              << "Total penalty: " << total_penalty << std::endl;
  }*/
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
  area_penalty_ = calAreaPenalty();
  package_penalty_ = calPackagePenalty();
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
  /*std::vector<float> area_penalty_list;
  std::vector<float> package_penalty_list;
  std::vector<float> net_penalty_list;

  for (int i = 0; i < num_perturb_per_step_; i++) {
    perturb();
    calPenalty();
    area_penalty_list.push_back(area_penalty_);
    package_penalty_list.push_back(package_penalty_);
    net_penalty_list.push_back(net_penalty_);
  }

  norm_area_penalty_ = std::accumulate(
      area_penalty_list.begin(), area_penalty_list.end(), 0.0);
  norm_package_penalty_ = std::accumulate(
      package_penalty_list.begin(), package_penalty_list.end(), 0.0);
  norm_net_penalty_
      = std::accumulate(net_penalty_list.begin(), net_penalty_list.end(), 0.0);
  norm_area_penalty_ /= area_penalty_list.size();
  norm_package_penalty_ /= package_penalty_list.size();
  norm_net_penalty_ /= net_penalty_list.size();*/
}

float SACore::calNormCost()
{
  float norm_cost = 0.0;
  calPenalty();
  norm_cost += area_penalty_weight_ * area_penalty_ / norm_area_penalty_;
  norm_cost
      += package_penalty_weight_ * package_penalty_ / norm_package_penalty_;
  norm_cost += net_penalty_weight_ * net_penalty_ / norm_net_penalty_;
  return norm_cost;
}

void SACore::run(float cooling_acceleration_factor)
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
  int step = 1;
  float temperature = init_temperature_;
  /*const float t_factor = std::exp(std::log(min_temperature_ / init_temp)
                                  / max_num_step_);*/
  const float t_factor
      = std::exp(std::log(min_temperature_ / init_temperature_)
                 / (max_num_step_ * cooling_acceleration_factor));
  // SA process
  while (step <= max_num_step_) {
    for (int i = 0; i < num_perturb_per_step_; i++) {
      perturb();
      cost = calNormCost();
      delta_cost = cost - pre_cost;
      const float num = distribution_(generator_);
      const float prob
          = (delta_cost > 0.0) ? exp((-1) * delta_cost / temperature) : 1;
      if (num < prob) {
        pre_cost = cost;
      } else {
        restore();
      }
    };
    // end time
    temperature *= t_factor;
    cost_list_.push_back(pre_cost);
    T_list_.push_back(temperature);
    // increase step
    step++;
  }  // end while
  // update the final results
  packFloorplan();
  calPenalty();
}

}  // namespace par