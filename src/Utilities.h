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
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <map>
#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <cassert>

namespace chiplet {

/**
 * @brief Matrix is a convenience alias for a 2D vector
 */
template <typename T> using Matrix = std::vector<std::vector<T>>;

/**
 * @brief Converts a vector of floats to a space-separated string
 * @param vec The vector to convert
 * @return A string representation of the vector
 */
std::string GetVectorString(const std::vector<float>& vec);

/**
 * @brief Splits a string into tokens based on delimiters (space and comma)
 * @param line The string to split
 * @return A vector of tokens
 */
std::vector<std::string> SplitLine(const std::string& line);

/**
 * @brief Adds the elements of vector b to vector a in-place
 * @param a The target vector to be modified
 * @param b The vector to add
 * @throws std::invalid_argument if vectors have different sizes
 */
void Accumulate(std::vector<float>& a, const std::vector<float>& b);

/**
 * @brief Calculates the weighted sum of two vectors
 * @param a First vector
 * @param a_factor Weight for first vector
 * @param b Second vector
 * @param b_factor Weight for second vector
 * @return The weighted sum as a new vector
 * @throws std::invalid_argument if vectors have different sizes
 */
std::vector<float> WeightedSum(const std::vector<float>& a, float a_factor,
                             const std::vector<float>& b, float b_factor);

/**
 * @brief Divides each element of a vector by a factor
 * @param a The vector to divide
 * @param factor The divisor
 * @return A new vector with the divided values
 */
std::vector<float> DivideFactor(const std::vector<float>& a, float factor);

/**
 * @brief Divides each element of a vector by the corresponding element in another vector
 * @param emb The numerator vector
 * @param factor The denominator vector
 * @return A new vector with element-wise division results
 */
std::vector<float> DivideVectorElebyEle(const std::vector<float>& emb,
                                      const std::vector<float>& factor);

/**
 * @brief Multiplies each element of a vector by a factor
 * @param a The vector to multiply
 * @param factor The multiplier
 * @return A new vector with the multiplied values
 */
std::vector<float> MultiplyFactor(const std::vector<float>& a, float factor);

// Vector operators

/**
 * @brief Adds two vectors element-wise
 * @param a First vector
 * @param b Second vector
 * @return A new vector containing the sum
 * @throws std::invalid_argument if vectors have different sizes
 */
std::vector<float> operator+(const std::vector<float>& a,
                           const std::vector<float>& b);

/**
 * @brief Multiplies a vector by a scalar
 * @param a The vector
 * @param factor The scalar multiplier
 * @return A new vector with each element multiplied by the factor
 */
std::vector<float> operator*(const std::vector<float>& a, float factor);

/**
 * @brief Subtracts two vectors element-wise
 * @param a First vector
 * @param b Second vector
 * @return A new vector containing the difference
 * @throws std::invalid_argument if vectors have different sizes
 */
std::vector<float> operator-(const std::vector<float>& a,
                           const std::vector<float>& b);

/**
 * @brief Multiplies two vectors element-wise
 * @param a First vector
 * @param b Second vector
 * @return A new vector containing the element-wise products
 * @throws std::invalid_argument if vectors have different sizes
 */
std::vector<float> operator*(const std::vector<float>& a,
                           const std::vector<float>& b);

/**
 * @brief Checks if all elements in vector a are less than corresponding elements in b
 * @param a First vector
 * @param b Second vector
 * @return True if all elements in a are less than corresponding elements in b
 * @throws std::invalid_argument if vectors have different sizes
 */
bool operator<(const std::vector<float>& a, const std::vector<float>& b);

/**
 * @brief Checks if all rows in matrix a are less than or equal to corresponding rows in b
 * @param a First matrix
 * @param b Second matrix
 * @return True if all rows meet the less than or equal condition
 */
bool operator<=(const Matrix<float>& a, const Matrix<float>& b);

/**
 * @brief Checks if two vectors are equal
 * @param a First vector
 * @param b Second vector
 * @return True if vectors have the same size and equal elements
 */
bool operator==(const std::vector<float>& a, const std::vector<float>& b);

// Basic vector functions

/**
 * @brief Calculates the absolute value of each element in a vector
 * @param a The input vector
 * @return A new vector with absolute values
 */
std::vector<float> abs(const std::vector<float>& a);

/**
 * @brief Calculates the Euclidean norm (L2 norm) of a vector
 * @param a The input vector
 * @return The L2 norm
 */
float norm2(const std::vector<float>& a);

/**
 * @brief Calculates the weighted Euclidean norm of a vector
 * @param a The input vector
 * @param factor The weight vector
 * @return The weighted L2 norm
 * @throws std::invalid_argument if a's size exceeds factor's size
 */
float norm2(const std::vector<float>& a, const std::vector<float>& factor);

} // namespace chiplet