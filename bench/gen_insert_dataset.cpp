// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

/// \brief Generate datasets for insertion benchmark

#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <random>

#include "dataset_generator.hpp"

enum class Mode {
  kInvalid = -1,
  kRandomInt = 0,
  kSkewedInt = 1,
  kRandomString = 2,
  kSkewedString = 3,
};

// Parse CLI arguments using getopt
bool parse_options(int argc, char* argv[], unsigned int& random_seed,
                   std::size_t& num_total, double& duplicate_ratio,
                   double& skewed_mean, Mode& mode, std::size_t& string_length,
                   std::string& output_file_name) {
  int c;
  while ((c = getopt(argc, argv, "m:n:d:s:l:i:r:")) != -1) {
    switch (c) {
      case 'm': {
        const auto n = std::stoi(optarg);
        mode = static_cast<Mode>(n);
        break;
      }

      case 'n':
        num_total = std::stoull(optarg);  // required
        break;

      case 'i':
        output_file_name = optarg;  // option
        break;

      case 'r':
        random_seed = std::stoull(optarg);  // option
        break;

      case 'd':
        duplicate_ratio = std::stod(optarg);  // option
        break;

      case 's':
        skewed_mean = std::stod(optarg);  // option
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

  if (num_total == 0) {
    std::cerr << "Number of total items is required" << std::endl;
    return false;
  }

  return true;
}

int main(int argc, char* argv[]) {
  Mode mode{Mode::kInvalid};
  unsigned int random_seed = std::random_device{}();
  std::size_t num_total = 0;
  double duplicate_ratio{0.0};
  double skewed_mean = 4;
  std::size_t string_length = 8;
  std::string output_file_name = "./insert-dataset.txt";

  if (!parse_options(argc, argv, random_seed, num_total, duplicate_ratio,
                     skewed_mean, mode, string_length, output_file_name)) {
    return EXIT_FAILURE;
  }

  // show options
  std::cout << "Options:" << std::endl;
  std::cout << "  mode: " << static_cast<int>(mode) << std::endl;
  std::cout << "  random seed: " << random_seed << std::endl;
  std::cout << "  number of total items: " << num_total << std::endl;
  std::cout << "  skewed mean: " << skewed_mean << std::endl;
  std::cout << "  string length: " << string_length << std::endl;
  std::cout << "  output file name: " << output_file_name << std::endl;
  std::cout << "  duplicate ratios:" << duplicate_ratio << std::endl;

  std::mt19937_64 rg(random_seed);

  if (mode == Mode::kRandomInt) {
    std::vector<uint64_t> vec;
    GenRandomInts(num_total, duplicate_ratio, vec, rg);
    DumpToFile(output_file_name, vec);
  } else if (mode == Mode::kSkewedInt) {
    std::vector<uint64_t> vec;
    GenSkewedRandomInts(num_total, skewed_mean, vec, rg);
    DumpToFile(output_file_name, vec);
  } else if (mode == Mode::kRandomString) {
    std::vector<std::string> vec;
    GenRandomStrings(num_total, string_length, duplicate_ratio, vec, rg);
    DumpToFile(output_file_name, vec);
  } else if (mode == Mode::kSkewedString) {
    std::vector<std::string> vec;
    GenSkewedRandomStrings(num_total, string_length, skewed_mean, vec, rg);
    DumpToFile(output_file_name, vec);
  } else {
    std::cerr << "Invalid mode" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Done" << std::endl;
  return EXIT_SUCCESS;
}