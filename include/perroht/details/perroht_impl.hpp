// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <cmath>

#include "mmap.hpp"
#include "memory.hpp"
#include "header.hpp"
#include "data_holder.hpp"
#include "key_value_traits.hpp"
#include "capacity_algorithms.hpp"

namespace perroht::prhdtls {

template <typename Key, typename Value, typename Hash, typename KeyEqualOp,
          bool embed, typename Alloc>
class PerrohtImpl {
 private:
  using KVTraits = KeyValueTraits<Key, Value, embed>;
  using CapacityAlgo = PowerOfTwoCapacity;

 public:
  using KeyType = typename KVTraits::KeyType;
  using ValueType = typename KVTraits::ValueType;
  using KeyValueType = typename KVTraits::KeyValueType;
  using Hasher = Hash;
  using KeyEqual = KeyEqualOp;
  using SizeType = std::size_t;
  using DifferentType = std::ptrdiff_t;
  using Allocator = RebindAlloc<Alloc, KeyValueType>;

 private:
  using SelfType = PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>;

  using DataHolderType = DataHolder<KeyValueType, embed, Allocator>;
  using ByteAllocator = RebindAlloc<Allocator, std::byte>;
  using BytePointer = typename AllocTraits<ByteAllocator>::pointer;
  using ConstBytePointer = typename AllocTraits<ByteAllocator>::const_pointer;

  using HeaderAllocator = RebindAlloc<Allocator, Header>;
  using HeaderPointer = typename AllocTraits<HeaderAllocator>::pointer;
  using DataAllocator = RebindAlloc<Allocator, DataHolderType>;
  using DataPointer = typename AllocTraits<DataAllocator>::pointer;

  template <bool IsConst>
  class BaseIterator;

  static constexpr SizeType kNullPos = std::numeric_limits<SizeType>::max();

  // The maximum load factor value used to determine automatic capacity change.
  static constexpr double kMinimumMaxLoadFactor = 0.125;

  // Grows the capacity automatically when the mean probe distance exceeds
  // this value.
  static constexpr double kAutoGrowProbeDistance = 10;

 public:
  using Iterator = BaseIterator<false>;
  using ConstIterator = BaseIterator<true>;

  static constexpr bool Embed() { return embed; }

  PerrohtImpl() = default;

  PerrohtImpl(const SizeType initial_capacity, const float max_load_factor,
              const Hasher& hasher, const KeyEqual& key_equal,
              const Allocator& alloc)
      : max_load_factor_(max_load_factor),
        allocator_(alloc),
        hasher_(hasher),
        key_equal_(key_equal) {
    max_load_factor_ = pCleanseMaxLoadFactor(max_load_factor_);
    Reserve(initial_capacity);
  }

  PerrohtImpl(const PerrohtImpl& other)
      : max_load_factor_(other.max_load_factor_),
        allocator_(
            AllocTraits<Allocator>::select_on_container_copy_construction(
                other.allocator_)),
        hasher_(other.hasher_),
        key_equal_(other.key_equal_) {
    pCopyConstructEntriesIndividuallyFrom(other);
  }

  PerrohtImpl(PerrohtImpl&& other) noexcept
      : max_load_factor_(std::move(other.max_load_factor_)),
        allocator_(std::move(other.allocator_)),
        hasher_(std::move(other.hasher_)),
        key_equal_(std::move(other.key_equal_)),
        mean_probe_distance_(std::move(other.mean_probe_distance_)),
        size_(std::move(other.size_)),
        capacity_index_(std::move(other.capacity_index_)),
        table_(std::move(other.table_)) {
    other.mean_probe_distance_ = 0;
    other.size_ = 0;
    other.capacity_index_ = 0;
    other.table_ = nullptr;
  }

  PerrohtImpl(const PerrohtImpl& other, const Allocator& alloc)
      : max_load_factor_(other.max_load_factor_),
        allocator_(alloc),
        hasher_(other.hasher_),
        key_equal_(other.key_equal_) {
    pCopyConstructEntriesIndividuallyFrom(other);
  }

  PerrohtImpl(PerrohtImpl&& other, const Allocator& alloc) noexcept
      : max_load_factor_(std::move(other.max_load_factor_)),
        allocator_(alloc),
        hasher_(std::move(other.hasher_)),
        key_equal_(std::move(other.key_equal_)) {
    if (other.allocator_ == alloc) {
      // Move all members
      mean_probe_distance_ = std::move(other.mean_probe_distance_);
      size_ = std::move(other.size_);
      capacity_index_ = std::move(other.capacity_index_);
      table_ = std::move(other.table_);

      other.mean_probe_distance_ = 0;
      other.size_ = 0;
      other.capacity_index_ = 0;
      other.table_ = nullptr;
    } else {
      // Move construct each element individually
      pMoveConstructEntriesIndividuallyFrom(std::move(other));
    }
  }

