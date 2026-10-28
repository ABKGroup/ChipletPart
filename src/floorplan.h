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

#include <map>
#include <random>
#include <vector>
#include <limits>
#include <memory>
#include <cmath>
#include <iostream>

struct BundledNet {
  BundledNet() { }

  BundledNet(std::pair<int, int> terminals_, float weight_, float reach_)
    : terminals(terminals_), weight(weight_), reach(reach_) { }
    
  BundledNet(std::pair<int, int> terminals_, float weight_, float reach_, float io_area_)
    : terminals(terminals_), weight(weight_), reach(reach_), io_area(io_area_) { }
  
  std::pair<int, int> terminals;
  // Bandwidth-derived weight. Must be float: fractional bandwidths (e.g. 0.8)
  // used to be truncated to 0 by an int member, silently dropping the net.
  float weight = 0.0f;
  float reach;
  float io_area = 1.0;
};


struct Chiplet
{
  Chiplet() { }
  
  Chiplet(float x_, float y_, float width_, float height_, float min_area_, float halo_width_)
    : x(x_), y(y_), width(width_), height(height_), min_area(min_area_), halo_width(halo_width_) {}
  
  Chiplet(Chiplet const& other)
    : x(other.x), y(other.y), width(other.width), height(other.height), min_area(other.min_area), halo_width(other.halo_width) {}
  
  Chiplet& operator=(Chiplet const& other) {
    x = other.x;
    y = other.y;
    width = other.width;
    height = other.height;
    min_area = other.min_area;
    halo_width = other.halo_width;
    return *this;
  }
  
  void setX(float x_) { x = x_; }
  void setY(float y_) { y = y_; }

  float getWidth() const { return width + 2 * halo_width; };
  float getHeight() const { return height + 2 * halo_width; };

  float getX() const { return x; };
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

    const float area = nominalArea();
    float min_width = std::sqrt(area / max_ar_);
    float max_width = std::sqrt(area / min_ar_);
    width = width_ - 2 * halo_width;
    width = std::max(min_width, std::min(max_width, width));
    height = area / width;
  };

  void setHeight(float height_) {
    if (height_ <= 2 * halo_width) {
      return;
    }
  
    const float area = nominalArea();
    float max_height = std::sqrt(area * max_ar_);
    float min_height = std::sqrt(area * min_ar_);
    height = height_ - 2 * halo_width;
    height = std::max(min_height, std::min(max_height, height));
    width = area / height;
  }
  
  // Grow the chiplet to fill the (halo-inclusive) box width_ x height_.
  // Only growth is allowed: the box must not be smaller than the current
  // footprint in either direction, and must be larger in at least one.
  // (The previous version required growth in BOTH directions and then
  // recomputed height from width/height, which swapped the two dimensions.)
  void setShape(float width_, float height_) {
    const float eps = 1e-6f;
    if (width_ < getWidth() - eps || height_ < getHeight() - eps) {
      return;
    }
    if (width_ <= getWidth() + eps && height_ <= getHeight() + eps) {
      return;
    }
    float new_width = width_ - 2 * halo_width;
    float new_height = height_ - 2 * halo_width;
    if (new_width <= 0.0f || new_height <= 0.0f) {
      return;
    }
    if (new_width * new_height < min_area) {
      const float scale = std::sqrt(min_area / (new_width * new_height));
      new_width *= scale;
      new_height *= scale;
    }
    width = new_width;
    height = new_height;
  }

  float getMinArea() const { return min_area; }
  
  void resizeRandomly(float aspect_ratio) {
    const float area = nominalArea();
    height = std::sqrt(area * aspect_ratio);
    width = area / height;
  }

  // Area a resize operation works with. Resizing always returns the chiplet
  // to its nominal (block) area: growth beyond it only ever comes from
  // expandClusters() and must stay reversible, otherwise accepted expansions
  // ratchet the area up over the run (it could never shrink again).
  float nominalArea() const {
    return (min_area > 0.0f) ? min_area : width * height;
  }

  float x = 0.0;
  float y = 0.0;
  float width = 0.0;
  float height = 0.0;
  float min_area = 0.0;
  float halo_width = 0.0;
  float min_ar_ = 0.2;
  float max_ar_ = 5.0;
};


