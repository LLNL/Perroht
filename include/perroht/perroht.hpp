// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <functional>
#include <memory>
#include <utility>

#include "details/perroht_impl.hpp"

namespace perroht {

// TODO: Change the default hash function?

/// \brief  Persistent Robin Hood Hash Table
/// \tparam Data The data wrapper type to be stored in the table.
/// \tparam Hash The hash function to be used.
/// \tparam KeyEqualOp The key equality operator to be used.
/// \tparam embed If true, works as a flat map. Otherwise, works as a node map.
/// \tparam Alloc The allocator to be used.
template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename KeyEqualOp = std::equal_to<Key>, bool embed = true,
          typename Alloc = std::allocator<typename prhdtls::KeyValueTraits<
              Key, Value, embed>::KeyValueType>>
class Perroht {
 private:
  using SelfType = Perroht<Key, Value, Hash, KeyEqualOp, embed, Alloc>;
  using Impl = prhdtls::PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>;

 public:
  using KeyType = typename Impl::KeyType;
  using ValueType = typename Impl::ValueType;
  using KeyValueType = typename Impl::KeyValueType;
  using Hasher = typename Impl::Hasher;
  using KeyEqual = typename Impl::KeyEqual;
  using Allocator = typename Impl::Allocator;
  using SizeType = typename Impl::SizeType;
  using DifferentType = typename Impl::DifferentType;
  using Iterator = typename Impl::Iterator;
  using ConstIterator = typename Impl::ConstIterator;

  /// \brief Return the maximum probe distance this container accepts.
  /// This container grows automatically when the probe distance exceeds this
  /// value.
  static constexpr SizeType MaxProbeDistance() {
    return Impl::MaxProbeDistance();
  }

  /// \brief Return the value of the embed template parameter.
  static constexpr bool Embed() { return Impl::Embed(); }

  /// \brief Constructor.
  /// \param alloc The allocator object.
  explicit Perroht(const Allocator& alloc = Allocator())
      : impl_(0, 0.875, Hasher(), KeyEqual(), alloc) {}

  /// \brief Constructor.
  /// \param capacity The initial capacity of the container.
  /// \param max_load_factor The maximum load factor of the container.
  /// Must be greater than 0.0 and equal to or less than 1.0.
  /// The load factor is calculated as Size() / Capacity().
  /// This value is used as one of the thresholds to decide when to grow the
  /// container.
  /// \param hash The hash function.
  /// \param key_equal The key equality operator.
  /// \param alloc The allocator object.
  explicit Perroht(const SizeType capacity, const float max_load_factor = 0.875,
                   const Hasher& hash = Hasher(),
                   const KeyEqual& key_equal = KeyEqual(),
                   const Allocator& alloc = Allocator())
      : impl_(capacity, max_load_factor, hash, key_equal, alloc) {}

  /// \brief Copy Constructor.
  /// \param other The other Perroht to copy from.
  Perroht(const Perroht& other) = default;

  /// \brief Move Constructor.
  /// \param other The other Perroht to move from.
  Perroht(Perroht&& other) = default;

  /// \brief Allocator-extended copy constructor.
  /// \param other The other Perroht to copy from.
  /// \param alloc The allocator object.
  Perroht(const Perroht& other, const Allocator& alloc)
      : impl_(other.impl_, alloc) {}

  /// \brief Allocator-extended move constructor.
  /// \param other The other Perroht to move from.
  /// \param alloc The allocator object.
  Perroht(Perroht&& other, const Allocator& alloc)
      : impl_(std::move(other.impl_), alloc) {}

  /// \brief Copy Assignment Operator.
  Perroht& operator=(const Perroht& other) = default;

  /// \brief Move Assignment Operator.
  /// \param other The other Perroht to move from.
  Perroht& operator=(Perroht&& other) noexcept = default;

  /// \brief Destructor.
  ~Perroht() noexcept = default;