  PerrohtImpl& operator=(const PerrohtImpl& other) {
    if (this == &other) return *this;
    pFreeTable();

    max_load_factor_ = other.max_load_factor_;
    hasher_ = other.hasher_;
    key_equal_ = other.key_equal_;
    using CopyAlloc =
        typename AllocTraits<Allocator>::propagate_on_container_copy_assignment;
    if constexpr (std::is_same_v<CopyAlloc, std::true_type>) {
      allocator_ = other.allocator_;
    }
    pCopyConstructEntriesIndividuallyFrom(other);

    return *this;
  }

  PerrohtImpl& operator=(PerrohtImpl&& other) noexcept {
    if (this == &other) return *this;
    pFreeTable();

    max_load_factor_ = std::move(other.max_load_factor_);
    hasher_ = std::move(other.hasher_);
    key_equal_ = std::move(other.key_equal_);
    constexpr auto propagate_alloc = typename AllocTraits<
        Allocator>::propagate_on_container_move_assignment();
    if constexpr (propagate_alloc) {
      allocator_ = std::move(other.allocator_);
    }

    if (propagate_alloc || allocator_ == other.allocator_) {
      // As other's allocator was propagated or the same as the current one,
      // we can just move the data.
      mean_probe_distance_ = std::move(other.mean_probe_distance_);
      size_ = std::move(other.size_);
      capacity_index_ = std::move(other.capacity_index_);
      table_ = std::move(other.table_);

      other.mean_probe_distance_ = 0;
      other.size_ = 0;
      other.capacity_index_ = 0;
      other.table_ = nullptr;
    } else {
      // As two allocators are not the same, we need to move construct each
      // element.
      pMoveConstructEntriesIndividuallyFrom(std::move(other));
    }

    return *this;
  }

  ~PerrohtImpl() noexcept { pFreeTable(); }

  void Swap(PerrohtImpl& other) noexcept {
    using std::swap;
    if constexpr (std::is_same_v<typename AllocTraits<
                                     Allocator>::propagate_on_container_swap,
                                 std::true_type>) {
      swap(allocator_, other.allocator_);
    } else {
      // If two allocators are not the same,
      // that is an undefined behavior in the C++ standard.
      assert(allocator_ == other.allocator_);
    }

    using std::swap;
    swap(max_load_factor_, other.max_load_factor_);
    swap(hasher_, other.hasher_);
    swap(key_equal_, other.key_equal_);
    swap(mean_probe_distance_, other.mean_probe_distance_);
    swap(size_, other.size_);
    swap(capacity_index_, other.capacity_index_);
    swap(table_, other.table_);
  }

  template <typename K, typename V, typename H, typename E, bool e, typename A>
  friend constexpr bool operator==(
      const PerrohtImpl<K, V, H, E, e, A>& lhd,
      const PerrohtImpl<K, V, H, E, e, A>& rhd) noexcept;

  template <typename K, typename V, typename H, typename E, bool e, typename A>
  friend constexpr bool operator!=(
      const PerrohtImpl<K, V, H, E, e, A>& lhd,
      const PerrohtImpl<K, V, H, E, e, A>& rhd) noexcept;

  template <typename KVType>
  inline std::pair<Iterator, bool> Insert(KVType&& data) {
    SizeType pos = 0;
    bool found = false;
    std::tie(pos, found) = pLocate(KVTraits::GetKey(data));
    if (found) {
      return {Iterator(pos, this), false};
    }
    auto d = pConstructDataHolder(std::forward<KVType>(data));
    pos = pInsert(true, std::move(d), pos);
    return {Iterator(pos, this), true};
  }

  /// Construct and insert a new element to the container if there is no element
  /// with the same key in the container. The new element is constructed even if
  /// there is an element with the same key in the container. In the case,
  /// the newly constructed element will be destroyed immediately.
  template <typename... Args>
  std::pair<Iterator, bool> Emplace(Args&&... args) {
    auto data = pConstructDataHolder(std::forward<Args>(args)...);
    SizeType pos = kNullPos;
    {
      bool found = false;
      std::tie(pos, found) = pLocate(KVTraits::GetKey(data.Get()));
      if (found) {
        data.Clear(allocator_);
        return {Iterator(pos, this), false};
      }
    }
    pos = pInsert(true, std::move(data));
    return {Iterator(pos, this), true};
  }