// A floorplan starting point: sequence pair plus per-chiplet aspect ratio
// (height / width at nominal area). Carried by the feasibility-aware refiner
// from one probe to the next and into the full floorplanner.
struct FloorplanSeed
{
  std::vector<int> pos_seq;
  std::vector<int> neg_seq;
  std::vector<float> aspect_ratios;
  bool empty() const { return pos_seq.empty(); }
  size_t size() const { return pos_seq.size(); }
  void clear() { pos_seq.clear(); neg_seq.clear(); aspect_ratios.clear(); }
};

class SACore
{
  public:
    SACore(
        int worker_id,
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
        unsigned seed);                                
 
 
    int getWorkerId() const {
      return worker_id_;
    }

    // Run the full annealing schedule (max_num_step steps from the current
    // temperature).
    void run();
    // Run `steps` annealing steps from the current temperature, so that a
    // schedule can be split into segments (used by go-with-the-winners).
    void runSteps(int steps);
    bool isValid() const;
    // Returns true if the solution currently satisfies every net reach.
    float getNetPenalty() const { return net_penalty_; }
    float getTemp() const { return temperature_; }
    void getMacros(std::vector<Chiplet>& macros) const;   
    void getPosSeq(std::vector<int>& pos_seq) const {
      pos_seq = pos_seq_;
    }
    
    void getNegSeq(std::vector<int>& neg_seq) const {
      neg_seq = neg_seq_;
    }

    void setPosSeq(const std::vector<int>& pos_seq) {
      pos_seq_ = pos_seq;
    }

    void setNegSeq(const std::vector<int>& neg_seq) {
      neg_seq_ = neg_seq;
    }

    float getPackageSize() const;
    float getCost() {
      return calNormCost();
    }
  
    float getNormAreaPenalty() const {
      return norm_area_penalty_;
    }

    float getNormPackagePenalty() const {
      return norm_package_penalty_;
    }

    float getNormNetPenalty() const {
      return norm_net_penalty_;
    }

    void setNormAreaPenalty(float norm_area_penalty) {
      norm_area_penalty_ = norm_area_penalty;
    }

    void setNormPackagePenalty(float norm_package_penalty) {
      norm_package_penalty_ = norm_package_penalty;
    }

    void setNormNetPenalty(float norm_net_penalty) {
      norm_net_penalty_ = norm_net_penalty;
    }

    void initialize();
    // Pack and evaluate the current sequence pair / shapes without perturbing
    // them (initialize() applies one random perturbation first). Sets the
    // normalisation constants if they are still unset.
    void evaluateCurrent();
    // Smallest total reach violation seen at any accepted state so far, and
    // (when one was feasible) the sequence pair of the last feasible state.
    float getMinNetPenaltySeen() const { return min_net_penalty_seen_; }
    bool hasSeenValid() const { return seen_valid_; }
    void getValidSeqs(std::vector<int>& pos_seq, std::vector<int>& neg_seq) const {
      pos_seq = valid_pos_seq_;
      neg_seq = valid_neg_seq_;
    }
    // Reinstate the last feasible state (sequence pair and shapes) if the
    // walk ended in an infeasible one. Returns false if none was seen.
    bool restoreValid();
    // Lowest-violation state seen so far (feasible or not): export it as a
    // seed, or jump back to it.
    void getMinSeed(FloorplanSeed& seed) const;
    void restoreMin();
    // Start from `seed` (sequence pair and, when sizes match, shapes).
    void applySeed(const FloorplanSeed& seed);
    // Greedy descent on the reach violation alone: `num_perturbs` random
    // perturbations, each kept only if it does not increase the violation.
    // Used by the per-move feasibility probe of the partitioner.
    void quench(int num_perturbs);
    float getCoolingRate() const {
      return cooling_rate_;
    }

    bool checkViolation();
    // Summary of reach violations in the current solution: number of
    // violating nets and the largest excess length; prints up to max_lines
    // individual violations to `os`.
    int reportViolations(std::ostream& os, int max_lines) const;
    void setTemp(float temp) {
      init_temperature_ = temp;
      temperature_ = temp;
    }

    void setMacros(const std::vector<Chiplet>& macros) {
      macros_ = macros;
      pre_macros_ = macros;
    }

