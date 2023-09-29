// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>

#include <perroht/perroht.hpp>

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
#include <metall/metall.hpp>
#include <metall/container/vector.hpp>
#include <metall/container/queue.hpp>
#include <metall/container/scoped_allocator.hpp>
#endif

#include <memory>
#include <vector>

using PerrohtContainer = perroht::Perroht<int, int>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using PerrohtMetall = perroht::Perroht<
    int, int, std::hash<int>, std::equal_to<int>, true,
    metall::manager::allocator_type<
        perroht::prhdtls::KeyValueTraits<int, int, true>::KeyValueType>>;
#endif

static constexpr const char* kMetallDataStorePath = "./test-perroht";

template <typename T>
class PerrohtUniqueTest_KeyValue : public ::testing::Test {
  void create(PerrohtContainer*& m) { m = new PerrohtContainer(); }

  void destroy(PerrohtContainer*& m) { delete m; }

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  void create(PerrohtMetall*& m) {
    manager = new metall::manager(metall::create_only, kMetallDataStorePath);
    m = manager->construct<PerrohtMetall>("perroht")(manager->get_allocator());
  }

  void destroy(PerrohtMetall*& m) {
    manager->destroy_ptr<PerrohtMetall>(m);
    delete manager;
  }
#endif

 protected:
  void SetUp() override { create(perroht_); }
  void TearDown() override { destroy(perroht_); }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  metall::manager* manager;
#endif
  T* perroht_;
};

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using MapTypes = ::testing::Types<PerrohtContainer, PerrohtMetall>;
#else
using MapTypes = ::testing::Types<PerrohtContainer>;
#endif
TYPED_TEST_SUITE(PerrohtUniqueTest_KeyValue, MapTypes);

