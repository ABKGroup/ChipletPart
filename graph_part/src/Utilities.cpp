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
// This file includes the basic utility functions for operations
///////////////////////////////////////////////////////////////////////////////
#include "Utilities.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <random>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace chiplet {
std::string GetVectorString(const std::vector<float> &vec) {
  std::string line;
  for (auto value : vec) {
    line += std::to_string(value) + " ";
  }
  return line;
}

// Convert Tcl list to vector

// char_match:  determine if the char is part of deliminators
bool CharMatch(char c, const std::string &delim) {
  auto it = delim.begin();
  while (it != delim.end()) {
    if ((*it) == c) {
      return true;
    }
    ++it;
  }
  return false;
}

// find the next position for deliminator char
std::string::const_iterator FindDelim(std::string::const_iterator start,
                                      std::string::const_iterator end,
                                      const std::string &delim) {
  while (start != end && !CharMatch(*start, delim)) {
    start++;
  }
  return start;
}

// find the next position for non deliminator char
std::string::const_iterator FindNotDelim(std::string::const_iterator start,
                                         std::string::const_iterator end,
                                         const std::string &delim) {
  while (start != end && CharMatch(*start, delim)) {
    start++;
  }
  return start;
}

// Split a string based on deliminator : empty space and ","
std::vector<std::string> SplitLine(const std::string &line) {
  std::vector<std::string> items;
  std::string deliminators(", "); // empty space ,
  auto start = line.cbegin();
  while (start != line.cend()) {
    start = FindNotDelim(start, line.cend(), deliminators);
    auto end = FindDelim(start, line.cend(), deliminators);
    if (start != line.cend()) {
      items.emplace_back(start, end);
      start = end;
    }
  }
  return items;
}

// Add right vector to left vector
void Accumulate(std::vector<float> &a, const std::vector<float> &b) {
  assert(a.size() == b.size());
  std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<float>());
}

// weighted sum
std::vector<float> WeightedSum(const std::vector<float> &a,
                               const float a_factor,
                               const std::vector<float> &b,
                               const float b_factor) {
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  auto a_iter = a.begin();
  auto b_iter = b.begin();
  while (a_iter != a.end()) {
    result.push_back(((*a_iter++) * a_factor + (*b_iter++) * b_factor) /
                     (a_factor + b_factor));
  }
  return result;
}

// divide the vector
std::vector<float> DivideFactor(const std::vector<float> &a,
                                const float factor) {
  std::vector<float> result = a;
  for (auto &value : result) {
    value /= factor;
  }
  return result;
}

// multiply the vector
std::vector<float> MultiplyFactor(const std::vector<float> &a,
                                  const float factor) {
  std::vector<float> result = a;
  for (auto &value : result) {
    value *= factor;
  }
  return result;
}

// divide the vectors element by element
std::vector<float> DivideVectorElebyEle(const std::vector<float> &emb,
                                        const std::vector<float> &factor) {
  std::vector<float> result;
  auto emb_iter = emb.begin();
  auto factor_iter = factor.begin();
  while (emb_iter != emb.end() && factor_iter != factor.end()) {
    if ((*factor_iter) != 0.0) {
      result.push_back((*emb_iter) / (*factor_iter));
    } else {
      result.push_back(*emb_iter);
    }
    emb_iter++;
    factor_iter++;
  }
  return result;
}

// operation for two vectors +, -, *,  ==, <
std::vector<float> operator+(const std::vector<float> &a,
                             const std::vector<float> &b) {
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(), a.end(), b.begin(), std::back_inserter(result),
                 std::plus<float>());
  return result;
}

std::vector<float> operator-(const std::vector<float> &a,
                             const std::vector<float> &b) {
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(), a.end(), b.begin(), std::back_inserter(result),
                 std::minus<float>());
  return result;
}

std::vector<float> operator*(const std::vector<float> &a,
                             const std::vector<float> &b) {
  assert(a.size() == b.size());
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(), a.end(), b.begin(), std::back_inserter(result),
                 std::multiplies<float>());
  return result;
}

std::vector<float> operator*(const std::vector<float> &a, const float factor) {
  std::vector<float> result;
  result.reserve(a.size());
  for (auto value : a) {
    result.push_back(value * factor);
  }
  return result;
}

bool operator<(const std::vector<float> &a, const std::vector<float> &b) {
  assert(a.size() == b.size());
  auto a_iter = a.begin();
  auto b_iter = b.begin();
  while (a_iter != a.end()) {
    if ((*a_iter++) >= (*b_iter++)) {
      return false;
    }
  }
  return true;
}

bool operator==(const std::vector<float> &a, const std::vector<float> &b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

bool operator<=(const Matrix<float> &a, const Matrix<float> &b) {
  const int num_dim =
      std::min(static_cast<int>(a.size()), static_cast<int>(b.size()));
  for (int dim = 0; dim < num_dim; dim++) {
    if ((a[dim] < b[dim]) || (a[dim] == b[dim])) {
      continue;
    }
    return false;
  }
  return true;
}

// Basic functions for a vector
std::vector<float> abs(const std::vector<float> &a) {
  std::vector<float> result;
  result.reserve(a.size());
  std::transform(a.begin(), a.end(), std::back_inserter(result),
                 static_cast<float (*)(float)>(&std::abs));
  return result;
}

float norm2(const std::vector<float> &a) {
  float result{0};
  result = std::inner_product(a.begin(), a.end(), a.begin(), result);
  return std::sqrt(result);
}

float norm2(const std::vector<float> &a, const std::vector<float> &factor) {
  float result{0};
  assert(a.size() <= factor.size());
  auto a_iter = a.begin();
  auto factor_iter = factor.begin();
  while (a_iter != a.end()) {
    result += (*a_iter) * (*a_iter) * std::abs(*factor_iter);
    a_iter++;
    factor_iter++;
  }
  return std::sqrt(result);
}

} // namespace chiplet