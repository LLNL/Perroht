// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

/// \brief Run insert benchmark reading dataset from file

#include <getopt.h>
#include <iostream>
#include <string>
#include <fstream>

#include <metall/metall.hpp>
#include <metall/container/unordered_map.hpp>

#include "bench_common.hpp"
#include "dataset_generator.hpp"

enum class DataType {
  kInvalid = -1,
  kInt64 = 0,
  kString = 1,
};

template <typename MapType>
double InsertItems(const std::string& input_file_path,
                   const std::size_t batch_size, MapType& map) {
  using KeyType = typename MapType::key_type;

  std::vector<KeyType> keys(batch_size);
  std::ifstream ifs(input_file_path);
  if (!ifs) {
    std::cerr << "Failed to open " << input_file_path << std::endl;
    std::abort();
  }

  double total_elapsed_time = 0.0;
  while (true) {
    std::size_t num_reads = 0;
    for (; num_reads < batch_size; ++num_reads) {
      std::string buf;
      if (!std::getline(ifs, buf)) {
        break;
      }
      std::stringstream ss(buf);
      ss >> keys[num_reads];
    }
    if (num_reads == 0) {
      break;
    }

    const auto start_time = perroht::time::Start();
    for (std::size_t i = 0; i < num_reads; ++i) {
      map[keys[i]];
    }
    total_elapsed_time += perroht::time::GetDuration(start_time);
  }

  return total_elapsed_time;
}

template <typename DataType>
void RunBench(const std::size_t num_repeats, const std::size_t batch_size,
              const std::string& input_file_path) {
  RunBenchmark(
      num_repeats,
      [&]() {
        std::unordered_map<DataType, DataType> map;
        return InsertItems(input_file_path, batch_size, map);
      },
      true, "Insert-STL");

  RunBenchmark(
      num_repeats,
      [&]() {
        PerrohtMap<DataType, DataType> map;
        return InsertItems(input_file_path, batch_size, map);
      },
      true, "Insert-Perroht");
}

template <typename DataType>
void RunBenchMetall(const std::size_t num_repeats, const std::size_t batch_size,
                    const std::string& input_file_path,
                    const std::string& data_store_path) {
  RunBenchmark(
      num_repeats,
      [&]() {
        metall::manager manager(metall::create_only, data_store_path.c_str());
        auto* map = manager.construct<
            metall::container::unordered_map<DataType, DataType>>("map")(
            manager.get_allocator());
        return InsertItems(input_file_path, batch_size, *map);
      },
      true, "Insert-Metall-STL");

  RunBenchmark(
      num_repeats,
      [&]() {
        metall::manager manager(metall::create_only, data_store_path.c_str());
        auto* map = manager.construct<PerrohtMapMetall<DataType, DataType>>(
            "map")(manager.get_allocator());
        return InsertItems(input_file_path, batch_size, *map);
      },
      true, "Insert-Metall-Perroht");
}

// parse CLI arguments using getopt
bool ParseOptions(int argc, char* argv[], std::size_t& num_repeats,
                  std::size_t& batch_size, std::string& input_file_path,
                  DataType& data_type, std::string& data_store_path) {
  int c;
  while ((c = getopt(argc, argv, "n:b:i:t:d:")) != -1) {
    switch (c) {
      case 'n':
        num_repeats = std::stoull(optarg);  // option
        break;

      case 'b':
        batch_size = std::stoull(optarg);  // option
        break;

      case 'i':
        input_file_path = optarg;  // option
        break;

      case 't': {
        const auto n = std::stoi(optarg);  // Required
        data_type = static_cast<DataType>(n);
        break;
      }

      case 'd':
        data_store_path = optarg;  // option
        break;

      default:
        std::cerr << "Invalid option" << std::endl;
        return false;
    }
  }

  if (data_type == DataType::kInvalid) {
    std::cerr << "Data type is invalid or not given" << std::endl;
    return false;
  }

  return true;
}

int main(int argc, char* argv[]) {
  std::size_t num_repeats = 5;
  std::size_t batch_size = 1000000;
  std::string input_file_path = "./insert-dataset.txt";
  DataType data_type = DataType::kInvalid;
  std::string data_store_path = "";
  ParseOptions(argc, argv, num_repeats, batch_size, input_file_path, data_type,
               data_store_path);

  if (data_type == DataType::kInt64) {
    RunBench<uint64_t>(num_repeats, batch_size, input_file_path);
  } else if (data_type == DataType::kString) {
    RunBench<std::string>(num_repeats, batch_size, input_file_path);
  } else {
    std::cerr << "Invalid data type" << std::endl;
    return 1;
  }

  if (!data_store_path.empty()) {
    if (data_type == DataType::kInt64) {
      RunBenchMetall<uint64_t>(num_repeats, batch_size, input_file_path,
                               data_store_path);
    } else if (data_type == DataType::kString) {
      assert(false);  // TODO: change to use boost::string
      RunBenchMetall<std::string>(num_repeats, batch_size, input_file_path,
                                  data_store_path);
    } else {
      std::cerr << "Invalid data type" << std::endl;
      return 1;
    }
  }

  return EXIT_SUCCESS;
}