template <typename T>
void DefaultConstructorTest(T& perroht) {
  EXPECT_EQ(perroht->Size(), 0);
  EXPECT_GE(perroht->Capacity(), 0);
  EXPECT_TRUE(perroht->Empty());
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, DefaultConstructor) {
  TypeParam* perroht = this->perroht_;
  DefaultConstructorTest(perroht);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, CopyConstructor) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair<int, int>(1, 11));
  perroht->Insert(std::make_pair<int, int>(2, 22));
  perroht->Insert(std::make_pair<int, int>(3, 33));
  perroht->Insert(std::make_pair<int, int>(4, 44));
  TypeParam copy_perroht(*perroht);
  EXPECT_EQ(copy_perroht.Count(1), 1);
  EXPECT_EQ(copy_perroht.Count(2), 1);
  EXPECT_EQ(copy_perroht.Count(3), 1);
  EXPECT_EQ(copy_perroht.Count(4), 1);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, MoveConstructor) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair<int, int>(1, 11));
  perroht->Insert(std::make_pair<int, int>(2, 22));
  perroht->Insert(std::make_pair<int, int>(3, 33));
  perroht->Insert(std::make_pair<int, int>(4, 44));
  TypeParam move_perroht(std::move(*perroht));
  EXPECT_EQ(move_perroht.Count(1), 1);
  EXPECT_EQ(move_perroht.Count(2), 1);
  EXPECT_EQ(move_perroht.Count(3), 1);
  EXPECT_EQ(move_perroht.Count(4), 1);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Empty) {
  TypeParam* perroht = this->perroht_;
  EXPECT_TRUE(perroht->Empty())
      << "Size is not 0. size is instead " << perroht->Size();

  perroht->Insert(std::make_pair(0, 10));
  EXPECT_FALSE(perroht->Empty());

  perroht->Clear();
  EXPECT_TRUE(perroht->Empty());
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Insert) {
  TypeParam* perroht = this->perroht_;
  EXPECT_EQ(perroht->Size(), 0);
  EXPECT_GE(perroht->Capacity(), 0);

  {
    const auto ret = perroht->Insert(std::make_pair(0, 10));
    EXPECT_EQ(ret.first->first, 0);
    EXPECT_EQ(ret.first->second, 10);
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(perroht->Size(), 1);
    EXPECT_GE(perroht->Capacity(), 1);
  }

  // Fail to insert duplicate key
  {
    const auto ret = perroht->Insert(std::make_pair(0, 10));
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(perroht->Size(), 1);
    EXPECT_GE(perroht->Capacity(), 1);
  }

  {
    const auto ret = perroht->Insert(std::make_pair(1, 11));
    EXPECT_EQ(ret.first->first, 1);
    EXPECT_EQ(ret.first->second, 11);
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(perroht->Size(), 2);
    EXPECT_GE(perroht->Capacity(), 2);
  }

  // Fail to insert duplicate key
  {
    const auto ret = perroht->Insert(std::make_pair(1, 11));
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(perroht->Size(), 2);
    EXPECT_GE(perroht->Capacity(), 2);
  }

  {
    const auto ret = perroht->Insert(std::make_pair(2, 12));
    EXPECT_EQ(ret.first->first, 2);
    EXPECT_EQ(ret.first->second, 12);
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(perroht->Size(), 3);
    EXPECT_GE(perroht->Capacity(), 3);
  }

  // Fail to insert duplicate key
  {
    const auto ret = perroht->Insert(std::make_pair(2, 12));
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(perroht->Size(), 3);
    EXPECT_GE(perroht->Capacity(), 3);
  }

  {
    // Const Reference version
    std::pair<int, int> d(3, 13);
    const auto ret = perroht->Insert(d);
    EXPECT_EQ(ret.first->first, 3);
    EXPECT_EQ(ret.first->second, 13);
    EXPECT_TRUE(ret.second);
    EXPECT_EQ(perroht->Size(), 4);
    EXPECT_GE(perroht->Capacity(), 4);
  }

  // Fail to insert duplicate key
  {
    // Const Reference version
    std::pair<int, int> d(3, 13);
    const auto ret = perroht->Insert(d);
    EXPECT_FALSE(ret.second);
    EXPECT_EQ(perroht->Size(), 4);
    EXPECT_GE(perroht->Capacity(), 4);
  }
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Find) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 12));
  perroht->Insert(std::make_pair(3, 13));

  {
    const auto ret = perroht->Find(0);
    EXPECT_EQ(ret->first, 0);
    EXPECT_EQ(ret->second, 10);
  }

  {
    const auto ret = perroht->Find(1);
    EXPECT_EQ(ret->first, 1);
    EXPECT_EQ(ret->second, 11);
  }

  {
    const auto ret = perroht->Find(2);
    EXPECT_EQ(ret->first, 2);
    EXPECT_EQ(ret->second, 12);
  }

  {
    const auto ret = perroht->Find(3);
    EXPECT_EQ(ret->first, 3);
    EXPECT_EQ(ret->second, 13);
  }

  const auto& const_perroht = perroht;
  {
    const auto ret = const_perroht->Find(0);
    EXPECT_EQ(ret->first, 0);
    EXPECT_EQ(ret->second, 10);
  }

  {
    const auto ret = const_perroht->Find(1);
    EXPECT_EQ(ret->first, 1);
    EXPECT_EQ(ret->second, 11);
  }

  {
    const auto ret = const_perroht->Find(2);
    EXPECT_EQ(ret->first, 2);
    EXPECT_EQ(ret->second, 12);
  }

  {
    const auto ret = const_perroht->Find(3);
    EXPECT_EQ(ret->first, 3);
    EXPECT_EQ(ret->second, 13);
  }
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, FindExpectedEnd) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));

  {
    const auto ret = perroht->Find(0);
    EXPECT_EQ(ret->first, 0);
    EXPECT_EQ(ret->second, 10);
  }

  {
    const auto ret = perroht->Find(1);
    EXPECT_EQ(ret->first, 1);
    EXPECT_EQ(ret->second, 11);
  }

  {
    const auto ret = perroht->Find(2);
    EXPECT_EQ(ret, perroht->End());
  }

  {
    const auto ret = perroht->Find(3);
    EXPECT_EQ(ret, perroht->End());
  }

  const auto& const_perroht = perroht;
  {
    const auto ret = const_perroht->Find(0);
    EXPECT_EQ(ret->first, 0);
    EXPECT_EQ(ret->second, 10);
  }

  {
    const auto ret = const_perroht->Find(1);
    EXPECT_EQ(ret->first, 1);
    EXPECT_EQ(ret->second, 11);
  }

  {
    const auto ret = const_perroht->Find(2);
    EXPECT_EQ(ret, perroht->End());
  }

  {
    const auto ret = const_perroht->Find(3);
    EXPECT_EQ(ret, perroht->End());
  }
}
struct CustomHash {
  std::size_t operator()(int const& key) const noexcept {
    return key * (key + 3);
  }
  std::size_t operator()(std::string const& key) const noexcept {
    unsigned long hash = 5381;
    for (size_t i = 0; i < key.size(); ++i)
      hash = 33 * hash + (unsigned char)key[i];
    return hash;
  }
};

