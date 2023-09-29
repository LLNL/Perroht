// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
#include <metall/metall.hpp>
#endif

#include <vector>
#include <utility>

#include <perroht/details/data_holder.hpp>

#include <type_traits>

using embed_kv = perroht::prhdtls::DataHolder<int, true, std::allocator<int>>;
using node_kv = perroht::prhdtls::DataHolder<int, false>;
using embed_k = perroht::prhdtls::DataHolder<std::vector<int>, true>;
using node_k = perroht::prhdtls::DataHolder<std::vector<int>, false>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using embed_kv_metall =
    perroht::prhdtls::DataHolder<int, true,
                                 metall::manager::allocator_type<int>>;
using node_kv_metall =
    perroht::prhdtls::DataHolder<int, false,
                                 metall::manager::allocator_type<int>>;
using embed_k_metall = perroht::prhdtls::DataHolder<
    std::vector<int>, true, metall::manager::allocator_type<std::vector<int>>>;
using node_k_metall = perroht::prhdtls::DataHolder<
    std::vector<int>, false, metall::manager::allocator_type<std::vector<int>>>;
#endif

static constexpr const char *kMetallDataStorePath = "./test-data-holder";

template <typename T>
class DataHolderTest_KeyValue : public ::testing::Test {
  void create(embed_kv *&d) {
    a = new typename T::AllocatorType();
    d = new embed_kv(*a, 10);
  }

  void create(node_kv *&d) {
    a = new typename T::AllocatorType();
    d = new node_kv(*a, 10);
  }

  void destroy(embed_kv *&d) {
    d->Clear(*a);
    delete d;
    delete a;
  }

  void destroy(node_kv *&d) {
    d->Clear(*a);
    delete d;
    delete a;
  }

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  void create(embed_kv_metall *&d) {
    manager = new metall::manager(metall::create_only, kMetallDataStorePath);
    a = new typename T::AllocatorType(manager->get_allocator<int>());
    d = manager->construct<embed_kv_metall>("perroht")(*a, 10);
  }

  void create(node_kv_metall *&d) {
    manager = new metall::manager(metall::create_only, kMetallDataStorePath);
    a = new typename T::AllocatorType(manager->get_allocator<int>());
    d = manager->construct<node_kv_metall>("perroht")(*a, 10);
  }

  void destroy(embed_kv_metall *&d) {
    d->Clear(*a);
    manager->destroy_ptr<embed_kv_metall>(d);
    delete a;
    delete manager;
  }

  void destroy(node_kv_metall *&d) {
    d->Clear(*a);
    manager->destroy_ptr<node_kv_metall>(d);
    delete a;
    delete manager;
  }
#endif

 protected:
  void SetUp() override { create(data_); }
  void TearDown() override { destroy(data_); }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  metall::manager *manager;
#endif
  T *data_;
  typename T::AllocatorType *a;
};

template <typename T>
class DataHolderTest_Key : public ::testing::Test {
  void create(embed_k *&d) {
    a = new typename T::AllocatorType();
    d = new embed_k(*a, 10);
  }

  void create(node_k *&d) {
    a = new typename T::AllocatorType();
    d = new node_k(*a, 10);
  }

  void destroy(embed_k *&d) {
    d->Clear(*a);
    delete d;
    delete a;
  }

  void destroy(node_k *&d) {
    d->Clear(*a);
    delete d;
    delete a;
  }

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  void create(embed_k_metall *&d) {
    manager = new metall::manager(metall::create_only, kMetallDataStorePath);
    a = new typename T::AllocatorType(manager->get_allocator<int>());
    d = manager->construct<embed_k_metall>("perroht")(*a, 10);
  }

  void create(node_k_metall *&d) {
    manager = new metall::manager(metall::create_only, kMetallDataStorePath);
    a = new typename T::AllocatorType(manager->get_allocator<int>());
    d = manager->construct<node_k_metall>("perroht")(*a, 10);
  }

  void destroy(embed_k_metall *&d) {
    d->Clear(*a);
    manager->destroy_ptr<embed_k_metall>(d);
    delete a;
    delete manager;
  }

  void destroy(node_k_metall *&d) {
    d->Clear(*a);
    manager->destroy_ptr<node_k_metall>(d);
    delete a;
    delete manager;
  }
#endif

 protected:
  void SetUp() override { create(data_); }
  void TearDown() override { destroy(data_); }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  metall::manager *manager;
#endif
  T *data_;
  typename T::AllocatorType *a;
};

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using MapTypes_KeyValue =
    ::testing::Types<embed_kv, embed_kv_metall, node_kv, node_kv_metall>;
using MapTypes_Key =
    ::testing::Types<embed_k, embed_k_metall, node_k, node_k_metall>;
#else
using MapTypes_KeyValue = ::testing::Types<embed_kv, node_kv>;
using MapTypes_Key = ::testing::Types<embed_k, node_k>;
#endif

TYPED_TEST_SUITE(DataHolderTest_KeyValue, MapTypes_KeyValue);
TYPED_TEST_SUITE(DataHolderTest_Key, MapTypes_Key);

TYPED_TEST(DataHolderTest_KeyValue, DefaultConstructInPlaceor) {}

TYPED_TEST(DataHolderTest_Key, DefaultConstructInPlaceor) {}

