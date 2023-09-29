// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <chrono>

namespace perroht::time {

inline auto Start() { return std::chrono::high_resolution_clock::now(); }

inline double GetDuration(const decltype(Start())& start_time) {
  return double(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start_time)
                    .count()) /
         1000.0;
}
}  // namespace perroht::time