    // Copy the complete solution state (sequence pair + macro shapes) from
    // another worker; the receiving worker keeps its own RNG and schedule.
    void adoptSolution(const SACore& other) {
      pos_seq_ = other.pos_seq_;
      neg_seq_ = other.neg_seq_;
      pre_pos_seq_ = other.pos_seq_;
      pre_neg_seq_ = other.neg_seq_;
      macros_ = other.macros_;
      pre_macros_ = other.macros_;
    }

  private:
    void calPenalty();
    float calNetPenalty() const;
    float calAreaPenalty() const;
    float calPackagePenalty() const;
    float calNormCost();

    // operations
    void perturb();
    void restore();
    void packFloorplan();

    float calNetViolation(const BundledNet* net) const;
    // Uniform random index in [0, n). Clamped: libc++'s
    // uniform_real_distribution<float> can return exactly 1.0f, which would
    // otherwise index one past the end.
    int randIndex(int n) {
      const int idx = static_cast<int>(std::floor(distribution_(generator_) * n));
      return std::min(std::max(idx, 0), n - 1);
    }

    // actions used
    void singleSeqSwap(bool pos);
    void doubleSeqSwap();
    void resizeOneCluster();
    // expand clusters to fill all the deadspace
    void expandClusters();
    void calSegmentLoc(float seg_start,
                       float seg_end,
                       int& start_id,
                       int& end_id,
                       std::vector<float>& grid);

    /////////////////////////////////////////////
    // private member variables
    /////////////////////////////////////////////
    // nets
    std::vector<BundledNet> nets_;
   
    // weight for different penalty
    float area_penalty_weight_ = 0.0;
    float package_penalty_weight_ = 0.0;
    float net_penalty_weight_ = 0.0;

    // operation parameters
    float pos_swap_prob_ = 0.0;
    float neg_swap_prob_ = 0.0;
    float double_swap_prob_ = 0.0;
    float resize_prob_ = 0.0;
    float expand_prob_ = 0.0;

    // SA parameters
    int max_num_step_ = 0;
    int num_perturb_per_step_ = 0;
    float cooling_rate_ = 0.0;
    float init_temperature_ = 1.0;
    float temperature_ = 1.0;        // current temperature of the schedule
    float min_temperature_ = 1e-10;  // floor: avoids exp(-x/0) once cooled out
    unsigned seed_ = 0;
    int worker_id_ = 0;

    // seed for reproduciabilty
    std::mt19937 generator_;
    std::uniform_real_distribution<float> distribution_;

    // current solution
    std::vector<int> pos_seq_;
    std::vector<int> neg_seq_;
    std::vector<Chiplet> macros_;  // here the macros can be HardMacro or SoftMacro

    // previous solution
    std::vector<int> pre_pos_seq_;
    std::vector<int> pre_neg_seq_;
    std::vector<Chiplet> pre_macros_;  // here the macros can be HardMacro or SoftMacro
    int macro_id_ = -1;          // the macro changed in the perturb
    int action_id_ = -1;         // the action_id of current step

    // metrics
    float width_ = 0.0;
    float height_ = 0.0;
    float pre_width_ = 0.0;
    float pre_height_ = 0.0;

    float area_penalty_ = 0.0;
    float package_penalty_ = 0.0;
    float net_penalty_ = 0.0;

    float pre_area_penalty_ = 0.0;
    float pre_package_penalty_ = 0.0;
    float pre_net_penalty_ = 0.0;

    float norm_area_penalty_ = 0.0;
    float norm_package_penalty_ = 0.0;
    float norm_net_penalty_ = 0.0;

    float net_reach_penalty_acc_ = 0.01;
    float min_net_penalty_seen_ = std::numeric_limits<float>::max();
    bool seen_valid_ = false;
    std::vector<int> valid_pos_seq_;
    std::vector<int> valid_neg_seq_;
    std::vector<Chiplet> valid_macros_;
    std::vector<int> min_pos_seq_;
    std::vector<int> min_neg_seq_;
    std::vector<Chiplet> min_macros_;
    void trackViolation();

    std::vector<float> cost_list_;  // store the cost in the list
    std::vector<float> T_list_;     // store the temperature
};

using SACorePtr = std::shared_ptr<SACore>;