using PerrohtContainer_CustomHash = perroht::Perroht<int, int, CustomHash>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using PerrohtMetall_CustomHash = perroht::Perroht<
    int, int, CustomHash, std::equal_to<int>, true,
    metall::manager::allocator_type<
        perroht::prhdtls::KeyValueTraits<int, int, true>::KeyValueType>>;
#endif

template <typename T>
class PerrohtUniqueTest_KeyValue_CustomHash : public ::testing::Test {
  void create(PerrohtContainer_CustomHash*& m) {
    m = new PerrohtContainer_CustomHash();
  }

  void destroy(PerrohtContainer_CustomHash*& m) { delete m; }

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  void create(PerrohtMetall_CustomHash*& m) {
    manager = new metall::manager(metall::create_only, kMetallDataStorePath);
    m = manager->construct<PerrohtMetall_CustomHash>("perroht")(
        manager->get_allocator());
  }

  void destroy(PerrohtMetall_CustomHash*& m) {
    manager->destroy_ptr<PerrohtMetall_CustomHash>(m);
    delete manager;
  }
#endif

 protected:
  void SetUp() override { create(perroht_); }
  void TearDown() override { destroy(perroht_); }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  metall::manager* manager;
#endif
  T* perroht_;
};

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using MapTypes_CustomHash =
    ::testing::Types<PerrohtContainer_CustomHash, PerrohtMetall_CustomHash>;
#else
using MapTypes_CustomHash = ::testing::Types<PerrohtContainer_CustomHash>;
#endif
TYPED_TEST_SUITE(PerrohtUniqueTest_KeyValue_CustomHash, MapTypes_CustomHash);

