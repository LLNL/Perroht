// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <iostream>
#include <unordered_map>
#include <cassert>
#include <random>
#include <string>
#include <utility>
#include <cstdlib>

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
#include <metall/metall.hpp>
#include <metall/container/string.hpp>
#include <metall/utility/hash.hpp>
#endif

#include <perroht/unordered_map.hpp>

// generate a random string of length string_length
std::string GenRandomString(const std::size_t string_length,
                            std::mt19937& rng) {
  static constexpr const char* kCharList =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::string str(string_length, 'x');
  for (std::size_t i = 0; i < string_length; ++i) {
    str[i] = kCharList[rng() % (sizeof(kCharList) - 1)];
  }
  return str;
}

template <typename Map>
void ConstMap(const std::size_t num_operations,
              const std::size_t num_insertions, const std::size_t string_length,
              Map& map) {
  std::mt19937 rng(123);
  std::mt19937 op_rng(321);
  const auto num_erases = num_operations - num_insertions;

  std::vector<std::pair<std::string, bool>> str_vec;
  for (std::size_t i = 0; i < num_insertions; ++i) {
    const auto str = GenRandomString(string_length, rng);
    str_vec.push_back(std::make_pair(str, true));
    if (op_rng() % num_operations < num_erases) {
      str_vec.push_back(std::make_pair(str, false));
    }
  }
  std::shuffle(str_vec.begin(), str_vec.end(), rng);

  for (const auto& item : str_vec) {
    const auto key =
        typename Map::key_type(item.first.c_str(), map.get_allocator());
    if (item.second) {
      map.emplace(key, "testing");
    } else {
      map.erase(key);
    }
  }
}

template <typename Map, typename Nap>
void Check(Map& map, Nap& nap) {
  for (const auto& item : map) {
    const auto key =
        typename Nap::key_type(item.first.c_str(), nap.get_allocator());
    if (!nap.count(key)) {
      std::cerr << __LINE__ << " Error: " << key << " not found" << std::endl;
    }
    if (nap.at(key).compare(item.second.c_str())) {
      std::cerr << __LINE__ << " Error: " << nap[key] << " != " << item.second
                << std::endl;
    }
  }
  for (const auto& item : nap) {
    const auto key =
        typename Map::key_type(item.first.c_str(), map.get_allocator());
    if (!map.count(key)) {
      std::cerr << __LINE__ << " Error: " << key << " not found" << std::endl;
    }
    if (map.at(key).compare(item.second.c_str())) {
      std::cerr << __LINE__ << " Error: " << map[key] << " != " << item.second
                << std::endl;
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr
        << "Usage: " << argv[0]
        << " [# of operations] [# of insertions ratio <= 1.0] [string length]"
        << std::endl;
    return EXIT_FAILURE;
  }

  const std::size_t num_operations = std::stoull(argv[1]);
  const std::size_t num_insertions = std::stod(argv[2]) * num_operations;
  const std::size_t string_length = std::stoull(argv[3]);

  {
    using M =
        std::unordered_map<std::string, std::string, std::hash<std::string>>;
    using N = perroht::unordered_flat_map<std::string, std::string,
                                          std::hash<std::string>>;
    M map;
    ConstMap(num_operations, num_insertions, string_length, map);

    N nap;
    ConstMap(num_operations, num_insertions, string_length, nap);

    Check(map, nap);
  }

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  {
    static constexpr const char* kMetallDataStorePath = "./test-unordered-map";
    using M =
        std::unordered_map<std::string, std::string, std::hash<std::string>>;
    using N = perroht::unordered_flat_map<
        metall::container::string, metall::container::string,
        metall::utility::string_hash<metall::container::string>,
        std::equal_to<>, metall::manager::scoped_allocator_type<std::string>>;

    {
      M map;
      ConstMap(num_operations, num_insertions, string_length, map);

      metall::manager manager(metall::create_only, kMetallDataStorePath);
      auto* nap = manager.construct<N>("nap")(manager.get_allocator<>());
      ConstMap(num_operations, num_insertions, string_length, *nap);

      Check(map, *nap);
    }

    {
      M map;
      ConstMap(num_operations, num_insertions, string_length, map);

      metall::manager manager(metall::open_only, kMetallDataStorePath);
      auto* nap = manager.find<N>("nap").first;

      Check(map, *nap);
    }
  }
#endif

  std::cout << "Success" << std::endl;

  return EXIT_SUCCESS;
}