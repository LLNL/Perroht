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
double FindItems(const std::string& insert_file_path,
                 const std::string& find_file_path,
                 const std::size_t batch_size, MapType& map) {
  using KeyType = typename MapType::key_type;

  // Insert items from file
  {
    std::ifstream ifs(insert_file_path);
    if (!ifs) {
      std::cerr << "Failed to open " << insert_file_path << std::endl;
      std::abort();
    }
    while (true) {
      std::string buf;
      if (!std::getline(ifs, buf)) {
        break;
      }
      std::stringstream ss(buf);
      KeyType key;
      ss >> key;
      map[key];
    }
  }

  std::vector<KeyType> keys(batch_size);
  std::ifstream ifs(find_file_path);
  if (!ifs) {
    std::cerr << "Failed to open " << find_file_path << std::endl;
    std::abort();
  }
  double total_elapsed_time = 0.0;
  std::size_t num_hits = 0;
  std::size_t num_total_reads = 0;
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
    num_total_reads += num_reads;
    const auto start_time = perroht::time::Start();
    for (std::size_t i = 0; i < num_reads; ++i) {
      num_hits += map.count(keys[i]);
    }
    total_elapsed_time += perroht::time::GetDuration(start_time);
  }
  // Simple way to prevent compilers from optimizing the benchmark loop above
  if (num_hits > num_total_reads) {
    std::cerr << "Error: num_hits > num_total_reads" << std::endl;
    std::abort();
  }
  return total_elapsed_time;
}

template <typename DataType>
void RunBench(const std::size_t num_repeats, const std::size_t batch_size,
              const std::string& insert_file_path,
              const std::string& find_file_path) {
  RunBenchmark(
      num_repeats,
      [&]() {
        std::unordered_map<DataType, DataType> map;
        return FindItems(insert_file_path, find_file_path, batch_size, map);
      },
      true, "Find-STL");

  RunBenchmark(
      num_repeats,
      [&]() {
        PerrohtMap<DataType, DataType> map;
        return FindItems(insert_file_path, find_file_path, batch_size, map);
      },
      true, "Find-Perroht");
}

template <typename DataType>
void RunBenchMetall(const std::size_t num_repeats, const std::size_t batch_size,
                    const std::string& insert_file_path,
                    const std::string& find_file_path,
                    const std::string& data_store_path) {
  RunBenchmark(
      num_repeats,
      [&]() {
        metall::manager manager(metall::create_only, data_store_path.c_str());
        auto* map = manager.construct<
            metall::container::unordered_map<DataType, DataType>>("map")(
            manager.get_allocator());
        return FindItems(insert_file_path, find_file_path, batch_size, *map);
      },
      true, "Find-Metall-STL");

  RunBenchmark(
      num_repeats,
      [&]() {
        metall::manager manager(metall::create_only, data_store_path.c_str());
        auto* map = manager.construct<PerrohtMapMetall<DataType, DataType>>(
            "map")(manager.get_allocator());
        return FindItems(insert_file_path, find_file_path, batch_size, *map);
      },
      true, "Find-Metall-Perroht");
}

// parse CLI arguments using getopt
bool ParseOptions(int argc, char* argv[], std::size_t& num_repeats,
                  std::size_t& batch_size, std::string& insert_file_path,
                  std::string& find_file_path, DataType& data_type,
                  std::string& data_store_path) {
  int c;
  while ((c = getopt(argc, argv, "n:b:i:f:t:d:")) != -1) {
    switch (c) {
      case 'n':
        num_repeats = std::stoull(optarg);  // option
        break;

      case 'b':
        batch_size = std::stoull(optarg);  // option
        break;

      case 'i':
        insert_file_path = optarg;  // option
        break;

      case 'f':
        find_file_path = optarg;  // option
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
  std::string insert_file_path = "./insert-dataset.txt";
  std::string find_file_path = "./find-dataset.txt";
  DataType data_type = DataType::kInvalid;
  std::string data_store_path = "";
  ParseOptions(argc, argv, num_repeats, batch_size, insert_file_path,
               find_file_path, data_type, data_store_path);

  if (data_type == DataType::kInt64) {
    RunBench<uint64_t>(num_repeats, batch_size, insert_file_path,
                       find_file_path);
  } else if (data_type == DataType::kString) {
    RunBench<std::string>(num_repeats, batch_size, insert_file_path,
                          find_file_path);
  } else {
    std::cerr << "Invalid data type" << std::endl;
    return 1;
  }

  if (!data_store_path.empty()) {
    if (data_type == DataType::kInt64) {
      RunBenchMetall<uint64_t>(num_repeats, batch_size, insert_file_path,
                               find_file_path, data_store_path);
    } else if (data_type == DataType::kString) {
      RunBenchMetall<std::string>(num_repeats, batch_size, insert_file_path,
                                  find_file_path, data_store_path);
    } else {
      std::cerr << "Invalid data type" << std::endl;
      return 1;
    }
  }

  return EXIT_SUCCESS;
}