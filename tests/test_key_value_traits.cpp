// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <perroht/details/key_value_traits.hpp>

TEST(KeyValueTraitsTest, KeyValueEmbed) {
  using Traits = perroht::prhdtls::KeyValueTraits<int, double, true>;
  EXPECT_EQ(typeid(Traits::KeyType), typeid(int));
  EXPECT_EQ(typeid(Traits::ValueType), typeid(double));
  EXPECT_EQ(typeid(Traits::KeyValueType), typeid(std::pair<int, double>));

  Traits::KeyValueType kv(10, 0);
  EXPECT_EQ(Traits::GetKey(kv), 10);
  const auto& kv_const = kv;
  EXPECT_EQ(Traits::GetKey(kv_const), 10);
}

TEST(KeyValueTraitsTest, KeyValueNode) {
  using Traits = perroht::prhdtls::KeyValueTraits<int, double, false>;
  EXPECT_EQ(typeid(Traits::KeyType), typeid(int));
  EXPECT_EQ(typeid(Traits::ValueType), typeid(double));
  EXPECT_EQ(typeid(Traits::KeyValueType), typeid(std::pair<const int, double>));

  Traits::KeyValueType kv(10, 0);
  EXPECT_EQ(Traits::GetKey(kv), 10);
  const auto& kv_const = kv;
  EXPECT_EQ(Traits::GetKey(kv_const), 10);
}

TEST(KeyValueTraitsTest, KeyNoValueEmbed) {
  using Traits =
      perroht::prhdtls::KeyValueTraits<int, perroht::VoidValue, true>;
  EXPECT_EQ(typeid(Traits::KeyType), typeid(int));
  EXPECT_EQ(typeid(Traits::ValueType), typeid(perroht::VoidValue));
  EXPECT_EQ(typeid(Traits::KeyValueType), typeid(int));

  Traits::KeyValueType kv(10);
  EXPECT_EQ(Traits::GetKey(kv), 10);
  const auto& kv_const = kv;
  EXPECT_EQ(Traits::GetKey(kv_const), 10);
}

TEST(KeyValueTraitsTest, KeyNoValueNode) {
  using Traits =
      perroht::prhdtls::KeyValueTraits<int, perroht::VoidValue, false>;
  EXPECT_EQ(typeid(Traits::KeyType), typeid(int));
  EXPECT_EQ(typeid(Traits::ValueType), typeid(perroht::VoidValue));
  EXPECT_EQ(typeid(Traits::KeyValueType), typeid(const int));

  Traits::KeyValueType kv(10);
  EXPECT_EQ(Traits::GetKey(kv), 10);

  const auto& kv_const = kv;
  EXPECT_EQ(Traits::GetKey(kv_const), 10);
}
