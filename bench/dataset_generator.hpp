// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <string>
#include <algorithm>
#include <random>
#include <fstream>
#include <cassert>
#include <sstream>
#include <vector>
#include <queue>

#include <perroht/utilities/hash.hpp>

inline std::string DToS(const double d) {
  std::string str = std::to_string(d);
  str.erase(str.find_last_not_of('0') + 1, std::string::npos);
  str.erase(str.find_last_not_of('.') + 1, std::string::npos);
  return str;
}

template <typename RandomGenerator>
inline std::string GenRandomString(const std::size_t string_length,
                                   RandomGenerator& rng) {
  static const std::string kCharList{
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"};

  std::string str(string_length, 'x');
  for (std::size_t i = 0; i < string_length; ++i) {
    str[i] = kCharList[rng() % kCharList.size()];
  }
  return str;
}

// Generate random int dataset
template <typename IntType, typename RandGenerator>
inline void GenRandomInts(const std::size_t n, const double duplicate_ratios,
                          std::vector<IntType>& vec, RandGenerator& rng) {
  vec.reserve(n);
  const std::size_t num_original = n * (1.0 - duplicate_ratios);
  for (std::size_t i = 0; i < num_original; ++i) {
    vec.push_back(rng());
  }

  // add duplicate keys
  for (std::size_t i = num_original; i < n; ++i) {
    vec.push_back(vec[rng() % num_original]);
  }

  // shuffle
  std::shuffle(vec.begin(), vec.end(), rng);

  assert(vec.size() == n);
}

// Generate random int dataset with skewed distribution
template <typename IntType, typename RandGenerator,
          unsigned int HashSeed = 24747>
inline void GenSkewedRandomInts(const std::size_t n, const double mean,
                                std::vector<IntType>& vec, RandGenerator& rng,
                                const bool scramble = true) {
  std::poisson_distribution<IntType> dist(mean);
  vec.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    const auto v = dist(rng);
    if (scramble) {
      vec.push_back(perroht::Hash<IntType, HashSeed>{}(v));
    } else {
      vec.push_back(v);
    }
  }

  std::shuffle(vec.begin(), vec.end(), rng);
}

// Generate random string dataset
template <typename RandGenerator>
inline void GenRandomStrings(const std::size_t n,
                             const std::size_t string_length,
                             const double duplicate_ratios,
                             std::vector<std::string>& vec,
                             RandGenerator& rng) {
  vec.reserve(n);
  const std::size_t num_original = n * (1.0 - duplicate_ratios);
  for (std::size_t i = 0; i < num_original; ++i) {
    vec.push_back(GenRandomString(string_length, rng));
  }

  // add duplicate keys
  for (std::size_t i = num_original; i < n; ++i) {
    vec.push_back(vec[rng() % num_original]);
  }

  std::shuffle(vec.begin(), vec.end(), rng);
  assert(vec.size() == n);
}

// Generate random string dataset with skewed distribution
template <typename RandGenerator>
inline void GenSkewedRandomStrings(const std::size_t n,
                                   const std::size_t string_length,
                                   const double mean,
                                   std::vector<std::string>& vec,
                                   RandGenerator& rng) {
  std::poisson_distribution<std::size_t> dist(mean);
  vec.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    const auto v = dist(rng);
    auto r = std::mt19937_64(v);
    vec.push_back(GenRandomString(string_length, r));
  }

  // add duplicate keys
  for (std::size_t i = n; i < vec.size(); ++i) {
    vec[i] = vec[rng() % n];
  }

  std::shuffle(vec.begin(), vec.end(), rng);
}

// Generate find dataset from insert dataset
template <typename T, typename RandGenerator>
void GenFindDataset(const std::size_t num_finds, const double hit_rate,
                    const std::vector<T>& in_vec, std::vector<T>& out_vec,
                    RandGenerator& rng) {
  out_vec.insert(out_vec.end(), in_vec.begin(), in_vec.end());
  std::shuffle(out_vec.begin(), out_vec.end(), rng);

  const std::size_t num_hits = num_finds * hit_rate;
  out_vec.resize(num_hits);

  // Generate non-hit keys
  // Does not check duplicates for simplicity,
  // assuming that the random generator is good enough.
  for (std::size_t i = num_hits; i < num_finds; ++i) {
    if constexpr (std::is_same_v<T, std::string>) {
      out_vec.push_back(GenRandomString(in_vec[0].size(), rng));
    } else {
      out_vec.push_back(rng());
    }
  }
  std::shuffle(out_vec.begin(), out_vec.end(), rng);
}

// Generate erase dataset from insert dataset
/// @param vec vector to store generated items. An item is a pair of value and
/// bool. If bool is false, it's an insert request. If true, it's an erase.
template <typename T, typename RandGenerator>
void GenEraseIntDataset(const std::size_t num_operations,
                        const double erase_ratio,
                        std::vector<std::pair<T, bool>>& out_vec,
                        RandGenerator& rng) {
  const std::size_t num_erases = num_operations * erase_ratio;
  const std::size_t num_inserts = num_operations - num_erases;

  out_vec.reserve(num_operations);
  for (std::size_t i = 0; i < num_inserts; ++i) {
    out_vec.emplace_back(rng(), false);
  }

  for (std::size_t i = num_inserts; i < num_operations; ++i) {
    out_vec.emplace_back(out_vec[rng() % num_inserts].first, true);
  }
}

