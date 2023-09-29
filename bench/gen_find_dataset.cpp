// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

/// \brief Generate datasets for insertion benchmark

#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <cassert>

#include "dataset_generator.hpp"

enum class Mode {
  kInvalid = -1,
  kRandomInt = 0,
  kRandomString = 1,
};

// Parse CLI arguments using getopt
bool parse_options(int argc, char* argv[], unsigned int& random_seed,
                   std::size_t& num_inserts, std::size_t& num_finds,
                   double& hit_rate, Mode& mode,
                   std::size_t& string_length, std::string& insert_file_name, std::string& find_file_name) {
  int c;
  while ((c = getopt(argc, argv, "m:n:k:f:h:l:i:r:")) != -1) {
    switch (c) {
      case 'm': {
        const auto n = std::stoi(optarg);
        mode = static_cast<Mode>(n);
        break;
      }

      case 'n':
        num_inserts = std::stoull(optarg);  // required
        break;

      case 'k':
        num_finds = std::stoull(optarg);  // required
        break;

      case 'i':
        insert_file_name = optarg;  // option
        break;

      case 'f':
        find_file_name = optarg;  // option
        break;

      case 'r':
        random_seed = std::stoull(optarg);  // option
        break;

      case 'h':
        hit_rate = std::stod(optarg);  // option
        break;

      case 'l':
        string_length = std::stoull(optarg);  // option
        break;

      case '?':
        std::cerr << "Unknown option: " << char(optopt) << std::endl;
        return false;

      default:
        std::cerr << "Unexpected error" << std::endl;
        return false;
    }
  }

  if (mode == Mode::kInvalid) {
    std::cerr << "Mode is required" << std::endl;
    return false;
  }

  if (num_inserts == 0) {
    std::cerr << "Number of insert items is required" << std::endl;
    return false;
  }

  if (num_finds == 0) {
    std::cerr << "Number of find items is required" << std::endl;
    return false;
  }

  if (hit_rate < 0.0 || hit_rate > 1.0) {
    std::cerr << "Hit ratio must be in [0.0, 1.0]" << std::endl;
    return false;
  }

  if (string_length == 0) {
    std::cerr << "String length must be positive" << std::endl;
    return false;
  }

  return true;
}

int main(int argc, char* argv[]) {
  Mode mode{Mode::kInvalid};
  unsigned int random_seed = std::random_device{}();
  std::size_t num_inserts = 0;
  std::size_t num_finds = 0;
  double hit_rate{1.0};
  std::size_t string_length = 8;
  std::string insert_file_name = "./insert-dataset.txt";
  std::string find_file_name = "./find-dataset.txt";

  if (!parse_options(argc, argv, random_seed, num_inserts, num_finds, hit_rate,
                     mode, string_length, insert_file_name, find_file_name)) {
    return EXIT_FAILURE;
  }
  std::mt19937_64 rg(random_seed);

  if (mode == Mode::kRandomInt) {
    std::vector<uint64_t> in_vec;
    GenRandomInts(num_inserts, 0, in_vec, rg);
    DumpToFile(insert_file_name, in_vec);

    std::vector<uint64_t> find_vec;
    GenFindDataset(num_finds, hit_rate, in_vec, find_vec, rg);
    DumpToFile(find_file_name, find_vec);
  } else if (mode == Mode::kRandomString) {
    std::vector<std::string> in_vec;
    GenRandomStrings(num_inserts, string_length, hit_rate, in_vec, rg);
    DumpToFile(insert_file_name, in_vec);

    std::vector<std::string> find_vec;
    GenFindDataset(num_finds, hit_rate, in_vec, find_vec, rg);
    DumpToFile(find_file_name, find_vec);
  } else {
    std::cerr << "Invalid mode" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Done" << std::endl;
  return EXIT_SUCCESS;
}