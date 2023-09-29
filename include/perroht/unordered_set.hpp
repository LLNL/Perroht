// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <utility>
#include <functional>

#include "details/basic_unordered_set.hpp"

namespace perroht {
template <typename Key, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<Key>>
class unordered_flat_set
    : public prhdtls::basic_unordered_set<Key, Hash, KeyEqual, true,
                                          Allocator> {
 private:
  using Base =
      prhdtls::basic_unordered_set<Key, Hash, KeyEqual, true, Allocator>;

 public:
  using Base::Base;
};

template <typename Key, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<Key>>
class unordered_node_set
    : public prhdtls::basic_unordered_set<Key, Hash, KeyEqual, false,
                                          Allocator> {
 private:
  using Base =
      prhdtls::basic_unordered_set<Key, Hash, KeyEqual, false, Allocator>;

 public:
  using Base::Base;
};
}  // namespace perroht