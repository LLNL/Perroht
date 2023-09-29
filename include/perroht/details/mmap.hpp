// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <sys/mman.h>

namespace perroht::prhdtls {

/// \brief A simple wrapper for madvise(2).
// madvise(2) may return EAGAIN if the kernel is busy. In this case, we retry
// madvise(2) until it succeeds or the number of retries reaches the
// `max_retries`.
inline bool os_madvise(void *const addr, const size_t length, const int advice,
                       const std::size_t max_retries = 4) {
  int ret = -1;
  std::size_t cnt = 0;
  do {
    ret = ::madvise(addr, length, advice);
    ++cnt;
  } while (ret == -1 && errno == EAGAIN && cnt < max_retries);

  return (ret == 0);
}

}  // namespace perroht::prhdtls