TYPED_TEST(PerrohtUniqueTest_KeyValue_CustomHash, Insert) {
  struct CustomHash {
    std::size_t operator()(int const& key) const noexcept {
      return key * (key + 3);
    }
    std::size_t operator()(std::string const& key) const noexcept {
      unsigned long hash = 5381;
      for (size_t i = 0; i < key.size(); ++i)
        hash = 33 * hash + (unsigned char)key[i];
      return hash;
    }
  };
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 12));
  perroht->Insert(std::make_pair(3, 13));

  EXPECT_EQ(perroht->Size(), 4);
  EXPECT_GE(perroht->Capacity(), 4);

  auto ret = perroht->Find(0);
  EXPECT_EQ(ret->first, 0);
  EXPECT_EQ(ret->second, 10);
  ret = perroht->Find(1);
  EXPECT_EQ(ret->first, 1);
  EXPECT_EQ(ret->second, 11);
  ret = perroht->Find(2);
  EXPECT_EQ(ret->first, 2);
  EXPECT_EQ(ret->second, 12);
  ret = perroht->Find(3);
  EXPECT_EQ(ret->first, 3);
  EXPECT_EQ(ret->second, 13);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Count) {
  TypeParam* perroht = this->perroht_;
  const auto& const_perroht = perroht;

  perroht->Insert(std::make_pair(0, 10));
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 0);
  EXPECT_EQ(perroht->Count(2), 0);
  EXPECT_EQ(perroht->Count(3), 0);
  EXPECT_EQ(const_perroht->Count(0), 1);
  EXPECT_EQ(const_perroht->Count(1), 0);
  EXPECT_EQ(const_perroht->Count(2), 0);
  EXPECT_EQ(const_perroht->Count(3), 0);

  perroht->Insert(std::make_pair(1, 11));
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 0);
  EXPECT_EQ(perroht->Count(3), 0);
  EXPECT_EQ(const_perroht->Count(0), 1);
  EXPECT_EQ(const_perroht->Count(1), 1);
  EXPECT_EQ(const_perroht->Count(2), 0);
  EXPECT_EQ(const_perroht->Count(3), 0);

  perroht->Insert(std::make_pair(2, 12));
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 1);
  EXPECT_EQ(perroht->Count(3), 0);
  EXPECT_EQ(const_perroht->Count(0), 1);
  EXPECT_EQ(const_perroht->Count(1), 1);
  EXPECT_EQ(const_perroht->Count(2), 1);
  EXPECT_EQ(const_perroht->Count(3), 0);

  perroht->Insert(std::make_pair(3, 13));
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 1);
  EXPECT_EQ(perroht->Count(3), 1);
  EXPECT_EQ(const_perroht->Count(0), 1);
  EXPECT_EQ(const_perroht->Count(1), 1);
  EXPECT_EQ(const_perroht->Count(2), 1);
  EXPECT_EQ(const_perroht->Count(3), 1);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Clear) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 12));
  perroht->Insert(std::make_pair(3, 13));
  perroht->Clear();
  EXPECT_EQ(perroht->Size(), 0);
  EXPECT_GE(perroht->Capacity(), 4);

  // Can insert again
  EXPECT_TRUE(perroht->Insert(std::make_pair(0, 10)).second);
  EXPECT_TRUE(perroht->Insert(std::make_pair(1, 11)).second);
}

