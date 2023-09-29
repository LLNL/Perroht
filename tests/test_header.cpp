// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>

#include <perroht/details/header.hpp>

using namespace perroht::prhdtls;

TEST(HeaderTest, DefaultConstructor) {
  Header header;
  EXPECT_TRUE(header.Empty());
}

TEST(HeaderTest, MaxProbeDistance) {
  EXPECT_GT(Header::MaxProbeDistance(), 0);
  EXPECT_LE(Header::MaxProbeDistance(),
            std::size_t(std::numeric_limits<Header::DistanceType>::max()));
}

TEST(HeaderTest, ProbeDistance) {
  Header header;
  for (std::size_t i = 0; i <= Header::MaxProbeDistance(); ++i) {
    header.SetProbeDistance(i);
    EXPECT_EQ(header.GetProbeDistance(), i);
    const auto& header_const = header;
    EXPECT_EQ(header_const.GetProbeDistance(), i);
  }
}

TEST(HeaderTest, Empty) {
  Header header;
  for (std::size_t i = 0; i <= Header::MaxProbeDistance(); ++i) {
    header.SetProbeDistance(i);
    EXPECT_FALSE(header.Empty());
  }
}

TEST(HeaderTest, Clear) {
  Header header;
  header.SetProbeDistance(1);
  header.Clear();
  EXPECT_TRUE(header.Empty());
}