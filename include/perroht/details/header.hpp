// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <limits>

namespace perroht::prhdtls {

class Header {
 private:
  using RawDataType = uint8_t;
  static constexpr std::size_t kEmpyMark =
      std::numeric_limits<RawDataType>::max();
  static constexpr std::size_t kMaxProbeDistance =
      std::numeric_limits<RawDataType>::max() - 1;

 public:
  using DistanceType = RawDataType;

  static constexpr DistanceType MaxProbeDistance() noexcept {
    return kMaxProbeDistance;
  }

  Header() : data_(kEmpyMark) {}

  Header(const DistanceType pos) : data_(pos) {}

  Header(const Header&) = default;
  Header(Header&&) = default;
  Header& operator=(const Header&) = default;
  Header& operator=(Header&&) = default;

  inline void Clear() noexcept { data_ = kEmpyMark; }

  inline bool Empty() const noexcept { return data_ == kEmpyMark; }

  inline void SetProbeDistance(const DistanceType pos) noexcept { data_ = pos; }

  inline DistanceType GetProbeDistance() const noexcept { return data_; }

 private:
  RawDataType data_;
};

}  // namespace perroht::prhdtls