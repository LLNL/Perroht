// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

namespace perroht::prhdtls {

/// \brief Template alias for std::allocator_traits.
template <typename Alloc>
using AllocTraits = std::allocator_traits<Alloc>;

/// \brief Template alias for rebind_alloc in std::allocator_traits.
template <typename Alloc, typename T>
using RebindAlloc = typename AllocTraits<Alloc>::template rebind_alloc<T>;

/// \brief Template alias for rebind_alloc in std::allocator_traits.
template <typename Alloc, typename T>
using RebindAllocTraits =
    typename std::allocator_traits<Alloc>::template rebind_traits<T>;

/// \brief Template alias for std::pointer_traits.
template <typename Pointer>
using PointerTraits = std::pointer_traits<Pointer>;

/// \brief Template alias for rebind in std::pointer_traits.
template <typename Pointer, typename T>
using RebindPointer = typename std::pointer_traits<Pointer>::template rebind<T>;

template <typename Pointer>
inline typename std::pointer_traits<Pointer>::element_type* ToAddress(
    const Pointer pointer) {
  return &(*pointer);
}

}  // namespace perroht::prhdtls