  template <typename... Args>
  std::pair<Iterator, bool> TryEmplace(const KeyType& key, Args&&... args) {
    SizeType pos = kNullPos;
    {
      bool found = false;
      std::tie(pos, found) = pLocate(key);
      if (found) {
        return {Iterator(pos, this), false};
      }
    }

    DataHolderType data(allocator_);
    if constexpr (std::is_same_v<Value, VoidValue>) {
      RebindAllocTraits<Allocator, KeyValueType>::construct(allocator_,
                                                            &data.Get(), key);
    } else {
      RebindAllocTraits<Allocator, KeyValueType>::construct(
          allocator_, &data.Get(), std::piecewise_construct,
          std::forward_as_tuple(key), std::forward_as_tuple(args...));
    }

    pos = pInsert(true, std::move(data), pos);
    return {Iterator(pos, this), true};
  }

  // TODO: reduce code duplication
  template <typename... Args>
  std::pair<Iterator, bool> TryEmplace(KeyType&& key, Args&&... args) {
    SizeType pos = SizeType(-1);
    {
      bool found = false;
      std::tie(pos, found) = pLocate(key);
      if (found) {
        return {Iterator(pos, this), false};
      }
    }

    DataHolderType data(allocator_);
    if constexpr (std::is_same_v<Value, VoidValue>) {
      RebindAllocTraits<Allocator, KeyValueType>::construct(
          allocator_, &data.Get(), std::move(key));
    } else {
      RebindAllocTraits<Allocator, KeyValueType>::construct(
          allocator_, &data.Get(), std::piecewise_construct,
          std::forward_as_tuple(std::move(key)),
          std::forward_as_tuple(args...));
    }

    pos = pInsert(true, std::move(data), pos);
    return {Iterator(pos, this), true};
  }

  inline SizeType Count(const KeyType& key) const {
    return pLocate(key).second ? 1 : 0;
  }

  inline Iterator Find(const KeyType& key) {
    const auto [pos, found] = pLocate(key);
    if (!found) {
      return End();
    }
    return Iterator(pos, this);
  }

  inline ConstIterator Find(const KeyType& key) const {
    const auto [pos, found] = pLocate(key);
    if (!found) {
      return End();
    }
    return ConstIterator(pos, this);
  }

  inline bool Contains(const KeyType& key) const { return pLocate(key).second; }

  inline SizeType Erase(const KeyType& key) {
    return pEraseSingle(key) ? 1 : 0;
  }

  inline Iterator Erase(const Iterator it) {
    if (it == End()) {
      return End();
    }
    auto pos = it.Position();
    pEraseSingleAt(pos);

    // Find the next valid entry.
    for (SizeType i = 0; i < Capacity(); ++i) {
      if (!pGetHeader(pos).Empty()) {
        return Iterator(pos, this);
      }
      pos = pIncrementPosition(pos);
    }

    return End();
  }

  inline Iterator Erase(const ConstIterator it) {
    return Erase(Iterator(it.Position(), this));
  }

  inline SizeType Size() const { return size_; }

  inline SizeType MaxSize() const noexcept {
    return std::allocator_traits<Allocator>::max_size(allocator_);
  }

  inline bool Empty() const { return Size() == 0; }

  inline SizeType Capacity() const {
    return CapacityAlgo::ToCapacity(capacity_index_);
  }

  inline void Clear() noexcept { pClearAll(); }

  inline Iterator Begin() { return Iterator(0, this); }

  inline ConstIterator Begin() const { return ConstIterator(0, this); }

  inline ConstIterator CBegin() const { return ConstIterator(0, this); }

  inline Iterator End() { return Iterator(Capacity(), this); }

  inline ConstIterator End() const { return ConstIterator(Capacity(), this); }

  inline ConstIterator CEnd() const { return ConstIterator(Capacity(), this); }

  bool Reserve(const SizeType capacity) {
    if (capacity <= Capacity()) {
      return true;
    }

    const auto new_capacity = CapacityAlgo::AdjustCapacity(capacity);
    auto new_table = pAllocateTable(new_capacity);
    if (!new_table) {
      pFreeTable();
      return false;
    }

    // As we are increasing the capacity, we don't need to check the capacity.
    constexpr bool check_capacity = false;
    return pTransferEntriesTo(std::move(new_table), new_capacity,
                              check_capacity);
  }