  /// \brief Equal operator.
  /// \param rhd The right hand side Perroht to compare with.
  /// \param lhd The left hand side Perroht to compare with.
  /// \return True if the two containers have the size sizes and equal key-value
  /// elements. This operator does not check the order of the elements.
  template <typename K, typename V, typename H, typename E, bool e, typename A>
  friend constexpr bool operator==(
      const Perroht<K, V, H, E, e, A>& rhd,
      const Perroht<K, V, H, E, e, A>& lhd) noexcept;

  /// \brief Not equal operator.
  /// \param rhd The right hand side Perroht to compare with.
  /// \param lhd The left hand side Perroht to compare with.
  /// \return True if the two Perroht are not equal (== operator
  /// returns false), false otherwise.
  template <typename K, typename V, typename H, typename E, bool e, typename A>
  friend constexpr bool operator!=(
      const Perroht<K, V, H, E, e, A>& rhd,
      const Perroht<K, V, H, E, e, A>& lhd) noexcept;

  /// \brief Get the allocator.
  /// \return The allocator.
  inline Allocator GetAllocator() const { return impl_.GetAllocator(); }

  // ----- Iterators ----- //

  /// \brief Get an iterator to the first element.
  /// \return An iterator to the first element.
  inline Iterator Begin() { return impl_.Begin(); }

  /// \brief Get an iterator to the first element.
  /// This is a const version of Begin().
  /// \return An iterator to the first element.
  inline ConstIterator Begin() const { return impl_.Begin(); }

  /// \brief Get an iterator to the first element.
  /// This is a const version of Begin().
  /// \return An iterator to the first element.
  inline ConstIterator CBegin() const { return impl_.CBegin(); }

  /// \brief Get an iterator to the element after the last element.
  /// \return An iterator to the element after the last element.
  inline Iterator End() { return impl_.End(); }

  /// \brief Get an iterator to the element after the last element.
  /// This is a const version of End().
  /// \return An iterator to the element after the last element.
  inline ConstIterator End() const { return impl_.End(); }

  /// \brief Get an iterator to the element after the last element.
  /// This is a const version of End().
  /// \return An iterator to the element after the last element.
  inline ConstIterator CEnd() const { return impl_.CEnd(); }

  // ----- Capacity ----- //

  /// \brief Check if the table is empty.
  /// \return True if the table is empty, false otherwise.
  inline bool Empty() const { return impl_.Empty(); }

  /// \brief Size of the table.
  /// \return The number of elements in the table.
  inline SizeType Size() const { return impl_.Size(); }

  /// \brief Returns the maximum number of elements this container can hold
  /// theoretically. The actual maximum number of elements will be smaller than
  /// this number in practice.
  /// \return number of elements this container can hold theoretically.
  inline SizeType MaxSize() const noexcept { return impl_.MaxSize(); }

  // ----- Modifiers ----- //

  /// \brief Clear the table.
  /// This will remove all elements from the table.
  /// The capacity of the table will not be changed.
  inline void Clear() noexcept { impl_.Clear(); }

  /// \brief Insert a new element into the table.
  /// \param data Key-value data to be inserted.
  /// \return A pair of an iterator to the inserted element and a bool
  /// indicating whether the insertion was successful.
  inline std::pair<Iterator, bool> Insert(const KeyValueType& data) {
    return impl_.Insert(data);
  }

  /// \brief Insert a new element into the table.
  /// \param data Key-value data to be inserted.
  /// \return A pair of an iterator to the inserted element and a bool
  /// indicating whether the insertion was successful.
  inline std::pair<Iterator, bool> Insert(KeyValueType&& data) {
    return impl_.Insert(std::move(data));
  }

  /// \brief Insert a new element into the table.
  /// \param args Arguments to be forwarded to the constructor of Data.
  /// \return A pair of an iterator to the inserted element and a bool
  /// indicating whether the insertion was successful.
  template <class... Args>
  inline std::pair<Iterator, bool> Emplace(Args&&... args) {
    return impl_.Emplace(std::forward<Args>(args)...);
  }

