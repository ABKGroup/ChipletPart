///////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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
#pragma once

// Include OpenMP if available
#if HAVE_OPENMP
#include <omp.h>

// OpenMP utility functions
namespace omp_utils {

/**
 * Get the maximum number of threads available for OpenMP
 */
inline int get_max_threads() {
  return omp_get_max_threads();
}

/**
 * Get the current thread ID (0-based)
 */
inline int get_thread_num() {
  return omp_get_thread_num();
}

/**
 * Get the number of threads in the current parallel region
 */
inline int get_num_threads() {
  return omp_get_num_threads();
}

/**
 * Set the number of threads to use in parallel regions
 */
inline void set_num_threads(int num_threads) {
  omp_set_num_threads(num_threads);
}

/**
 * Check if code is running in a parallel region
 */
inline bool in_parallel() {
  return omp_in_parallel() != 0;
}

/**
 * Get the wall clock time in seconds
 */
inline double get_wtime() {
  return omp_get_wtime();
}

} // namespace omp_utils

#else // !HAVE_OPENMP - Fallback implementations for when OpenMP is not available

#include <chrono>
#include <thread>

// Fallback implementations
namespace omp_utils {

/**
 * Get the maximum number of threads available (fallback uses hardware concurrency)
 */
inline int get_max_threads() {
  return std::thread::hardware_concurrency();
}

/**
 * Get the current thread ID (always 0 in single-threaded mode)
 */
inline int get_thread_num() {
  return 0;
}

/**
 * Get the number of threads (always 1 in single-threaded mode)
 */
inline int get_num_threads() {
  return 1;
}

/**
 * Set the number of threads (no-op in single-threaded mode)
 */
inline void set_num_threads(int /*num_threads*/) {
  // No-op
}

/**
 * Check if code is running in a parallel region (always false in single-threaded mode)
 */
inline bool in_parallel() {
  return false;
}

/**
 * Get the wall clock time in seconds (fallback using std::chrono)
 */
inline double get_wtime() {
  using clock = std::chrono::high_resolution_clock;
  static const auto start_time = clock::now();
  const auto current_time = clock::now();
  const auto elapsed = std::chrono::duration<double>(current_time - start_time);
  return elapsed.count();
}

} // namespace omp_utils

#endif // HAVE_OPENMP 