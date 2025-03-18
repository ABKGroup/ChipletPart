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

#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <vector>
#include <algorithm>
#include <chrono>
#include <numeric>

namespace chiplet {

struct BundledNet {
  BundledNet() = default;

  BundledNet(std::pair<int, int> terminals_, int weight_, float reach_,
             float io_area_)
      : terminals(terminals_), weight(weight_), reach(reach_),
        io_area(io_area_) {}

  std::pair<int, int> terminals;
  int weight = 0;
  float reach = 0.0f;
  float io_area = 1.0f;
};

struct Chiplet {
  Chiplet() = default;

  Chiplet(float x_, float y_, float width_, float height_, float min_area_,
          float halo_width_)
      : x(x_), y(y_), width(width_), height(height_), min_area(min_area_),
        halo_width(halo_width_) {}

  Chiplet(const Chiplet& other) = default;
  Chiplet& operator=(const Chiplet& other) = default;
  
  // Move constructor and assignment for efficiency
  Chiplet(Chiplet&& other) noexcept = default;
  Chiplet& operator=(Chiplet&& other) noexcept = default;

  void setX(float x_) { x = x_; }
  void setY(float y_) { y = y_; }

  float getWidth() const { return width + 2 * halo_width; }
  float getHeight() const { return height + 2 * halo_width; }

  float getX() const { return x; }
  float getY() const { return y; }
  float getRealX() const { return x + halo_width; }
  float getRealY() const { return y + halo_width; }
  float getRealWidth() const { return width; }
  float getRealHeight() const { return height; }

  float getArea() const { return width * height; }

  void setWidth(float width_) {
    if (width_ <= 2 * halo_width) {
      return;
    }

    float area = std::max(min_area, width * height);
    float min_width = std::sqrt(area / max_ar_);
    float max_width = std::sqrt(area / min_ar_);
    width = std::clamp(width_ - 2 * halo_width, min_width, max_width);
    height = area / width;
  }

  void setHeight(float height_) {
    if (height_ <= 2 * halo_width) {
      return;
    }

    float area = std::max(width * height, min_area);
    float max_height = std::sqrt(area * max_ar_);
    float min_height = std::sqrt(area * min_ar_);
    height = std::clamp(height_ - 2 * halo_width, min_height, max_height);
    width = area / height;
  }

  void setShape(float width_, float height_) {
    if (width_ <= getWidth() || height_ <= getHeight()) {
      return;
    }

    width = width_ - 2 * halo_width;
    height = height_ - 2 * halo_width;
    float aspect_ratio = width / height;
    float area = std::max(width * height, min_area);
    height = std::sqrt(area / aspect_ratio);
    width = area / height;
  }

  float getMinArea() const { return min_area; }

  void resizeRandomly(float aspect_ratio) {
    float area = std::max(width * height, min_area);
    height = std::sqrt(area / aspect_ratio);
    width = area / height;
  }

  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  float min_area = 0.0f;
  float halo_width = 0.0f;
  float min_ar_ = 0.2f;
  float max_ar_ = 5.0f;
};

class SACore {
public:
  SACore(std::vector<Chiplet> chiplets, 
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
         std::vector<int>& neg_seq);

  void run(float cooling_acceleration_factor);
  
  // Query methods
  bool isValid() const;
  float getPackageSize() const { return width_ * height_; }
  float getCost() { return calNormCost(); }
  
  float getNormAreaPenalty() const { return norm_area_penalty_; }
  float getNormPackagePenalty() const { return norm_package_penalty_; }
  float getNormNetPenalty() const { return norm_net_penalty_; }
  float getCoolingRate() const { return cooling_rate_; }

  // Setter methods
  void setNormAreaPenalty(float norm_area_penalty) {
    norm_area_penalty_ = norm_area_penalty;
  }

  void setNormPackagePenalty(float norm_package_penalty) {
    norm_package_penalty_ = norm_package_penalty;
  }

  void setNormNetPenalty(float norm_net_penalty) {
    norm_net_penalty_ = norm_net_penalty;
  }
  
  // Data access methods
  void getMacros(std::vector<Chiplet>& macros) const { macros = macros_; }
  void getPosSeq(std::vector<int>& pos_seq) const { pos_seq = pos_seq_; }
  void getNegSeq(std::vector<int>& neg_seq) const { neg_seq = neg_seq_; }
  void setPosSeq(const std::vector<int>& pos_seq) { pos_seq_ = pos_seq; }
  void setNegSeq(const std::vector<int>& neg_seq) { neg_seq_ = neg_seq; }

  // Core methods
  void initialize();
  void checkViolation();

private:
  // Penalty calculation
  void calPenalty();
  float calNetPenalty() const;
  float calAreaPenalty() const;
  float calPackagePenalty() const;
  float calNormCost();
  float calNetViolation(const BundledNet* net) const;

  // Core SA operations
  void perturb();
  void restore();
  void packFloorplan();

  // Perturbation actions
  void singleSeqSwap(bool pos);
  void doubleSeqSwap();
  void resizeOneCluster();
  void expandClusters();
  
  // Helper method
  void calSegmentLoc(float seg_start, float seg_end, int& start_id, int& end_id,
                     const std::vector<float>& grid);

  /////////////////////////////////////////////
  // Private member variables
  /////////////////////////////////////////////
  // nets
  std::vector<BundledNet> nets_;

  // weight for different penalty
  float area_penalty_weight_ = 0.0f;
  float package_penalty_weight_ = 0.0f;
  float net_penalty_weight_ = 0.0f;

  // operation parameters
  float pos_swap_prob_ = 0.0f;
  float neg_swap_prob_ = 0.0f;
  float double_swap_prob_ = 0.0f;
  float resize_prob_ = 0.0f;
  float expand_prob_ = 0.0f;

  // SA parameters
  int max_num_step_ = 0;
  int num_perturb_per_step_ = 0;
  float cooling_rate_ = 0.0f;
  float init_temperature_ = 1.0f;
  float min_temperature_ = 1e-10f;
  unsigned seed_ = 0;

  // Random number generation
  std::mt19937 generator_;
  std::uniform_real_distribution<float> distribution_;

  // current solution
  std::vector<int> pos_seq_;
  std::vector<int> neg_seq_;
  std::vector<Chiplet> macros_; 

  // previous solution
  std::vector<int> pre_pos_seq_;
  std::vector<int> pre_neg_seq_;
  std::vector<Chiplet> pre_macros_;
  int macro_id_ = -1;  // the macro changed in the perturb
  int action_id_ = -1; // the action_id of current step

  // metrics
  float width_ = 0.0f;
  float height_ = 0.0f;
  float pre_width_ = 0.0f;
  float pre_height_ = 0.0f;

  float area_penalty_ = 0.0f;
  float package_penalty_ = 0.0f;
  float net_penalty_ = 0.0f;

  float pre_area_penalty_ = 0.0f;
  float pre_package_penalty_ = 0.0f;
  float pre_net_penalty_ = 0.0f;

  float norm_area_penalty_ = 0.0f;
  float norm_package_penalty_ = 0.0f;
  float norm_net_penalty_ = 0.0f;

  float net_reach_penalty_acc_ = 0.01f;

  std::vector<float> cost_list_; // store the cost in the list
  std::vector<float> T_list_;    // store the temperature
};

using SACorePtr = std::shared_ptr<SACore>;

} // namespace chiplet