  /// \brief Insert a new element into the table if the key does not exist.
  /// If the key already exists, an iterator to the existing element is
  /// returned. \param key The key to search and to insert if not found. \param
  /// args Arguments to be forwarded to the constructor and element. This
  /// argument is ignored if the container is a key-only one. \return A pair of
  /// an iterator to the inserted element and a bool indicating whether the
  /// insertion was successful.
  template <typename... Args>
  inline std::pair<Iterator, bool> TryEmplace(const KeyType& key,
                                              Args&&... args) {
    return impl_.TryEmplace(key, std::forward<Args>(args)...);
  }

  /// \brief Insert a new element into the table if the key does not exist.
  /// If the key already exists, an iterator to the existing element is
  /// returned. This function is the rvalue key version of TryEmplace(). \param
  /// key The key to search and to insert if not found. \param args Arguments to
  /// be forwarded to the constructor and element. This argument is ignored if
  /// the container is a key-only one. \return A pair of an iterator to the
  /// inserted element and a bool indicating whether the insertion was
  /// successful.
  template <typename... Args>
  inline std::pair<Iterator, bool> TryEmplace(KeyType&& key, Args&&... args) {
    return impl_.TryEmplace(key, std::forward<Args>(args)...);
  }

  /// \brief Erase the element with the given key.
  /// \param key The key to erase.
  /// \return The number of elements erased.
  inline SizeType Erase(const KeyType& key) { return impl_.Erase(key); }

  /// \brief Erase the element pointed to by the given iterator.
  /// \param it The iterator to the element to erase.
  /// \return An iterator to the element after the erased element.
  /// If the erased element is the last element, End() is returned.
  /// If the given iterator is invalid, End() is returned.
  inline Iterator Erase(const Iterator it) { return impl_.Erase(it); }

  /// \brief Erase the element pointed to by the given iterator.
  /// \param it The iterator to the element to erase.
  /// \return An iterator to the element after the erased element.
  /// If the erased element is the last element, End() is returned.
  /// If the given iterator is invalid, End() is returned.
  inline Iterator Erase(const ConstIterator it) { return impl_.Erase(it); }

  /// \brief swap
  /// \param other The other Perroht to swap with.
  inline void Swap(Perroht& other) noexcept { impl_.Swap(other.impl_); }

  // ----- Lookup ----- //

  /// \brief Count the number of elements with the given key.
  /// \param key The key to count.
  /// \return The number of elements with the given key.
  inline SizeType Count(const KeyType& key) const { return impl_.Count(key); }

  /// \brief Find the element with the given key.
  /// \param key The key to find.
  /// \return An iterator to the element with the given key.
  /// If no such element exists, End() is returned.
  /// If multiple elements with the given key exist, the first one is returned.
  inline Iterator Find(const KeyType& key) { return impl_.Find(key); }

  /// \brief Find the element with the given key.
  /// This is a const version of find().
  /// \param key The key to find.
  /// \return An iterator to the element with the given key.
  /// If no such element exists, End() is returned.
  /// If multiple elements with the given key exist, the first one is returned.
  inline ConstIterator Find(const KeyType& key) const {
    return impl_.Find(key);
  }

  /// \brief Checks if there is an element with key equivalent to key in the
  /// container. \param key The key of the element to search for. \return true
  /// if there is such an element, otherwise false.
  inline bool Contains(const KeyType& key) const { return impl_.Contains(key); }

  // ----- Hash policy ----- //

  /// \brief Get the load factor.
  /// \return The load factor.
  /// This is the ratio of the number of elements in the table to the capacity
  /// of the table.
  inline float LoadFactor() const { return impl_.LoadFactor(); }

  /// \brief Get the maximum load factor.
  /// \return The maximum load factor.
  /// This is the maximum ratio of the number of elements in the table to
  /// the capacity of the table.
  inline float MaxLoadFactor() const { return float(impl_.MaxLoadFactor()); }