TEST(DataHolderTest, DefaultConstructInPlaceor) {
  { perroht::prhdtls::DataHolder<int, true> data; }

  { perroht::prhdtls::DataHolder<int, false> data; }

  { perroht::prhdtls::DataHolder<std::vector<int>, true> data; }

  { perroht::prhdtls::DataHolder<std::vector<int>, false> data; }
}

TEST(DataHolderTest, Size) {
  EXPECT_EQ(sizeof(perroht::prhdtls::DataHolder<char, true>), sizeof(char));
  EXPECT_EQ(sizeof(perroht::prhdtls::DataHolder<char, false>),
            sizeof(perroht::prhdtls::DataHolder<char, false>::Pointer));
}

TYPED_TEST(DataHolderTest_KeyValue, ConstructInPlace) {
  TypeParam data;
  auto alloc = *(this->a);
  TypeParam::ConstructInPlace(alloc, &data, 10);
  EXPECT_EQ(data.Get(), 10);
  const auto &data_const = data;
  EXPECT_EQ(data_const.Get(), 10);
  data.Clear(alloc);
}

TYPED_TEST(DataHolderTest_Key, ConstructInPlace) {
  TypeParam data;
  auto alloc = *(this->a);
  TypeParam::ConstructInPlace(alloc, &data, 10);
  EXPECT_EQ(data.Get().size(), 10);
  const auto &data_const = data;
  EXPECT_EQ(data_const.Get().size(), 10);
  data.Clear(alloc);
}

TYPED_TEST(DataHolderTest_KeyValue, MoveConstruct) {
  TypeParam &data = *(this->data_);
  auto alloc = *(this->a);
  TypeParam data2(std::move(data));
  EXPECT_EQ(data2.Get(), 10);
  data2.Clear(alloc);
}

TYPED_TEST(DataHolderTest_Key, MoveConstruct) {
  TypeParam &data = *(this->data_);
  auto alloc = *(this->a);
  TypeParam data2(std::move(data));
  EXPECT_EQ(data2.Get().size(), 10);
  data2.Clear(alloc);
}

TYPED_TEST(DataHolderTest_KeyValue, MoveAssign) {
  TypeParam &data = *(this->data_);
  auto alloc = *(this->a);
  TypeParam data2(alloc, 20);
  data2.MoveAssign(alloc, std::move(data));
  EXPECT_EQ(data2.Get(), 10);
  data2.Clear(alloc);
}

TYPED_TEST(DataHolderTest_Key, MoveAssign) {
  TypeParam &data = *(this->data_);
  auto alloc = *(this->a);
  TypeParam data2(alloc, 20);
  data2.MoveAssign(alloc, std::move(data));
  EXPECT_EQ(data2.Get().size(), 10);
  data2.Clear(alloc);
}

TYPED_TEST(DataHolderTest_KeyValue, Swap) {
  TypeParam &data = *(this->data_);
  auto alloc = *(this->a);
  TypeParam data2(alloc, 20);
  using std::swap;
  swap(data, data2);
  EXPECT_EQ(data.Get(), 20);
  EXPECT_EQ(data2.Get(), 10);
  data2.Clear(alloc);
}

TYPED_TEST(DataHolderTest_Key, Swap) {
  TypeParam &data = *(this->data_);
  auto alloc = *(this->a);
  TypeParam data2(alloc, 20);
  using std::swap;
  swap(data, data2);
  EXPECT_EQ(data.Get().size(), 20);
  EXPECT_EQ(data2.Get().size(), 10);
  data2.Clear(alloc);
}

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
template <typename T>
class KeyValueMetallOffsetPointerTest : public ::testing::Test {};
using KeyValueMetallTypes = ::testing::Types<embed_kv_metall, node_kv_metall>;
TYPED_TEST_SUITE(KeyValueMetallOffsetPointerTest, KeyValueMetallTypes);

TYPED_TEST(KeyValueMetallOffsetPointerTest, Insert) {
  {
    metall::manager manager(metall::create_only, kMetallDataStorePath);
    auto alloc = manager.get_allocator<int>();
    auto data = manager.construct<TypeParam>("data")(alloc, 17);
    EXPECT_EQ(data->Get(), 17);
  }
  {
    metall::manager manager(metall::open_read_only, kMetallDataStorePath);
    auto data = manager.find<TypeParam>("data").first;
    EXPECT_EQ(data->Get(), 17);
  }
}

template <typename T>
class KeyMetallOffsetPointerTest : public ::testing::Test {};
using KeyMetallTypes = ::testing::Types<embed_k_metall, node_k_metall>;
TYPED_TEST_SUITE(KeyMetallOffsetPointerTest, KeyMetallTypes);

TYPED_TEST(KeyMetallOffsetPointerTest, Insert) {
  {
    metall::manager manager(metall::create_only, kMetallDataStorePath);
    auto alloc = manager.get_allocator<std::vector<int>>();
    auto data = manager.construct<TypeParam>("data")(alloc, 17);
    EXPECT_EQ(data->Get().size(), 17);
  }
  {
    metall::manager manager(metall::open_read_only, kMetallDataStorePath);
    auto data = manager.find<TypeParam>("data").first;
    EXPECT_EQ(data->Get().size(), 17);
  }
}
#endif