  bool Rehash(const SizeType capacity_request) {
    const SizeType new_capacity = CapacityAlgo::AdjustCapacity(
        std::max(capacity_request, pGetRequiredCapacity(Size())));

    // Check logic error
    assert(pEnoughCapacity(Size(), new_capacity) &&
           "new_capacity_index is too small to hold existing elements");

    auto new_table = pAllocateTable(new_capacity);
    if (!new_table) {
      pFreeTable();
      return false;
    }

    constexpr bool check_capacity = true;
    return pTransferEntriesTo(std::move(new_table), new_capacity,
                              check_capacity);
  }

  inline bool ShrinkToFit() { return Rehash(Size()); }

  inline Hasher GetHashFunction() const { return hasher_; }

  inline KeyEqual GetKeyEqual() const { return key_equal_; }

  inline Allocator GetAllocator() const { return allocator_; }

  inline float LoadFactor() const noexcept {
    return Capacity() == 0
               ? 0
               : static_cast<double>(Size()) / static_cast<double>(Capacity());
  }

  inline void MaxLoadFactor(const float max_load_factor) {
    const auto old_max_load_factor = MaxLoadFactor();
    max_load_factor_ = pCleanseMaxLoadFactor(max_load_factor);
    if (max_load_factor_ < old_max_load_factor) {
      Rehash(Capacity());
    }
  }

  // Return the value in double to avoid unexpected underflow when performing
  // calculation.
  inline double MaxLoadFactor() const noexcept { return max_load_factor_; }

  inline SizeType GetApproximateMeanProbeDistance() const noexcept {
    return mean_probe_distance_;
  }

  std::tuple<SizeType, double, SizeType> GetProbeDistanceStats()
      const noexcept {
    SizeType min_dist = 0;
    SizeType max_dist = 0;
    SizeType sum = 0;
    for (SizeType i = 0; i < Capacity(); ++i) {
      if (!pGetHeader(i).Empty()) {
        const auto pd = pGetProbeDistance(i);
        min_dist = std::min(min_dist, pd);
        max_dist = std::max(max_dist, pd);
        sum += pd;
      }
    }
    return std::make_tuple(min_dist, static_cast<double>(sum) / Size(),
                           max_dist);
  }

  std::vector<SizeType> GetProbeDistanceHistogram() const {
    std::vector<SizeType> histogram(Header::MaxProbeDistance() + 1, 0);
    for (SizeType i = 0; i < Capacity(); ++i) {
      if (!pGetHeader(i).Empty()) {
        ++histogram[pGetProbeDistance(i)];
      }
    }
    return histogram;
  }

 private:
  inline static constexpr float pCleanseMaxLoadFactor(
      const float max_load_factor) {
    return std::max(std::numeric_limits<float>::epsilon() * 100.0f,
                    std::min(max_load_factor, 1.0f));
  }

  inline static SizeType pGetMemorySize(const SizeType capacity) {
    return (sizeof(Header) + sizeof(DataHolderType)) * capacity;
  }

  inline static Header& pGetHeader(BytePointer table, const SizeType pos) {
#ifdef PERROHT_SEPARATE_HEADER
    return *reinterpret_cast<Header*>(ToAddress(table) + pos * sizeof(Header));
#else
    return *reinterpret_cast<Header*>(ToAddress(table) + pGetMemorySize(pos));
#endif
  }

  inline static const Header& pGetHeader(ConstBytePointer table,
                                         const SizeType pos) {
#ifdef PERROHT_SEPARATE_HEADER
    return *reinterpret_cast<const Header*>(ToAddress(table) +
                                            pos * sizeof(Header));
#else
    return *reinterpret_cast<const Header*>(ToAddress(table) +
                                            pGetMemorySize(pos));
#endif
  }

  inline static DataHolderType& pGetData(
      BytePointer table, [[maybe_unused]] const SizeType capacity,
      const SizeType pos) {
#ifdef PERROHT_SEPARATE_HEADER
    return *reinterpret_cast<DataHolderType*>(ToAddress(table) +
                                              capacity * sizeof(Header) +
                                              pos * sizeof(DataHolderType));
#else
    return *reinterpret_cast<DataHolderType*>(
        ToAddress(table) + pGetMemorySize(pos) + sizeof(Header));
#endif
  }

  inline static const DataHolderType& pGetData(
      ConstBytePointer table, [[maybe_unused]] const SizeType capacity,
      const SizeType pos) {
#ifdef PERROHT_SEPARATE_HEADER
    return *reinterpret_cast<const DataHolderType*>(
        ToAddress(table) + capacity * sizeof(Header) +
        pos * sizeof(DataHolderType));
#else
    return *reinterpret_cast<const DataHolderType*>(
        ToAddress(table) + pGetMemorySize(pos) + sizeof(Header));
#endif
  }