  /// \brief Change the maximum load factor and rehash the container to satisfy
  /// the condition. The actually set range may be different from the given one.
  /// \param max_load_factor The maximum load factor to desire within the range
  /// of (0.0, 1.0).
  /// \warning This function may be changed to do nothing in the future.
  inline void MaxLoadFactor(const float max_load_factor) {
    impl_.MaxLoadFactor(max_load_factor);
  }

  /// \brief Sets the capacity of the table at least to capacity and rehash the
  /// table. This will rehash the all items in the table.
  /// \return True if the operation was successful, false otherwise.
  inline bool Rehash(const SizeType capacity) { return impl_.Rehash(capacity); }

  /// \brief Shrink the capacity of the table to fit the number of elements if
  /// possible. \return True if the operation was successful or nothing
  /// happened, false otherwise.
  inline bool ShrinkToFit() { return impl_.ShrinkToFit(); }

  /// \brief Reserve space for the given number of elements.
  /// If the capacity of the table is already greater than or equal to the
  /// given capacity, this function does nothing.
  /// \param capacity The number of elements to reserve space for.
  /// \return True if the operation was successful or the given capacity is
  /// equal to or smaller than the current capacity, false otherwise.
  inline bool Reserve(SizeType capacity) { return impl_.Reserve(capacity); }

  /// \brief Get the min, mean, and max probe distances.
  /// This function Takes O(n) time, where n is the number of elements in the
  /// table.
  /// \return A tuple of the min, mean, and max probe distances.
  inline std::tuple<SizeType, double, SizeType> GetProbeDistanceStats()
      const noexcept {
    return impl_.GetProbeDistanceStats();
  }

  /// \brief Get the probe distance histogram.
  /// This function Takes O(n) time, where n is the number of elements in the
  /// table.
  /// \return A vector of the probe distance histogram.
  inline std::vector<SizeType> GetProbeDistanceHistogram() const {
    return impl_.GetProbeDistanceHistogram();
  }

  /// \brief Get an approximate mean probe distance.
  /// \return An approximate mean probe distance.
  inline SizeType GetApproximateMeanProbeDistance() const noexcept {
    return impl_.GetApproximateMeanProbeDistance();
  }

  // ----- Capacity ----- //

  /// \brief Capacity of the table.
  /// \return The capacity of the table.
  /// This is the maximum number of elements that can be stored in the table.
  /// This is not the number of elements currently stored in the table.
  inline SizeType Capacity() const { return impl_.Capacity(); }

  // ----- Observers ----- //

  /// \brief Get the hash function.
  /// \return The hash function.
  inline Hasher GetHashFunction() const { return impl_.GetHashFunction(); }

  /// \brief Get the key equal operator.
  /// \return The key equal operator.
  inline KeyEqual GetKeyEqual() const { return impl_.GetKeyEqual(); }

 private:
  Impl impl_;
};

template <typename Key, typename Value, typename Hash, typename KeyEqualOp,
          bool embed, typename Alloc>
constexpr bool operator==(
    const Perroht<Key, Value, Hash, KeyEqualOp, embed, Alloc>& rhd,
    const Perroht<Key, Value, Hash, KeyEqualOp, embed, Alloc>& lhd) noexcept {
  return rhd.impl_ == lhd.impl_;
}

template <typename Key, typename Value, typename Hash, typename KeyEqualOp,
          bool embed, typename Alloc>
constexpr bool operator!=(
    const Perroht<Key, Value, Hash, KeyEqualOp, embed, Alloc>& rhd,
    const Perroht<Key, Value, Hash, KeyEqualOp, embed, Alloc>& lhd) noexcept {
  return rhd.impl_ != lhd.impl_;
}

template <typename Key, typename Value, typename Hash, typename KeyEqualOp,
          bool embed, typename Alloc>
constexpr void swap(
    Perroht<Key, Value, Hash, KeyEqualOp, embed, Alloc>& rhd,
    Perroht<Key, Value, Hash, KeyEqualOp, embed, Alloc>& lhd) noexcept {
  rhd.Swap(lhd);
}

}  // namespace perroht
