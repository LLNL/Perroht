// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <unordered_map>

#include <perroht/unordered_map.hpp>
#include <perroht/utilities/time.hpp>
#include <perroht/utilities/hash.hpp>

template <typename Key, typename MappedValue>
using PerrohtMap =
    perroht::unordered_flat_map<Key, MappedValue, perroht::Hash<Key>,
                                std::equal_to<Key>>;

template <typename Key, typename MappedValue>
using PerrohtMapMetall = perroht::unordered_flat_map<
    Key, MappedValue, perroht::Hash<Key>, std::equal_to<Key>,
    metall::manager::allocator_type<std::pair<const Key, MappedValue>>>;

struct Stats {
  double min;
  double mean;
  double max;
  double std_dev;
};
inline std::ostream& operator<<(std::ostream& os, const Stats& stats) {
  os << "(min mean max std-dev)\t" << stats.min << "\t" << stats.mean << "\t"
     << stats.max << "\t" << stats.std_dev;
  return os;
}

/// Benchmark driver
/// Runs the given benchmark function 'n' times and returns some statistics,
/// such as min, max, mean, and standard deviation.
/// \tparam FuncT The benchmark function type, which returns a execution time
/// \param n The number of times to run the benchmark
/// \param func The benchmark function
/// \param verbose If true, print the statistics.
/// \return A tuple of min, max, mean, and standard deviation
template <typename FuncT>
inline Stats RunBenchmark(const std::size_t n, FuncT&& func,
                          const bool verbose = true,
                          const std::string& name = "") {
  std::vector<double> times;
  times.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    times.push_back(func());
  }

  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::min();
  double sum = 0.0;
  for (const auto& t : times) {
    min = std::min(min, t);
    max = std::max(max, t);
    sum += t;
  }
  const double mean = sum / double(n);

  double sum_sq = 0.0;
  for (const auto& t : times) {
    sum_sq += (t - mean) * (t - mean);
  }
  const double std_dev = std::sqrt(sum_sq / double(n));

  const Stats ret{min, mean, max, std_dev};
  if (verbose) {
    if (!name.empty()) {
      std::cout << name << "\t";
    }
    std::cout << ret << std::endl;
  }
  return ret;
}