  inline SizeType pGetRequiredCapacity(const SizeType size) const {
    return std::max(size, SizeType(std::ceil(size / MaxLoadFactor())));
  }

  inline bool pEnoughCapacity(const SizeType size,
                              const SizeType capacity) const {
    return (capacity * MaxLoadFactor()) >= size;
  }

  inline bool pEnoughCapacity(const SizeType size) const {
    return pEnoughCapacity(size, Capacity());
  }

  /// Calculate the ideal position for the given key,
  /// i.e. probe distance is 0 at an ideal position.
  inline SizeType pIdealPosition(const KeyType& key) const {
    const auto hash = hasher_(key);
    if constexpr (std::is_same_v<CapacityAlgo, PowerOfTwoCapacity>) {
      assert(Capacity() > 0);
      return hash & (Capacity() - 1);
    } else {
      return hash % Capacity();
    }
    assert(false);
  }

  inline SizeType pDecrementPosition(const SizeType pos) const {
    if constexpr (std::is_same_v<CapacityAlgo, PowerOfTwoCapacity>) {
      assert(Capacity() > 0);
      return (pos + Capacity() - 1) & (Capacity() - 1);
    } else {
      return (pos + Capacity() - 1) % Capacity();
    }
    assert(false);
  }

  inline SizeType pIncrementPosition(const SizeType pos) const {
    if constexpr (std::is_same_v<CapacityAlgo, PowerOfTwoCapacity>) {
      assert(Capacity() > 0);
      return (pos + 1) & (Capacity() - 1);
    } else {
      return (pos + 1) % Capacity();
    }
    assert(false);
  }

  /// Get the actual probe distance of the entry at the given position.
  /// If the stored distance is equal to or more than the maximum probe distance
  /// that can be stored,, calculate it's probe distance by recalculating the
  /// ideal position.
  /// This function should not be called for an empty entry.
  inline SizeType pGetProbeDistance(const SizeType pos) const {
    const auto& h = pGetHeader(pos);
    if (h.GetProbeDistance() < Header::MaxProbeDistance()) {
      return h.GetProbeDistance();
    }
    const auto ipos = pIdealPosition(KVTraits::GetKey(pGetData(pos).Get()));
    return (pos + Capacity() - ipos) % Capacity();
  }

  /// Set the probe distance of the entry at the given position adjusting the
  /// given distance to the maximum probe distance that can be stored.
  inline void pSetProbeDistance(const SizeType pos, const SizeType dist) {
    const auto pd =
        (dist < Header::MaxProbeDistance() ? dist : Header::MaxProbeDistance());
    pGetHeader(pos).SetProbeDistance(pd);
  }

  inline Header& pGetHeader(const SizeType pos) {
    return pGetHeader(table_, pos);
  }

  inline Header& pGetHeader(const SizeType pos) const {
    return pGetHeader(table_, pos);
  }

  inline DataHolderType& pGetData(const SizeType pos) {
    return pGetData(table_, Capacity(), pos);
  }

  inline DataHolderType& pGetData(const SizeType pos) const {
    return pGetData(table_, Capacity(), pos);
  }

  inline void pUpdateMeanProbeDistanceWithNewDistance(
      const SizeType d, const SizeType current_size) {
    mean_probe_distance_ =
        (mean_probe_distance_ * current_size + d) / (current_size + 1);
  }

  inline void pUpdateMeanProbeDistance(const SizeType old_d,
                                       const SizeType new_d,
                                       const SizeType size) {
    mean_probe_distance_ = (mean_probe_distance_ * size - old_d + new_d) / size;
  }

  void pCopyConstructEntriesIndividuallyFrom(const SelfType& other) {
    assert(this != &other && "Self copy assignment is not allowed");
    assert(Size() == 0);
    assert(Capacity() == 0);
    assert(table_ == nullptr);

    table_ = pAllocateTable(other.Capacity());
    capacity_index_ = other.capacity_index_;
    mean_probe_distance_ = other.mean_probe_distance_;

    for (SizeType i = 0; i < other.Capacity(); ++i) {
      auto& oh = other.pGetHeader(i);
      if (oh.Empty()) {
        continue;
      }

      pGetHeader(i) = oh;
      DataHolderType::ConstructInPlace(allocator_, &pGetData(i),
                                       other.pGetData(i).Get());
      ++size_;
    }
  }