template <typename RandGenerator>
void GenEraseStringDataset(const std::size_t num_operations,
                           const double erase_ratio,
                           const std::size_t string_length,
                           std::vector<std::pair<std::string, bool>>& out_vec,
                           RandGenerator& rng) {
  const std::size_t num_erases = num_operations * erase_ratio;
  const std::size_t num_inserts = num_operations - num_erases;

  out_vec.reserve(num_operations);
  for (std::size_t i = 0; i < num_inserts; ++i) {
    out_vec.emplace_back(GenRandomString(string_length, rng), false);
  }

  for (std::size_t i = num_inserts; i < num_operations; ++i) {
    out_vec.emplace_back(out_vec[rng() % num_inserts].first, true);
  }
}

// Generate insert and erase mixed dataset
// Erase requests are generated making sure they are inserted beforehand.
// Generated item's not exactly the same as 'erase_ratio', but this algorithm is
// simple and good enough for the purpose of benchmarking.
/// @param vec vector to store generated items. An item is a pair of value and
/// bool. If bool is false, it's an insert request. If true, it's an erase.
template <typename T, typename RandGenerator, unsigned int HashSeed = 24747>
void GenMixedEraseIntDataset(const std::size_t num_operations,
                              const double erase_ratio,
                              std::vector<std::pair<T, bool>>& vec,
                              RandGenerator& rng) {
  const std::size_t num_erases = num_operations * erase_ratio;
  const std::size_t num_inserts = num_operations - num_erases;

  std::vector<T> tmp_vec(num_inserts);
  for (std::size_t i = 0; i < num_inserts; ++i) {
    tmp_vec[i] = rng();
  }

  std::priority_queue<std::pair<uint64_t, std::size_t>,
                      std::vector<std::pair<uint64_t, std::size_t>>,
                      std::greater<>>
      erase_queue;
  vec.clear();
  vec.reserve(num_operations);
  std::size_t insert_pos = 0;

  for (std::size_t i = 0; i < num_operations; ++i) {
    const bool insert =
        (rng() % 100 > erase_ratio * 100) || erase_queue.empty();
    if (insert) {
      const auto& elem = tmp_vec[insert_pos];
      vec.emplace_back(elem, false);  // insert request
      erase_queue.emplace(perroht::Hash<T, HashSeed>{}(elem), insert_pos);
      ++insert_pos;
    } else {
      vec.emplace_back(vec[erase_queue.top().second].first,
                       true);  // erase request
      erase_queue.pop();
    }
  }
}

template <typename RandGenerator, unsigned int HashSeed = 24747>
void GenMixedEraseStringDataset(const std::size_t num_operations,
                                const double erase_ratio,
                                const std::size_t string_length,
                                std::vector<std::pair<std::string, bool>>& vec,
                                RandGenerator& rng) {
  const std::size_t num_erases = num_operations * erase_ratio;
  const std::size_t num_inserts = num_operations - num_erases;

  std::vector<std::string> tmp_vec(num_inserts);
  for (std::size_t i = 0; i < num_inserts; ++i) {
    tmp_vec[i] = GenRandomString(string_length, rng);
  }

  std::priority_queue<std::pair<uint64_t, std::size_t>,
                      std::vector<std::pair<uint64_t, std::size_t>>,
                      std::greater<>>
      erase_queue;
  vec.clear();
  vec.reserve(num_operations);
  std::size_t insert_pos = 0;

  for (std::size_t i = 0; i < num_operations; ++i) {
    const bool insert =
        (rng() % 100 > erase_ratio * 100) || erase_queue.empty();
    if (insert) {
      const auto& elem = tmp_vec[insert_pos];
      vec.emplace_back(elem, false);  // insert request
      erase_queue.emplace(perroht::StringHash<std::string, HashSeed>{}(elem),
                          insert_pos);
      ++insert_pos;
    } else {
      vec.emplace_back(vec[erase_queue.top().second].first,
                       true);  // erase request
      erase_queue.pop();
    }
  }
}

// dump dataset to file
template <typename T>
inline void DumpToFile(const std::string& file_name,
                       const std::vector<T>& vec) {
  std::cout << "Dumping dataset to " << file_name << std::endl;
  std::ofstream ofs(file_name);
  for (const auto& item : vec) {
    ofs << item << '\n';
  }
}

// dump dataset to file
template <typename T1, typename T2>
inline void DumpToFile(const std::string& file_name,
                       const std::vector<std::pair<T1, T2>>& vec) {
  std::cout << "Dumping dataset to " << file_name << std::endl;
  std::ofstream ofs(file_name);
  for (const auto& item : vec) {
    ofs << item.first << " " << item.second << '\n';
  }
}