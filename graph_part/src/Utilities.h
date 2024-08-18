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