  // \warning This function clean up the old table. Therefore,
  // the old table's allocator must be available.
  void pMoveConstructEntriesIndividuallyFrom(SelfType&& other) {
    assert(this != &other && "Self move assignment is not allowed");
    assert(Size() == 0);
    assert(Capacity() == 0);
    assert(table_ == nullptr);

    table_ = pAllocateTable(other.Capacity());
    capacity_index_ = other.capacity_index_;
    mean_probe_distance_ = other.mean_probe_distance_;

    for (SizeType i = 0; i < other.Capacity(); ++i) {
      auto& oh = other.pGetHeader(i);
      if (oh.Empty()) {
        continue;
      }

      pGetHeader(i) = std::move(oh);
      DataHolderType::ConstructInPlace(allocator_, &pGetData(i),
                                       std::move(other.pGetData(i).Get()));
      ++size_;
    }
    other.pFreeTable();
  }

  /// Locate the entry for the given key.
  /// If multiple entries are found, return the first one.
  /// If no entry is found, return the position where the entry should be
  /// inserted.
  std::pair<SizeType, bool> pLocate(const KeyType& key) const {
    if (Capacity() == 0) {
      return {Capacity(), false};  // not found
    }

    auto pos = pIdealPosition(key);

    for (SizeType dist = 0; dist < Capacity(); ++dist) {
      const auto& h = pGetHeader(pos);
      if (h.Empty() || pGetProbeDistance(pos) < dist) {
        break;  // not found
      }

      if (key_equal_(KVTraits::GetKey(pGetData(pos).Get()), key)) {
        return {pos, true};  // found
      }

      pos = pIncrementPosition(pos);
    }

    return {pos, false};  // not found
  }

  /// Grow the table by kGrowFactor even there is an enough capacity already.
  void pGrow(const SizeType min_required_size) {
    auto new_capacity_index = capacity_index_ + 1;

    // Make sure that the new capacity is enough to hold the elements.
    while (!pEnoughCapacity(min_required_size,
                            CapacityAlgo::ToCapacity(new_capacity_index))) {
      ++new_capacity_index;
    }
    Reserve(CapacityAlgo::ToCapacity(new_capacity_index));
  }

  void pInitTable(BytePointer table, const SizeType capacity) {
    HeaderAllocator alloc(GetAllocator());
    for (SizeType i = 0; i < capacity; ++i) {
      AllocTraits<HeaderAllocator>::construct(alloc, &pGetHeader(table, i));
      pGetHeader(table, i).Clear();
    }
  }

  /// Allocate and initialize a new table.
  BytePointer pAllocateTable(const SizeType capacity) {
    const auto size = (sizeof(Header) + sizeof(DataHolderType)) * capacity;
    ByteAllocator alloc(GetAllocator());
    BytePointer table = AllocTraits<ByteAllocator>::allocate(alloc, size);
    if (!table) {
      return nullptr;
    }
    pInitTable(table, capacity);
    return table;
  }

  bool pDeallocateTable(BytePointer table, const SizeType capacity) {
    if (!table || capacity == 0) {
      return true;
    }
    const auto size = (sizeof(Header) + sizeof(DataHolderType)) * capacity;
    ByteAllocator alloc(GetAllocator());
    AllocTraits<ByteAllocator>::deallocate(alloc, table, size);
    return true;
  }

  void pClearAll() {
    for (SizeType i = 0; i < Capacity(); ++i) {
      pClearAt(i);
    }
    size_ = 0;
    mean_probe_distance_ = 0;
  }

  /// Destroy and deallocate a table.
  void pFreeTable() noexcept {
    pClearAll();
    pDeallocateTable(table_, Capacity());
    capacity_index_ = 0;
    table_ = nullptr;
  }

  /// Replace the existing table with the given new table,
  /// moving the elements to the new one.
  bool pTransferEntriesTo(BytePointer new_table, const SizeType new_capacity,
                          const bool check_capacity) {
    assert(new_capacity == CapacityAlgo::AdjustCapacity(new_capacity) &&
           "Capacity must match one of the valid capacities");

    using std::swap;
    swap(table_, new_table);
    const auto old_capacity = CapacityAlgo::ToCapacity(capacity_index_);
    capacity_index_ = CapacityAlgo::ToIndex(new_capacity);
    size_ = 0;
    mean_probe_distance_ = 0;

    prhdtls::os_madvise(ToAddress(table_), pGetMemorySize(new_capacity),
                        MADV_RANDOM);

    // As tables were swapped, 'new_table' contains old data.
    // Rename it to old_table to avoid confusion.
    auto old_table = new_table;
    if (old_capacity == 0) {
      return true;
    }

    prhdtls::os_madvise(ToAddress(old_table), pGetMemorySize(old_capacity),
                        MADV_SEQUENTIAL);

    for (SizeType i = 0; i < old_capacity; ++i) {
      if (pGetHeader(old_table, i).Empty()) {
        continue;
      }
      pInsert(check_capacity, std::move(pGetData(old_table, old_capacity, i)));
      pGetHeader(old_table, i).Clear();
      pGetData(old_table, old_capacity, i).Clear(allocator_);
    }

    pDeallocateTable(old_table, old_capacity);
    return true;
  }

