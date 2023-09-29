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
  kMixedInt = 1,
  kRandomString = 2,
  kMixedString = 3,
};

// Parse CLI arguments using getopt
bool parse_options(int argc, char* argv[], Mode& mode,
                   unsigned int& random_seed, std::size_t& num_operations,
                   double& erase_ratio, std::size_t& string_length,
                   std::string& erase_file_name) {
  int c;
  while ((c = getopt(argc, argv, "m:n:r:e:l:o:")) != -1) {
    switch (c) {
      case 'm': {
        const auto n = std::stoi(optarg);
        mode = static_cast<Mode>(n);
        break;
      }

      case 'n':
        num_operations = std::stoull(optarg);  // required
        break;

      case 'r':
        random_seed = std::stoull(optarg);  // option
        break;

      case 'e':
        erase_ratio = std::stod(optarg);  // option
        break;

      case 'l':
        string_length = std::stoull(optarg);  // option
        break;

      case 'o':
        erase_file_name = optarg;  // option
        break;

      case '?':
        std::cerr << "Unknown option: " << static_cast<char>(optopt)
                  << std::endl;
        return false;

      default:
        std::cerr << "Unknown error while parsing options" << std::endl;
        return false;
    }
  }

  if (mode == Mode::kInvalid) {
    std::cerr << "Invalid mode" << std::endl;
    return false;
  }

  if (erase_ratio < 0.0 || erase_ratio > 1.0) {
    std::cerr << "Erase ratio must be in [0.0, 1.0]" << std::endl;
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
  std::size_t num_operations = 0;
  double erase_ratio{0.2};
  std::size_t string_length = 8;
  std::string erase_file_name = "./erase-dataset.txt";

  if (!parse_options(argc, argv, mode, random_seed, num_operations, erase_ratio,
                     string_length, erase_file_name)) {
    return EXIT_FAILURE;
  }
  std::mt19937_64 rg(random_seed);

  if (mode == Mode::kRandomInt) {
    std::vector<std::pair<uint64_t, bool>> vec;
    GenEraseIntDataset(num_operations, erase_ratio, vec, rg);
    DumpToFile(erase_file_name, vec);
  } else if (mode == Mode::kMixedInt) {
    std::vector<std::pair<uint64_t, bool>> vec;
    GenMixedEraseIntDataset(num_operations, erase_ratio, vec, rg);
    DumpToFile(erase_file_name, vec);
  } else if (mode == Mode::kRandomString) {
    std::vector<std::pair<std::string, bool>> vec;
    GenEraseStringDataset(num_operations, erase_ratio, string_length, vec, rg);
    DumpToFile(erase_file_name, vec);
  } else if (mode == Mode::kMixedString) {
    std::vector<std::pair<std::string, bool>> vec;
    GenMixedEraseStringDataset(num_operations, erase_ratio, string_length, vec,
                               rg);
    DumpToFile(erase_file_name, vec);
  } else {
    std::cerr << "Invalid mode" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Done" << std::endl;
  return EXIT_SUCCESS;
}