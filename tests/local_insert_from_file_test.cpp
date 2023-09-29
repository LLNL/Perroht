#include <iostream>
#include <fstream>
#include <vector>
#include <perroht/perroht.hpp>

/// this program reads value (single column) from a file and insert them into
/// a hash table. The hash table is implemented in perroht/perroht.hpp

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " [file name]" << std::endl;
    return EXIT_FAILURE;
  }

  std::vector<int64_t> values;
  {
    std::ifstream infile(argv[1]);
    int64_t value;
    while (infile >> value) {
      values.push_back(value);
    }
  }
  std::cout << "Read " << values.size() << " values from file" << std::endl;

  perroht::Perroht<int64_t, int64_t> ht;
  std::unordered_map<int64_t, int64_t> umap;
  for (std::size_t i = 0; i < values.size(); ++i) {
    ht.Insert(std::make_pair(values[i], i));
    umap.insert(std::make_pair(values[i], i));
    if (i % (values.size() / 10) == 0) {
      const auto ret = ht.GetProbeDistanceStats();
      std::size_t max_bucket_size = 0;
      for (std::size_t j = 0; j < umap.bucket_count(); ++j) {
        max_bucket_size = std::max(max_bucket_size, umap.bucket_size(j));
      }
      std::cout << i << " " << std::get<0>(ret) << " " << std::get<1>(ret)
                << " " << ht.GetApproximateMeanProbeDistance() << " "
                << std::get<2>(ret) <<  " | " << ht.LoadFactor() << std::endl;
    }
  }
  //  for (std::size_t i = 0; i < 100; ++i) {
  //    std::cout << values[i] % (ht.Capacity() * 2) << std::endl;
  //  }
  //  std::size_t max_bucket_size = 0;
  //  for (std::size_t j = 0; j < umap.bucket_count(); ++j) {
  //    // max_bucket_size = std::max(max_bucket_size, umap.bucket_size(j));
  //    std::cout << umap.bucket_size(j) << std::endl;
  //  }
  {
    const auto ret = ht.GetProbeDistanceHistogram();
    for (std::size_t i = 0; i < ret.size(); ++i) {
      if (ret[i] > 0) std::cout << i << " " << ret[i] << std::endl;
    }
  }
  {
    const auto ret = ht.GetProbeDistanceStats();
    std::cout << "" << std::get<0>(ret) << " " << std::get<1>(ret) << " "
              << std::get<2>(ret) << std::endl;
  }
    //  std::cout << ht.Capacity() << std::endl;
//  ht.Reserve(ht.Capacity() * 2);
//  {
//    std::cout << " ------- " << std::endl;
//    const auto ret = ht.GetProbeDistanceHistogram();
//    for (std::size_t i = 0; i < ret.size(); ++i) {
//      if (ret[i] > 0) std::cout << i << " " << ret[i] << std::endl;
//    }
//  }
//  {
//    const auto ret = ht.GetProbeDistanceStats();
//    std::cout << "" << std::get<0>(ret) << " " << std::get<1>(ret) << " "
//              << std::get<2>(ret) << std::endl;
//  }
  //  std::cout << ht.Capacity() << std::endl;
  //    std::cout << "Reserve " << std::get<0>(ret) << " " << std::get<1>(ret)
  //              << " " << std::get<2>(ret) << " " << ht.LoadFactor() <<
  //              std::endl;

  return EXIT_SUCCESS;
}