  template <typename... Args>
  inline DataHolderType pConstructDataHolder(Args&&... args) {
    return DataHolderType(allocator_, std::forward<Args>(args)...);
  }

  inline SizeType pInsert(const bool check_capacity, DataHolderType&& data,
                          const SizeType hint_pos = kNullPos) {
    SizeType inserted_pos = kNullPos;  // The position where the new element is
                                       // inserted.
    if (check_capacity && !pEnoughCapacity(Size() + 1)) {
      pGrow(Size() + 1);
      // As the table is grown, the hint position is no longer valid.
      inserted_pos = pForceInsert(std::move(data));
    } else {
      inserted_pos = pForceInsert(std::move(data), hint_pos);
    }
    if (GetApproximateMeanProbeDistance() > kAutoGrowProbeDistance &&
        LoadFactor() > kMinimumMaxLoadFactor) {
      // As 'data' was moved already, we need to copy its key here so that
      // we can find the new inserted position.
      const auto key = KVTraits::GetKey(pGetData(inserted_pos).Get());
      Reserve(Capacity() * 2);

      // Find the new inserted position
      const auto [new_position, found] = pLocate(key);
      assert(found);
      return new_position;
    }
    return inserted_pos;
  }

  /// Insert an element to the table, not checking the capacity or the
  /// duplicate entries.
  SizeType pForceInsert(DataHolderType&& data,
                        const SizeType hint_pos = kNullPos) {
    assert(Capacity() > 0);
    assert(pEnoughCapacity(Size() + 1));

    SizeType inserted_pos = kNullPos;  // The position where the new element is
                                       // inserted.
    SizeType pos;
    SizeType dist;
    if (hint_pos != kNullPos) {
      pos = hint_pos;
      dist = (pos - pIdealPosition(KVTraits::GetKey(data.Get())) + Capacity()) %
             Capacity();
    } else {
      pos = pIdealPosition(KVTraits::GetKey(data.Get()));
      dist = 0;
    }

    for (; dist < Capacity(); ++dist) {
      auto& existing_data = pGetData(pos);
      if (pGetHeader(pos).Empty()) {
        pSetProbeDistance(pos, dist);
        pUpdateMeanProbeDistanceWithNewDistance(dist, size_);
        new (&existing_data) DataHolderType(std::move(data));  // Move construct
        if (inserted_pos == kNullPos) {
          inserted_pos = pos;
        }
        ++size_;
        return inserted_pos;
      }

      const auto existing_pd = pGetProbeDistance(pos);
      if (existing_pd < dist) {
        using std::swap;
        swap(existing_data, data);
        pSetProbeDistance(pos, dist);
        pUpdateMeanProbeDistance(existing_pd, dist, size_);
        dist = existing_pd;
        if (inserted_pos == kNullPos) {
          inserted_pos = pos;
        }
      }
      pos = pIncrementPosition(pos);
    }

    assert(false && "Should not reach here");
    return kNullPos;
  }

  /// Clear the entry at the given position.
  /// Do nothing if the entry is already empty.
  inline void pClearAt(const SizeType pos) {
    assert(pos < Capacity());
    if (pGetHeader(pos).Empty()) {
      return;
    }
    pGetHeader(pos).Clear();
    pGetData(pos).Clear(allocator_);
  }

  inline bool pEraseSingle(const KeyType& key) {
    const auto [pos, found] = pLocate(key);
    if (!found) {
      return false;
    }
    pEraseSingleAt(pos);
    return true;
  }

  // Erase the element at the given position.
  // Then, shift the following elements forward until an empty slot or an
  // element with probe distance 0 is found.
  inline void pEraseSingleAt(const SizeType pos) {
    auto i = pIncrementPosition(pos);
    while (!pGetHeader(i).Empty() && pGetProbeDistance(i) > 0) {
      const auto pre_i = pDecrementPosition(i);
      pGetData(pre_i).MoveAssign(allocator_, std::move(pGetData(i)));
      const auto old_pd = pGetProbeDistance(i);
      pSetProbeDistance(pre_i, old_pd - 1);
      pUpdateMeanProbeDistance(old_pd, old_pd - 1, size_);
      i = pIncrementPosition(i);
    }
    pClearAt(pDecrementPosition(i));
    --size_;
  }

