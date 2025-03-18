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
#include <execinfo.h>

namespace chiplet {
std::string GetVectorString(const std::vector<float> &vec) {
  if (vec.empty()) {
    return "";
  }
  
  std::string result;
  result.reserve(vec.size() * 10); // Preallocate memory (estimate)
  
  for (size_t i = 0; i < vec.size() - 1; ++i) {
    result += std::to_string(vec[i]) + " ";
  }
  // Add last element without trailing space
  result += std::to_string(vec.back());
  
  return result;
}

namespace {
// Helper function to check if a character matches any in a delimiter string
inline bool CharMatch(char c, const std::string &delim) {
  return delim.find(c) != std::string::npos;
}

// Helper function to find the next delimiter character
std::string::const_iterator FindDelim(std::string::const_iterator start,
                                    std::string::const_iterator end,
                                    const std::string &delim) {
  return std::find_if(start, end, 
                    [&delim](char c) { return CharMatch(c, delim); });
}

// Helper function to find the next non-delimiter character
std::string::const_iterator FindNotDelim(std::string::const_iterator start,
                                       std::string::const_iterator end,
                                       const std::string &delim) {
  return std::find_if_not(start, end, 
                        [&delim](char c) { return CharMatch(c, delim); });
}
} // anonymous namespace

// Split a string based on deliminator : empty space and ","
std::vector<std::string> SplitLine(const std::string &line) {
  static const std::string delimiters(", "); // Space and comma
  std::vector<std::string> items;
  
  auto start = line.cbegin();
  while (start != line.cend()) {
    start = FindNotDelim(start, line.cend(), delimiters);
    if (start == line.cend()) break;
    
    auto end = FindDelim(start, line.cend(), delimiters);
    items.emplace_back(start, end);
    start = end;
  }
  
  return items;
}

// Add right vector to left vector
void Accumulate(std::vector<float> &a, const std::vector<float> &b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match for accumulation");
  }
  
  std::transform(a.begin(), a.end(), b.begin(), a.begin(),
                std::plus<float>());
}

// weighted sum
std::vector<float> WeightedSum(const std::vector<float> &a, float a_factor,
                             const std::vector<float> &b, float b_factor) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match for weighted sum");
  }
  
  const float total_factor = a_factor + b_factor;
  if (total_factor == 0.0f) {
    throw std::invalid_argument("Sum of factors cannot be zero");
  }
  
  std::vector<float> result(a.size());
  for (size_t i = 0; i < a.size(); ++i) {
    result[i] = (a[i] * a_factor + b[i] * b_factor) / total_factor;
  }
  
  return result;
}

// divide the vector
std::vector<float> DivideFactor(const std::vector<float> &a, float factor) {
  if (factor == 0.0f) {
    throw std::invalid_argument("Division by zero in DivideFactor");
  }
  
  std::vector<float> result(a.size());
  std::transform(a.begin(), a.end(), result.begin(),
                [factor](float value) { return value / factor; });
  
  return result;
}

// multiply the vector
std::vector<float> MultiplyFactor(const std::vector<float> &a, float factor) {
  std::vector<float> result(a.size());
  std::transform(a.begin(), a.end(), result.begin(),
                [factor](float value) { return value * factor; });
  
  return result;
}

// divide the vectors element by element
std::vector<float> DivideVectorElebyEle(const std::vector<float> &emb,
                                        const std::vector<float> &factor) {
  const size_t size = std::min(emb.size(), factor.size());
  std::vector<float> result(size);
  
  for (size_t i = 0; i < size; ++i) {
    result[i] = (factor[i] != 0.0f) ? emb[i] / factor[i] : emb[i];
  }
  
  return result;
}

// operation for two vectors +, -, *,  ==, <
std::vector<float> operator+(const std::vector<float> &a,
                             const std::vector<float> &b) {
  if (a.size() != b.size()) {
    std::cerr << "Vector size mismatch in operator+: a.size()=" << a.size() 
              << ", b.size()=" << b.size() << std::endl;
    
    // Print stack trace to help identify where the error is occurring
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** symbols = backtrace_symbols(callstack, frames);
    
    std::cerr << "Call stack:" << std::endl;
    for (int i = 0; i < frames; ++i) {
      std::cerr << symbols[i] << std::endl;
    }
    
    free(symbols);
    
    throw std::invalid_argument("Vector sizes must match for addition");
  }
  
  std::vector<float> result(a.size());
  std::transform(a.begin(), a.end(), b.begin(), result.begin(),
                std::plus<float>());
  
  return result;
}

std::vector<float> operator-(const std::vector<float> &a,
                             const std::vector<float> &b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match for subtraction");
  }
  
  std::vector<float> result(a.size());
  std::transform(a.begin(), a.end(), b.begin(), result.begin(),
                std::minus<float>());
  
  return result;
}

std::vector<float> operator*(const std::vector<float> &a,
                             const std::vector<float> &b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match for multiplication");
  }
  
  std::vector<float> result(a.size());
  std::transform(a.begin(), a.end(), b.begin(), result.begin(),
                std::multiplies<float>());
  
  return result;
}

std::vector<float> operator*(const std::vector<float> &a, float factor) {
  return MultiplyFactor(a, factor);
}

bool operator<(const std::vector<float> &a, const std::vector<float> &b) {
  if (a.size() != b.size()) {
    throw std::invalid_argument("Vector sizes must match for comparison");
  }
  
  return std::lexicographical_compare(
      a.begin(), a.end(), b.begin(), b.end(),
      [](float a_val, float b_val) { return a_val < b_val; });
}

bool operator==(const std::vector<float> &a, const std::vector<float> &b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

bool operator<=(const Matrix<float> &a, const Matrix<float> &b) {
  const size_t num_dim = std::min(a.size(), b.size());
  
  for (size_t dim = 0; dim < num_dim; ++dim) {
    if (!(a[dim] < b[dim] || a[dim] == b[dim])) {
      return false;
    }
  }
  
  return true;
}

// Basic functions for a vector
std::vector<float> abs(const std::vector<float> &a) {
  std::vector<float> result(a.size());
  std::transform(a.begin(), a.end(), result.begin(),
                [](float value) { return std::abs(value); });
  
  return result;
}

float norm2(const std::vector<float> &a) {
  return std::sqrt(std::inner_product(a.begin(), a.end(), a.begin(), 0.0f));
}

float norm2(const std::vector<float> &a, const std::vector<float> &factor) {
  if (a.size() > factor.size()) {
    throw std::invalid_argument("Factor vector must be at least as large as vector a");
  }
  
  float result = 0.0f;
  for (size_t i = 0; i < a.size(); ++i) {
    result += a[i] * a[i] * std::abs(factor[i]);
  }
  
  return std::sqrt(result);
}

} // namespace chiplet