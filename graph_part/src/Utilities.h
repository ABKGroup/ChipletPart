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
#include <map>
#include <string>
#include <vector>

namespace chiplet {
template <typename T> using Matrix = std::vector<std::vector<T>>;

std::string GetVectorString(const std::vector<float> &vec);

// Split a string based on deliminator : empty space and ","
std::vector<std::string> SplitLine(const std::string &line);

// Add right vector to left vector
void Accumulate(std::vector<float> &a, const std::vector<float> &b);

// weighted sum
std::vector<float> WeightedSum(const std::vector<float> &a, float a_factor,
                               const std::vector<float> &b, float b_factor);

// divide the vector
std::vector<float> DivideFactor(const std::vector<float> &a, float factor);

// divide the vectors element by element
std::vector<float> DivideVectorElebyEle(const std::vector<float> &emb,
                                        const std::vector<float> &factor);

// multiplty the vector
std::vector<float> MultiplyFactor(const std::vector<float> &a, float factor);

// operation for two vectors +, -, *,  ==, <
std::vector<float> operator+(const std::vector<float> &a,
                             const std::vector<float> &b);

std::vector<float> operator*(const std::vector<float> &a, float factor);

std::vector<float> operator-(const std::vector<float> &a,
                             const std::vector<float> &b);

std::vector<float> operator*(const std::vector<float> &a,
                             const std::vector<float> &b);

bool operator<(const std::vector<float> &a, const std::vector<float> &b);

bool operator<=(const Matrix<float> &a, const Matrix<float> &b);

bool operator==(const std::vector<float> &a, const std::vector<float> &b);

// Basic functions for a vector
std::vector<float> abs(const std::vector<float> &a);

float norm2(const std::vector<float> &a);

float norm2(const std::vector<float> &a, const std::vector<float> &factor);

} // namespace chiplet