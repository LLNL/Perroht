// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <iterator>
#include <algorithm>

namespace perroht::prhdtls {

class PowerOfTwoCapacity {
 public:
  using SizeType = std::size_t;
  using IndexType = uint8_t;
  static_assert(std::numeric_limits<SizeType>::digits <=
                (1ULL << std::numeric_limits<IndexType>::digits));

  inline static constexpr IndexType ToIndex(const SizeType size) noexcept {
    if (size == 0) return 0;
    return static_cast<SizeType>(std::ceil(std::log2(size))) + 1;
  }

  inline static constexpr SizeType ToCapacity(const IndexType index) noexcept {
    if (index == 0) return 0;
    return SizeType(1ULL << (index - 1));
  }

  inline static constexpr SizeType AdjustCapacity(
      const SizeType size) noexcept {
    return ToCapacity(ToIndex(size));
  }

  inline static constexpr SizeType MaxCapacity() noexcept {
    return 1ULL << (std::numeric_limits<SizeType>::digits - 1);
  }
};

class PrimeNumberCapacity {
 public:
  using SizeType = std::size_t;
  using IndexType = uint8_t;

 private:
  static constexpr SizeType kNumCapacities = 64;
  static constexpr SizeType kCapacities[kNumCapacities] = {
      1ul,
      2ul,
      5ul,
      11ul,
      23ul,
      47ul,
      97ul,
      199ul,
      409ul,
      823ul,
      1741ul,
      3469ul,
      6949ul,
      14033ul,
      28411ul,
      57557ul,
      116731ul,
      236897ul,
      480881ul,
      976369ul,
      1982627ul,
      4026031ul,
      8175383ul,
      16601593ul,
      33712729ul,
      68460391ul,
      139022417ul,
      282312799ul,
      573292817ul,
      1164186217ul,
      2364114217ul,
      4294967291ul,
      8589934583ull,
      17179869143ull,
      34359738337ull,
      68719476731ull,
      137438953447ull,
      274877906899ull,
      549755813881ull,
      1099511627689ull,
      2199023255531ull,
      4398046511093ull,
      8796093022151ull,
      17592186044399ull,
      35184372088777ull,
      70368744177643ull,
      140737488355213ull,
      281474976710597ull,
      562949953421231ull,
      1125899906842597ull,
      2251799813685119ull,
      4503599627370449ull,
      9007199254740881ull,
      18014398509481951ull,
      36028797018963913ull,
      72057594037927931ull,
      144115188075855859ull,
      288230376151711717ull,
      576460752303423433ull,
      1152921504606846883ull,
      2305843009213693951ull,
      4611686018427387847ull,
      9223372036854775783ull,
      18446744073709551557ull,
  };

  static_assert(kNumCapacities <= std::numeric_limits<IndexType>::max() + 1);

 public:
  inline static constexpr IndexType ToIndex(const SizeType size) noexcept {
    if (size == 0) return 0;
    const auto pos =
        std::lower_bound(kCapacities, kCapacities + kNumCapacities, size);
    return std::distance(kCapacities, pos) + 1;
  }

  inline static constexpr SizeType ToCapacity(const IndexType index) noexcept {
    if (index == 0) return 0;
    assert(index <= kNumCapacities);
    if (index >= kNumCapacities) {
      return MaxCapacity();
    }
    return kCapacities[index - 1];
  }

  inline static constexpr SizeType AdjustCapacity(
      const SizeType size) noexcept {
    return ToCapacity(ToIndex(size));
  }

  inline static constexpr SizeType MaxCapacity() noexcept {
    return 1ULL << (std::numeric_limits<SizeType>::digits - 1);
  }
};

}  // namespace perroht::prhdtls