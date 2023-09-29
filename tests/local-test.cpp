// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <iostream>
#include <unordered_set>
#include <cassert>
#include <random>
#include <string>
#include <utility>
#include <cstdlib>
#include <functional>

#include <perroht/unordered_set.hpp>

int main(int argc, char** argv) {
  const std::size_t num_operations = 1 << 20;
  const double insert_ratio = 0.9;
  const std::size_t num_insertions = num_operations * insert_ratio;
  const std::size_t num_erasures = num_operations - num_insertions;

  std::mt19937 rng(123);

  using M = std::unordered_set<uint64_t>;
  using N = perroht::unordered_node_set<uint64_t>;
  M std_set;
  N prh_set;
  for (size_t i = 0; i < num_operations; ++i) {
    const auto v = rng() % num_operations;
    if ((rng() % (num_insertions + num_erasures)) < num_insertions) {
      std_set.insert(v);
      prh_set.insert(v);
    } else {
      std_set.erase(v);
      prh_set.erase(v);
    }
  }

  for (const auto& v : std_set) {
    assert(prh_set.find(v) != prh_set.end());
  }

  for (const auto& v : prh_set) {
    assert(std_set.find(v) != std_set.end());
  }

  std::cout << "Copy" << std::endl;
  N prh_set2;
  for (const auto& v : prh_set) {
    prh_set2.insert(v);
  }

  return EXIT_SUCCESS;
}