template <typename T>
void TestIteratorHelper(T& it) {
  EXPECT_EQ(it->first, 0);
  EXPECT_EQ(it->second, 10);

  ++it;
  EXPECT_EQ(it->first, 1);
  EXPECT_EQ(it->second, 11);

  ++it;
  EXPECT_EQ(*it, std::make_pair(2, 12));

  it++;
  EXPECT_EQ(it->first, 3);
  EXPECT_EQ(it->second, 13);

  const auto old_it = it++;
  EXPECT_EQ(old_it->first, 3);
  EXPECT_EQ(old_it->second, 13);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Iterator) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 12));
  perroht->Insert(std::make_pair(3, 13));
  {
    TypeParam* perroht = this->perroht_;
    auto it = perroht->Begin();
    SCOPED_TRACE("Begin/End");
    TestIteratorHelper(it);
    EXPECT_EQ(it, perroht->End());
  }

  {
    TypeParam* perroht = this->perroht_;
    auto it = perroht->CBegin();
    SCOPED_TRACE("CBegin/CEnd");
    TestIteratorHelper(it);
    EXPECT_EQ(it, perroht->CEnd());
  }

  const auto& const_perroht = perroht;
  {
    auto it = const_perroht->Begin();
    SCOPED_TRACE("Begin/End (const)");
    TestIteratorHelper(it);
    EXPECT_EQ(it, const_perroht->End());
  }

  {
    auto it = const_perroht->CBegin();
    SCOPED_TRACE("CBegin/CEnd (const)");
    TestIteratorHelper(it);
    EXPECT_EQ(it, const_perroht->CEnd());
  }
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, ReserveAfterEmpty) {
  TypeParam* perroht = this->perroht_;
  EXPECT_TRUE(perroht->Reserve(100));
  EXPECT_GE(perroht->Capacity(), 100);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, ReserveAfterInsertion) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  EXPECT_TRUE(perroht->Reserve(100));
  EXPECT_GE(perroht->Capacity(), 100);

  // Make sure existing elements are still there
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Size(), 2);

  // Make sure we can still insert
  EXPECT_TRUE(perroht->Insert(std::make_pair(2, 12)).second);
  EXPECT_EQ(perroht->Count(2), 1);

  // Make sure Reserve() does not do anything if the capacity is already
  // large enough
  const auto old_capacity = perroht->Capacity();
  EXPECT_TRUE(perroht->Reserve(old_capacity));
  EXPECT_EQ(perroht->Capacity(), old_capacity);
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 1);

  EXPECT_TRUE(perroht->Reserve(1));
  EXPECT_EQ(perroht->Capacity(), old_capacity);
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 1);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, RehashAfterEmpty) {
  TypeParam* perroht = this->perroht_;
  EXPECT_TRUE(perroht->Rehash(100));
  EXPECT_GE(perroht->Capacity(), 100);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, RehashAfterInsertion) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  EXPECT_TRUE(perroht->Rehash(100));
  EXPECT_GE(perroht->Capacity(), 100);

  // Make sure existing elements are still there
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Size(), 2);

  // Make sure we can still insert
  EXPECT_TRUE(perroht->Insert(std::make_pair(2, 12)).second);
  EXPECT_EQ(perroht->Count(2), 1);

  // Make sure Rehash() does not do anything if the capacity is already
  // large enough
  const auto old_capacity = perroht->Capacity();
  EXPECT_TRUE(perroht->Rehash(old_capacity));
  EXPECT_EQ(perroht->Capacity(), old_capacity);
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 1);

  EXPECT_TRUE(perroht->Rehash(1));
  EXPECT_GE(perroht->Capacity(), 3);
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 1);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, ShrinkToFit) {
  TypeParam* perroht = this->perroht_;
  EXPECT_TRUE(perroht->ShrinkToFit());

  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 12));
  perroht->Insert(std::make_pair(3, 13));

  EXPECT_TRUE(perroht->ShrinkToFit());
  EXPECT_GE(perroht->Capacity(), 4);
  EXPECT_EQ(perroht->Size(), 4);
  EXPECT_EQ(perroht->Count(0), 1);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 1);
  EXPECT_EQ(perroht->Count(3), 1);

  perroht->Erase(0);
  perroht->Erase(1);
  perroht->Erase(2);

  const auto old_capacity = perroht->Capacity();
  EXPECT_TRUE(perroht->ShrinkToFit());
  EXPECT_LT(perroht->Capacity(), old_capacity);
  EXPECT_EQ(perroht->Size(), 1);
  EXPECT_EQ(perroht->Count(3), 1);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Erase) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 12));
  perroht->Insert(std::make_pair(3, 13));
  EXPECT_EQ(perroht->Erase(0), 1);
  EXPECT_EQ(perroht->Erase(0), 0);
  EXPECT_EQ(perroht->Size(), 3);
  EXPECT_EQ(perroht->Count(0), 0);
  EXPECT_EQ(perroht->Count(1), 1);
  EXPECT_EQ(perroht->Count(2), 1);
  EXPECT_EQ(perroht->Count(3), 1);

  EXPECT_EQ(perroht->Erase(1), 1);
  EXPECT_EQ(perroht->Erase(1), 0);
  EXPECT_EQ(perroht->Size(), 2);
  EXPECT_EQ(perroht->Count(0), 0);
  EXPECT_EQ(perroht->Count(1), 0);
  EXPECT_EQ(perroht->Count(2), 1);
  EXPECT_EQ(perroht->Count(3), 1);

  EXPECT_EQ(perroht->Erase(2), 1);
  EXPECT_EQ(perroht->Erase(2), 0);
  EXPECT_EQ(perroht->Size(), 1);
  EXPECT_EQ(perroht->Count(0), 0);
  EXPECT_EQ(perroht->Count(1), 0);
  EXPECT_EQ(perroht->Count(2), 0);
  EXPECT_EQ(perroht->Count(3), 1);

  EXPECT_EQ(perroht->Erase(3), 1);
  EXPECT_EQ(perroht->Erase(3), 0);
  EXPECT_EQ(perroht->Size(), 0);
  EXPECT_EQ(perroht->Count(0), 0);
  EXPECT_EQ(perroht->Count(1), 0);
  EXPECT_EQ(perroht->Count(2), 0);
  EXPECT_EQ(perroht->Count(3), 0);
  EXPECT_TRUE(perroht->Empty());

  EXPECT_EQ(perroht->Erase(0), 0);
  EXPECT_EQ(perroht->Erase(1), 0);
  EXPECT_EQ(perroht->Erase(2), 0);
  EXPECT_EQ(perroht->Erase(3), 0);

  // Can still insert after erasing
  EXPECT_TRUE(perroht->Insert(std::make_pair(0, 10)).second);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, EraseUsingIteratorFind) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  EXPECT_NE(perroht->Erase(perroht->Find(0)), perroht->End());
  EXPECT_EQ(perroht->Erase(perroht->Find(1)), perroht->End());
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, EraseUsingIteratorBegin) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(0, 10));
  perroht->Insert(std::make_pair(1, 11));
  EXPECT_NE(perroht->Erase(perroht->CBegin()), perroht->End());
  EXPECT_EQ(perroht->Erase(perroht->CBegin()), perroht->End());
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, LoadFactor) {
  TypeParam* perroht = this->perroht_;
  EXPECT_DOUBLE_EQ(perroht->LoadFactor(), 0.0);

  perroht->Insert(std::make_pair(0, 10));
  EXPECT_GE(perroht->LoadFactor(), 0.0);
  EXPECT_LE(perroht->LoadFactor(), perroht->MaxLoadFactor());

  perroht->Insert(std::make_pair(1, 11));
  EXPECT_GE(perroht->LoadFactor(), 0.0);
  EXPECT_LE(perroht->LoadFactor(), perroht->MaxLoadFactor());
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Swap) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 22));
  perroht->Insert(std::make_pair(3, 33));
  TypeParam perroht1(*perroht);
  perroht->Clear();
  perroht->Insert(std::make_pair(4, 44));
  perroht->Insert(std::make_pair(5, 55));
  EXPECT_TRUE(perroht->Contains(4));
  EXPECT_TRUE(perroht->Contains(5));
  EXPECT_TRUE(perroht1.Contains(1));
  EXPECT_TRUE(perroht1.Contains(2));
  EXPECT_TRUE(perroht1.Contains(3));
  perroht->Swap(perroht1);
  EXPECT_TRUE(perroht->Contains(1));
  EXPECT_TRUE(perroht->Contains(2));
  EXPECT_TRUE(perroht->Contains(3));
  EXPECT_TRUE(perroht1.Contains(4));
  EXPECT_TRUE(perroht1.Contains(5));
  swap(*perroht, perroht1);
  EXPECT_TRUE(perroht->Contains(4));
  EXPECT_TRUE(perroht->Contains(5));
  EXPECT_TRUE(perroht1.Contains(1));
  EXPECT_TRUE(perroht1.Contains(2));
  EXPECT_TRUE(perroht1.Contains(3));
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Equality) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 22));
  perroht->Insert(std::make_pair(3, 33));
  TypeParam perroht1(*perroht);
  EXPECT_EQ(perroht->Size(), 3);
  EXPECT_EQ(perroht1.Size(), 3);
  EXPECT_TRUE(*perroht == perroht1);
  EXPECT_TRUE(perroht1 == *perroht);
  EXPECT_TRUE(*perroht == *perroht);
  EXPECT_TRUE(perroht1 == perroht1);
}

