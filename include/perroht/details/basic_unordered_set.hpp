// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <utility>
#include <functional>
#include <memory>
#include <type_traits>

#include "perroht/perroht.hpp"

namespace perroht::prhdtls {
template <typename Key, typename Hash, typename KeyEqual, bool Embed,
          typename Allocator>
class basic_unordered_set {
 private:
  using ImplType = perroht::Perroht<Key, perroht::VoidValue, Hash, KeyEqual,
                                    Embed, Allocator>;
  using KeyValueType = typename ImplType::KeyValueType;

 public:
  using key_type = Key;
  using mapped_type = Key;
  using value_type = KeyValueType;
  using size_type = typename ImplType::SizeType;
  using difference_type = typename ImplType::DifferentType;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using allocator_type = Allocator;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::allocator_traits<Allocator>::pointer;
  using const_pointer =
      typename std::allocator_traits<Allocator>::const_pointer;
  // TODO: implement dedicated iterators
  using iterator = typename ImplType::Iterator;
  using const_iterator = typename ImplType::ConstIterator;

  basic_unordered_set()
      : impl_(0, 0.875, Hash(), key_equal(), allocator_type()){};

  basic_unordered_set(size_type n, const Hash& hash,
                      const allocator_type& alloc)
      : impl_(n, 0.875, hash, key_equal(), alloc){};

  explicit basic_unordered_set(size_type n, const Hash& hash = Hash(),
                               const key_equal& equal = key_equal(),
                               const allocator_type& alloc = allocator_type())
      : impl_(n, 0.875, hash, equal, alloc){};

  basic_unordered_set(size_type n, const allocator_type& alloc)
      : impl_(n, 0.875, Hash(), key_equal(), alloc){};

  explicit basic_unordered_set(const allocator_type& alloc)
      : impl_(0, 0.875, Hash(), key_equal(), alloc){};

  basic_unordered_set(const basic_unordered_set& other) = default;
  basic_unordered_set(basic_unordered_set&& other) noexcept = default;

  basic_unordered_set(const basic_unordered_set& other,
                      const allocator_type& alloc)
      : impl_(other.impl_, alloc) {}

  basic_unordered_set(basic_unordered_set&& other,
                      const allocator_type& alloc) noexcept
      : impl_(std::move(other.impl_), alloc) {}

  ~basic_unordered_set() = default;

  template <typename K, typename H, typename Q, bool E, typename A>
  friend bool operator==(const basic_unordered_set<K, H, Q, E, A>& lhs,
                         const basic_unordered_set<K, H, Q, E, A>& rhs);

  template <typename K, typename H, typename Q, bool E, typename A>
  friend bool operator!=(const basic_unordered_set<K, H, Q, E, A>& lhs,
                         const basic_unordered_set<K, H, Q, E, A>& rhs);

  basic_unordered_set& operator=(const basic_unordered_set& other) = default;
  basic_unordered_set& operator=(basic_unordered_set&& other) noexcept =
      default;

  allocator_type get_allocator() const noexcept { return impl_.GetAllocator(); }

  // ----- Modifiers ----- //

  void clear() noexcept { impl_.Clear(); }

  std::pair<iterator, bool> insert(const value_type& value) {
    return impl_.Insert(value);
  }

  std::pair<iterator, bool> insert(value_type&& value) {
    return impl_.Insert(std::move(value));
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return impl_.Emplace(std::forward<Args>(args)...);
  }

  size_type erase(const Key& key) { return impl_.Erase(key); }

  iterator erase(iterator pos) { return impl_.Erase(pos); }

  iterator erase(const_iterator pos) { return impl_.Erase(pos); }

  void swap(basic_unordered_set& other) noexcept(
      std::allocator_traits<Allocator>::is_always_equal::value &&
      std::is_nothrow_swappable<Hash>::value &&
      std::is_nothrow_swappable<key_equal>::value) {
    impl_.Swap(other.impl_);
  }

  // ----- Lookup ----- //

  size_type count(const Key& key) const { return impl_.Count(key); }

  template<typename K>
  iterator find(const K& key) { return impl_.Find(key); }

  template<typename K>
  const_iterator find(const K& key) const { return impl_.Find(key); }

  bool contains(const Key& key) const { return impl_.Contains(key); }

  // ----- Iterators ----- //

  iterator begin() noexcept { return impl_.Begin(); }

  const_iterator begin() const noexcept { return impl_.Begin(); }

  const_iterator cbegin() const noexcept { return impl_.Begin(); }

  iterator end() noexcept { return impl_.End(); }

  const_iterator end() const noexcept { return impl_.End(); }

  const_iterator cend() const noexcept { return impl_.End(); }

  // ----- Capacity ----- //

  bool empty() const noexcept { return impl_.Empty(); }

  size_type size() const noexcept { return impl_.Size(); }

  size_type max_size() const noexcept { return impl_.MaxSize(); }

  // ----- Bucket Interface ----- //

  size_type bucket_count() const noexcept { return impl_.Capacity(); }

  // ----- Hash Policy ----- //

  float load_factor() const noexcept { return impl_.LoadFactor(); }

  float max_load_factor() const noexcept { return impl_.MaxLoadFactor(); }

  void rehash(size_type count) { impl_.Rehash(count); }

  void reserve(size_type count) { impl_.Reserve(count); }

  // ----- Observers ----- //

  hasher hash_function() const { return impl_.GetHashFunction(); }

  key_equal key_eq() const { return impl_.KeyEq(); }

  // ----- Profiles (Perroht's unique functions) ----- //

  inline std::tuple<size_type, double, size_type> probe_distance_stats()
      const noexcept {
    return impl_.GetProbeDistanceStats();
  }

  inline std::vector<size_type> probe_distance_histogram() const {
    return impl_.GetProbeDistanceHistogram();
  }

 private:
  ImplType impl_;
};

template <typename Key, typename Hash, typename KeyEqual, bool Embed,
          typename Allocator>
void swap(basic_unordered_set<Key, Hash, KeyEqual, Embed, Allocator>& lhs,
          basic_unordered_set<Key, Hash, KeyEqual, Embed, Allocator>&
              rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}

template <typename Key, typename Hash, typename KeyEqual, bool Embed,
          typename Allocator>
bool operator==(
    const basic_unordered_set<Key, Hash, KeyEqual, Embed, Allocator>& lhs,
    const basic_unordered_set<Key, Hash, KeyEqual, Embed, Allocator>& rhs) {
  return lhs.impl_ == rhs.impl_;
}

template <typename Key, typename Hash, typename KeyEqual, bool Embed,
          typename Allocator>
bool operator!=(
    const basic_unordered_set<Key, Hash, KeyEqual, Embed, Allocator>& lhs,
    const basic_unordered_set<Key, Hash, KeyEqual, Embed, Allocator>& rhs) {
  return !(lhs == rhs);
}
}  // namespace perroht::prhdtls