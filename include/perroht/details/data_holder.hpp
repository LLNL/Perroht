// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <utility>
#include <functional>

#include "memory.hpp"

namespace perroht::prhdtls {

/// \brief A class to hold an instance of T.
/// If embed is true, the data is embedded in the class itself.
/// Otherwise, the data is allocated as an independent node, and this class
/// holds a pointer to the node. The purpose of this class is to provide the
/// same interface regardless of the value of embed.
template <typename T, bool embed, typename Allocator = std::allocator<T>>
class DataHolder {
 public:
  using DataType = T;
  using AllocatorType = RebindAlloc<Allocator, DataType>;
  using Pointer =
      RebindPointer<typename AllocTraits<AllocatorType>::const_pointer,
                    DataType>;

  template <typename... Args>
  static constexpr void ConstructInPlace(AllocatorType& alloc,
                                         DataHolder* const ptr,
                                         Args&&... args) {
    new (ptr) DataHolder(alloc, std::forward<Args>(args)...);
  }

  DataHolder() {
    if constexpr (!embed) {
      data_ = nullptr;
    }
  }

  /// \brief Constructor.
  /// Construct the data using the given arguments.
  template <typename... Args>
  DataHolder(AllocatorType& alloc, Args&&... args) {
    if constexpr (embed) {
      AllocTraits<AllocatorType>::construct(alloc, &data_,
                                            std::forward<Args>(args)...);
    } else {
      data_ = AllocTraits<AllocatorType>::allocate(alloc, 1);
      if (!data_) {
        assert(false);
        std::abort();
      }
      AllocTraits<AllocatorType>::construct(alloc, ToAddress(data_),
                                            std::forward<Args>(args)...);
    }
  }

  /// \brief Move constructor.
  DataHolder(DataHolder&& other) noexcept : data_(std::move(other.data_)) {
    if constexpr (!embed) {
      other.data_ = nullptr;
    }
  }

  // Allows the following operations when embed is true:
  DataHolder(const DataHolder&) = delete;
  DataHolder& operator=(const DataHolder&) = delete;
  DataHolder& operator=(DataHolder&& other) noexcept = delete;

  ~DataHolder() noexcept {
    if constexpr (!embed) {
      assert(!data_);  // safeguard for preventing memory leak
    }
  }

  /// \brief Swap the data with another DataHolder.
  void Swap(DataHolder& other) noexcept {
    using std::swap;
    swap(data_, other.data_);
  }

  /// \brief Move the data from another DataHolder.
  /// If embed is true, data object is simply moved from other using std::move.
  /// Otherwise, data node's pointer is moved from other to this after
  /// destructing the existing data. This function assumes that this and other
  /// use the same allocator, alloc.
  inline void MoveAssign(Allocator& alloc, DataHolder&& other) {
    if constexpr (embed) {
      data_ = std::move(other.data_);
    } else {
      if (data_) {
        Clear(alloc);
      }
      data_ = other.data_;
      other.data_ = nullptr;
    }
  }

  /// \brief Get the data.
  inline DataType& Get() noexcept {
    if constexpr (embed) {
      return data_;
    } else {
      return *data_;
    }
  }

  /// \brief Get the data.
  inline const DataType& Get() const noexcept {
    if constexpr (embed) {
      return data_;
    } else {
      return *data_;
    }
  }

  /// \brief Clear the data.
  void Clear(AllocatorType& alloc) {
    if constexpr (embed) {
      AllocatorType a(alloc);
      RebindAllocTraits<AllocatorType, DataType>::destroy(a, &data_);
    } else {
      if (!data_) return;
      AllocatorType a(alloc);
      RebindAllocTraits<AllocatorType, DataType>::destroy(a, ToAddress(data_));
      AllocTraits<AllocatorType>::deallocate(a, ToAddress(data_), 1);
      data_ = nullptr;
    }
  }

 private:
  // To avoid constructing data_, which may not have a default constructor or be
  // expensive to construct, hold the data_ in a union with a dummy variable
  // whose size is the same as data_.
  union {
    std::byte dummy_[sizeof(DataType)];
    typename std::conditional<embed, DataType, Pointer>::type data_;
  };
};

/// \brief Swap two DataHolder.
template <typename T, bool embed, typename Allocator>
void swap(DataHolder<T, embed, Allocator>& lhs,
          DataHolder<T, embed, Allocator>& rhs) noexcept {
  lhs.Swap(rhs);
}

}  // namespace perroht::prhdtls
