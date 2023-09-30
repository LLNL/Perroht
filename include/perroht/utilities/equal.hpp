// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

namespace perroht {

/// \brief A functor that compares two string container types.
struct StringEqual {
  template <typename S1, typename S2>
  bool operator()(const S1& lhs, const S2& rhs) const {
    return lhs.compare(0, lhs.size(), rhs.c_str(), rhs.size()) == 0;
  }
};

}  // namespace perroht