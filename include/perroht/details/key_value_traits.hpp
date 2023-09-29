// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

namespace perroht {
/// \brief A void value (no value) type.
struct VoidValue {};
}  // namespace perroht

namespace perroht::prhdtls {
// Cannot use `const KeyType` if embed is true because Robin Hood Hashing
// Table shuffles around entries internally.
template <typename Key, typename Value, bool embed>
struct KeyValueTraits {
  using KeyType = Key;
  using ValueType = Value;
  using KeyValueType =
      typename std::conditional<embed, std::pair<KeyType, ValueType>,
                                std::pair<const KeyType, ValueType>>::type;

  static constexpr const KeyType& GetKey(
      const KeyValueType& key_value) noexcept {
    return key_value.first;
  }
};

template <typename Key, bool embed>
struct KeyValueTraits<Key, VoidValue, embed> {
  using KeyType = Key;
  using ValueType = VoidValue;
  using KeyValueType = KeyType;

  static constexpr const KeyType& GetKey(
      const KeyValueType& key_value) noexcept {
    return key_value;
  }
};
}  // namespace perroht::prhdtls