TYPED_TEST(PerrohtUniqueTest_KeyValue, Inequality) {
  TypeParam* perroht = this->perroht_;
  perroht->Insert(std::make_pair(1, 11));
  perroht->Insert(std::make_pair(2, 22));
  perroht->Insert(std::make_pair(3, 33));
  TypeParam perroht1(*perroht);
  perroht1.Erase(3);
  EXPECT_TRUE(*perroht != perroht1);
  EXPECT_TRUE(perroht1 != *perroht);
  EXPECT_TRUE(*perroht == *perroht);
  EXPECT_TRUE(perroht1 == perroht1);
}

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using metall::container::scoped_allocator_adaptor;

TEST(MetallNestedContainersTest, VectorOfPerroht) {
  using outer_vector_type = metall::container::vector<
      PerrohtMetall,
      scoped_allocator_adaptor<metall::manager::allocator_type<PerrohtMetall>>>;

  metall::manager manager(metall::create_only, kMetallDataStorePath);

  manager.construct<outer_vector_type>("vec-of-perroht")(
      manager.get_allocator<>());
}

TEST(MetallNestedContainersTest, PerrohtOfVector) {
  using DataType =
      boost::unordered_map<int, metall::manager::allocator_type<int>>;
  using PerrohtContainer =
      perroht::Perroht<int, DataType, std::hash<int>, std::equal_to<int>, true,
                       scoped_allocator_adaptor<metall::manager::allocator_type<
                           perroht::prhdtls::KeyValueTraits<
                               int, DataType, true>::KeyValueType>>>;

  metall::manager manager(metall::create_only, kMetallDataStorePath);
  manager.construct<PerrohtContainer>("perroht")(manager.get_allocator<>());
}