  bool pEqual(const PerrohtImpl& other) const noexcept {
    if (Size() != other.Size()) {
      return false;
    }

    for (SizeType i = 0; i < Capacity(); ++i) {
      if (pGetHeader(i).Empty()) {
        continue;
      }

      const auto& key = KVTraits::GetKey(pGetData(i).Get());
      const auto [pos, found] = other.pLocate(key);
      if (!found || pGetData(i).Get() != other.pGetData(pos).Get()) {
        return false;
      }
    }
    return true;
  }

  float max_load_factor_{0.875};  // TODO: remove or uses more compact type
  Allocator allocator_{};         // 8B
  Hasher hasher_{};
  KeyEqual key_equal_{};
  float mean_probe_distance_{0};               // TODO: use more compact type
  SizeType size_{0};                           // 8B
  CapacityAlgo::IndexType capacity_index_{0};  // 1B
  BytePointer table_{nullptr};                 // 8B
};

template <typename Key, typename Value, typename Hash, typename KeyEqualOp,
          bool embed, typename Alloc>
constexpr bool operator==(
    const PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>& lhd,
    const PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>&
        rhd) noexcept {
  return lhd.pEqual(rhd);
}

template <typename Key, typename Value, typename Hash, typename KeyEqualOp,
          bool embed, typename Alloc>
constexpr bool operator!=(
    const PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>& lhd,
    const PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>&
        rhd) noexcept {
  return !(lhd == rhd);
}

template <typename Key, typename Value, typename Hash, typename KeyEqualOp,
          bool embed, typename Alloc>
constexpr void swap(
    PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>& lhd,
    PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>& rhd) noexcept {
  lhd.Swap(rhd);
}

template <typename Key, typename Value, typename Hash, typename KeyEqualOp,
          bool embed, typename Alloc>
template <bool IsConst>
class PerrohtImpl<Key, Value, Hash, KeyEqualOp, embed, Alloc>::BaseIterator {
 private:
  using ContainerPointer = typename std::conditional_t<
      IsConst,
      RebindPointer<typename AllocTraits<Allocator>::pointer, const SelfType>,
      RebindPointer<typename AllocTraits<Allocator>::pointer, SelfType>>;

 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = KeyValueType;
  using difference_type = std::ptrdiff_t;
  using reference =
      typename std::conditional<IsConst, const value_type&, value_type&>::type;
  using pointer =
      typename std::conditional<IsConst, const value_type*, value_type*>::type;

  BaseIterator(const SizeType pos, ContainerPointer container)
      : start_pos_(pos), container_(container) {
    assert(container_);
    assert(start_pos_ <= container_->Capacity());

    // Already at the end?
    if (start_pos_ == container_->Capacity()) {
      // Move the offset_t to the 'end' position.
      offset_ = container_->Capacity();
      return;
    }

    // Make sure to move to the first valid position.
    for (; offset_ < container_->Capacity(); ++offset_) {
      if (!pHeader().Empty()) {
        break;
      }
    }
  }

  BaseIterator& operator++() {
    pMoveToNext();
    return *this;
  }

  BaseIterator operator++(int) {
    auto tmp = *this;
    pMoveToNext();
    return tmp;
  }

  reference operator*() const { return pGet(); }

  pointer operator->() const { return &(pGet()); }

  bool operator==(const BaseIterator& other) const {
    if (offset_ == container_->Capacity() &&
        other.offset_ == other.container_->Capacity()) {
      // Both are at the end
      return true;
    }

    return container_ == other.container_ && Position() == other.Position();
  }

  bool operator!=(const BaseIterator& other) const { return !(*this == other); }

  SizeType Position() const { return start_pos_ + offset_; }

 private:
  auto& pHeader() const { return container_->pGetHeader(Position()); }

  auto& pGet() const { return container_->pGetData(Position()).Get(); }

  // Move to the next valid position.
  void pMoveToNext() {
    if (offset_ == container_->Capacity()) {
      return;
    }
    ++offset_;
    for (; offset_ < container_->Capacity(); ++offset_) {
      if (!pHeader().Empty()) {
        break;
      }
    }
  }

  SizeType start_pos_{0};
  // possible range [0, capacity]
  // offset_ == capacity means that the iterator is at the 'end' position.
  SizeType offset_{0};
  ContainerPointer container_{nullptr};
};

}  // namespace perroht::prhdtls