TEST(MetallNestedContainersTest, PerrohtOfPerroht) {
  using DataType = boost::unordered_map<
      int, metall::manager::allocator_type<
               perroht::prhdtls::KeyValueTraits<int, int, true>::KeyValueType>>;
  using PerrohtContainer =
      perroht::Perroht<int, DataType, std::hash<int>, std::equal_to<int>, true,
                       scoped_allocator_adaptor<metall::manager::allocator_type<
                           perroht::prhdtls::KeyValueTraits<
                               int, DataType, true>::KeyValueType>>>;

  metall::manager manager(metall::create_only, kMetallDataStorePath);
  manager.construct<PerrohtContainer>("perroht")(manager.get_allocator<>());
}

TEST(MetallOffsetPointerTest, Insert) {
  {
    metall::manager manager(metall::create_only, kMetallDataStorePath);
    auto perroht =
        manager.construct<PerrohtMetall>("perroht")(manager.get_allocator<>());
    perroht->Insert(std::make_pair(1, 2));
    perroht->Insert(std::make_pair(3, 4));
    perroht->Insert(std::make_pair(5, 6));
    perroht->Insert(std::make_pair(7, 8));
    perroht->Insert(std::make_pair(9, 10));
    EXPECT_EQ(perroht->Count(1), 1);
    EXPECT_EQ(perroht->Count(3), 1);
    EXPECT_EQ(perroht->Count(5), 1);
    EXPECT_EQ(perroht->Count(7), 1);
    EXPECT_EQ(perroht->Count(9), 1);
  }
  {
    metall::manager manager(metall::open_read_only, kMetallDataStorePath);
    auto perroht = manager.find<PerrohtMetall>("perroht").first;
    EXPECT_EQ(perroht->Count(1), 1);
    EXPECT_EQ(perroht->Count(3), 1);
    EXPECT_EQ(perroht->Count(5), 1);
    EXPECT_EQ(perroht->Count(7), 1);
    EXPECT_EQ(perroht->Count(9), 1);
  }
}
#endif