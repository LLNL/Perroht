// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <perroht/unordered_map.hpp>

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
#include <metall/metall.hpp>
#include <metall/container/vector.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#endif

#include <utility>
#include <memory>

#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
#include <boost/unordered_map.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_node_map.hpp>
#endif

using testing::ElementsAre;
using testing::Not;
using testing::Pair;
using testing::WhenSorted;

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
namespace bip = boost::interprocess;
template <typename T>
using bip_allocator =
    bip::allocator<T, bip::managed_mapped_file::segment_manager>;
#endif

using flat_map =
    perroht::unordered_flat_map<int, int, std::hash<int>, std::equal_to<int>,
                                std::allocator<int>>;
using node_map =
    perroht::unordered_node_map<int, int, std::hash<int>, std::equal_to<int>,
                                std::allocator<int>>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using flat_map_metall =
    perroht::unordered_flat_map<int, int, std::hash<int>, std::equal_to<int>,
                                metall::manager::allocator_type<int>>;
using flat_map_bip =
    perroht::unordered_flat_map<int, int, std::hash<int>, std::equal_to<int>,
                                bip_allocator<int>>;
using node_map_metall =
    perroht::unordered_node_map<int, int, std::hash<int>, std::equal_to<int>,
                                metall::manager::allocator_type<int>>;
using node_map_bip =
    perroht::unordered_node_map<int, int, std::hash<int>, std::equal_to<int>,
                                bip_allocator<int>>;
#endif
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
using u_map = boost::unordered_map<int, int, std::hash<int>, std::equal_to<int>,
                                   std::allocator<int>>;
using b_flat_map =
    boost::unordered_flat_map<int, int, std::hash<int>, std::equal_to<int>,
                              std::allocator<int>>;
using b_node_map =
    boost::unordered_node_map<int, int, std::hash<int>, std::equal_to<int>,
                              std::allocator<int>>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using u_map_metall =
    boost::unordered_map<int, int, std::hash<int>, std::equal_to<int>,
                         metall::manager::allocator_type<int>>;
using u_map_bip = boost::unordered_map<int, int, std::hash<int>,
                                       std::equal_to<int>, bip_allocator<int>>;
using b_flat_map_metall =
    boost::unordered_flat_map<int, int, std::hash<int>, std::equal_to<int>,
                              metall::manager::allocator_type<int>>;
using b_flat_map_bip =
    boost::unordered_flat_map<int, int, std::hash<int>, std::equal_to<int>,
                              bip_allocator<int>>;
using b_node_map_metall =
    boost::unordered_node_map<int, int, std::hash<int>, std::equal_to<int>,
                              metall::manager::allocator_type<int>>;
using b_node_map_bip =
    boost::unordered_node_map<int, int, std::hash<int>, std::equal_to<int>,
                              bip_allocator<int>>;
#endif
#endif

static constexpr const char* kMetallDataStorePath = "./test-unordered-map";
static constexpr const char* kBIPDataStorePath = "./test-unordered-map-bip";

#define CTOR_DTOR_PERSISTENT(TYPE)                                           \
  void create(TYPE##_metall*& m) {                                           \
    manager = new metall::manager(metall::create_only, kMetallDataStorePath, \
                                  1 << 30);                                  \
    m = manager->construct<TYPE##_metall>("unordered_map")(                  \
        manager->get_allocator());                                           \
  }                                                                          \
  void create(TYPE##_bip*& m) {                                              \
    bip::file_mapping::remove(kBIPDataStorePath);                            \
    b_manager = new bip::managed_mapped_file(bip::create_only,               \
                                             kBIPDataStorePath, 1 << 23);    \
    m = b_manager->construct<TYPE##_bip>("unordered_map")(                   \
        b_manager->get_allocator<int>());                                    \
  }                                                                          \
  void destroy(TYPE##_metall*& m) {                                          \
    manager->destroy_ptr<TYPE##_metall>(m);                                  \
    delete manager;                                                          \
  }                                                                          \
  void destroy(TYPE##_bip*& m) {                                             \
    b_manager->destroy_ptr<TYPE##_bip>(m);                                   \
    delete b_manager;                                                        \
  }

template <typename T>
class UnorderedMap : public ::testing::Test {
  void create(flat_map*& m) { m = new flat_map(); }
  void create(node_map*& m) { m = new node_map(); }
  void destroy(flat_map*& m) { delete m; }
  void destroy(node_map*& m) { delete m; }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  CTOR_DTOR_PERSISTENT(flat_map);
  CTOR_DTOR_PERSISTENT(node_map);
#endif

#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
  void create(u_map*& m) { m = new u_map(); }
  void create(b_flat_map*& m) { m = new b_flat_map(); }
  void create(b_node_map*& m) { m = new b_node_map(); }
  void destroy(u_map*& m) { delete m; }
  void destroy(b_flat_map*& m) { delete m; }
  void destroy(b_node_map*& m) { delete m; }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  CTOR_DTOR_PERSISTENT(u_map);
  CTOR_DTOR_PERSISTENT(b_flat_map);
  CTOR_DTOR_PERSISTENT(b_node_map);
#endif
#endif

 protected:
  void SetUp() override { create(map_); }
  void TearDown() override { destroy(map_); }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  metall::manager* manager;
  bip::managed_mapped_file* b_manager;
#endif
  T* map_;
};

using MapTypes =
    ::testing::Types<flat_map, node_map
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
                     ,
                     flat_map_metall, flat_map_bip, node_map_metall,
                     node_map_bip
#endif
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
                     ,
                     u_map, b_flat_map, b_node_map
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
                     ,
                     u_map_metall, u_map_bip, b_flat_map_metall, b_flat_map_bip,
                     b_node_map_metall, b_node_map_bip
#endif
#endif
                     >;
TYPED_TEST_SUITE(UnorderedMap, MapTypes);

//TYPED_TEST(UnorderedMap, DefaultConstructor) { TypeParam* map = this->map_; }

TYPED_TEST(UnorderedMap, CopyConstructor) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(1, 11));
  map->insert(std::make_pair<int, int>(2, 22));
  map->insert(std::make_pair<int, int>(3, 33));
  map->insert(std::make_pair<int, int>(4, 44));
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22),
                                           Pair(3, 33), Pair(4, 44))));
  TypeParam copy_map(*map);
  EXPECT_THAT(copy_map, WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22),
                                               Pair(3, 33), Pair(4, 44))));
}

TYPED_TEST(UnorderedMap, MoveConstructor) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(1, 11));
  map->insert(std::make_pair<int, int>(2, 22));
  map->insert(std::make_pair<int, int>(3, 33));
  map->insert(std::make_pair<int, int>(4, 44));
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22),
                                           Pair(3, 33), Pair(4, 44))));
  TypeParam move_map(std::move(*map));
  EXPECT_THAT(move_map, WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22),
                                               Pair(3, 33), Pair(4, 44))));
}

TYPED_TEST(UnorderedMap, Insert) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(1, 1));
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1))));
  auto kv = std::make_pair<int, int>(2, 2);
  map->insert(kv);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2))));
  EXPECT_EQ(map->size(), 2);
  EXPECT_TRUE(map->contains(1));
  EXPECT_TRUE(map->contains(2));
  // Duplicate element should not be assigned
  map->insert(std::make_pair<int, int>(1, 7));
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2))));
  EXPECT_EQ(map->size(), 2);
  EXPECT_TRUE(map->contains(1));
  EXPECT_TRUE(map->contains(2));
}

TYPED_TEST(UnorderedMap, InsertAndFind) {
  TypeParam* map = this->map_;

  map->insert(std::make_pair<int, int>(0, 10));
  map->insert(std::make_pair<int, int>(1, 11));
  map->insert(std::make_pair<int, int>(2, 12));
  map->insert(std::make_pair<int, int>(3, 13));

  EXPECT_EQ(map->size(), 4);
  EXPECT_GE(map->bucket_count(), 4);

  auto ret = map->find(0);
  EXPECT_EQ(ret->first, 0);
  EXPECT_EQ(ret->second, 10);
  ret = map->find(1);
  EXPECT_EQ(ret->first, 1);
  EXPECT_EQ(ret->second, 11);
  ret = map->find(2);
  EXPECT_EQ(ret->first, 2);
  EXPECT_EQ(ret->second, 12);
  ret = map->find(3);
  EXPECT_EQ(ret->first, 3);
  EXPECT_EQ(ret->second, 13);
}

TYPED_TEST(UnorderedMap, Emplace) {
  TypeParam* map = this->map_;
  map->emplace(1, 1);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1))));
  map->emplace(2, 2);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2))));
  EXPECT_EQ(map->size(), 2);
  EXPECT_TRUE(map->contains(1));
  EXPECT_TRUE(map->contains(2));
  // Duplicate element should not be assigned
  map->emplace(1, 7);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2))));
  EXPECT_EQ(map->size(), 2);
  EXPECT_TRUE(map->contains(1));
  EXPECT_TRUE(map->contains(2));
}

TYPED_TEST(UnorderedMap, TryEmplace) {
  TypeParam* map = this->map_;
  map->try_emplace(1, 1);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1))));
  map->try_emplace(2, 2);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2))));
  EXPECT_EQ(map->size(), 2);
  EXPECT_TRUE(map->contains(1));
  EXPECT_TRUE(map->contains(2));
  // Duplicate element should not be assigned
  map->try_emplace(1, 7);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2))));
  EXPECT_EQ(map->size(), 2);
  EXPECT_TRUE(map->contains(1));
  EXPECT_TRUE(map->contains(2));
  map->try_emplace(5, 3);
  map->try_emplace(7, 8);
  map->try_emplace(9, 1);
  map->try_emplace(8, 4);
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2), Pair(5, 3),
                                     Pair(7, 8), Pair(8, 4), Pair(9, 1))));
}

TYPED_TEST(UnorderedMap, Count) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(1, 1));
  map->insert(std::make_pair<int, int>(2, 2));
  map->insert(std::make_pair<int, int>(3, 3));
  EXPECT_EQ(map->count(1), 1);
  EXPECT_EQ(map->count(2), 1);
  EXPECT_EQ(map->count(3), 1);
  EXPECT_EQ(map->count(4), 0);
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2), Pair(3, 3))));
  EXPECT_THAT(*map, Not(WhenSorted(ElementsAre(Pair(4, 6)))));
}

TYPED_TEST(UnorderedMap, Erase) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(1, 1));
  map->insert(std::make_pair<int, int>(2, 2));
  map->insert(std::make_pair<int, int>(3, 3));
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2), Pair(3, 3))));
  EXPECT_THAT(*map, Not(WhenSorted(ElementsAre(Pair(4, 6)))));
  map->erase(1);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(2, 2), Pair(3, 3))));
  EXPECT_THAT(*map, Not(WhenSorted(ElementsAre(Pair(1, 1), Pair(4, 6)))));
  map->erase(2);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(3, 3))));
  EXPECT_THAT(*map,
              Not(WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2), Pair(4, 6)))));
  map->erase(3);
  EXPECT_THAT(*map, Not(WhenSorted(ElementsAre(Pair(1, 1), Pair(2, 2),
                                               Pair(3, 3), Pair(4, 6)))));
}

TYPED_TEST(UnorderedMap, Clear) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(0, 10));
  map->insert(std::make_pair<int, int>(1, 11));
  map->insert(std::make_pair<int, int>(2, 12));
  map->insert(std::make_pair<int, int>(3, 13));
  EXPECT_FALSE(map->empty());
  map->clear();
  EXPECT_EQ(map->size(), 0);
  EXPECT_GE(map->bucket_count(), 4);
  EXPECT_TRUE(map->empty());

  // Can insert again
  EXPECT_TRUE(map->insert(std::make_pair<int, int>(0, 10)).second);
  EXPECT_TRUE(map->insert(std::make_pair<int, int>(1, 11)).second);
}

template <typename T>
void TestIteratorHelper(T& it) {
  EXPECT_EQ(it->first, 0);
  EXPECT_EQ(it->second, 10);

  ++it;
  EXPECT_EQ(it->first, 1);
  EXPECT_EQ(it->second, 11);

  ++it;
  EXPECT_EQ(it->first, 2);
  EXPECT_EQ(it->second, 12);

  it++;
  EXPECT_EQ(it->first, 3);
  EXPECT_EQ(it->second, 13);

  const auto old_it = it++;
  EXPECT_EQ(old_it->first, 3);
  EXPECT_EQ(old_it->second, 13);
}

TYPED_TEST(UnorderedMap, Iterator) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(0, 10));
  map->insert(std::make_pair<int, int>(1, 11));
  map->insert(std::make_pair<int, int>(2, 12));
  map->insert(std::make_pair<int, int>(3, 13));
  {
    auto it = map->begin();
    SCOPED_TRACE("begin/end");
    TestIteratorHelper(it);
    EXPECT_EQ(it, map->end());
  }

  {
    auto it = map->cbegin();
    SCOPED_TRACE("cbegin/cend");
    TestIteratorHelper(it);
    EXPECT_EQ(it, map->cend());
  }

  const auto& const_map = map;
  {
    auto it = const_map->begin();
    SCOPED_TRACE("begin/end (const)");
    TestIteratorHelper(it);
    EXPECT_EQ(it, const_map->end());
  }

  {
    auto it = const_map->cbegin();
    SCOPED_TRACE("cbegin/cend (const)");
    TestIteratorHelper(it);
    EXPECT_EQ(it, const_map->cend());
  }
}

TYPED_TEST(UnorderedMap, ReserveAfterEmpty) {
  TypeParam* map = this->map_;
  map->reserve(100);
  EXPECT_GE(map->bucket_count(), 100);
}

TYPED_TEST(UnorderedMap, ReserveAfterinsertion) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(0, 10));
  map->insert(std::make_pair<int, int>(1, 11));
  map->reserve(100);
  EXPECT_GE(map->bucket_count(), 100);

  // Make sure existing elements are still there
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(0, 10), Pair(1, 11))));
  EXPECT_EQ(map->size(), 2);

  // Make sure we can still insert
  EXPECT_TRUE(map->insert(std::make_pair<int, int>(2, 12)).second);
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(0, 10), Pair(1, 11), Pair(2, 12))));

  // Reserve behavior should be using rehash() as defined in stl documentation
  const auto old_capacity = map->bucket_count();
  map->reserve(old_capacity);
  EXPECT_EQ(map->bucket_count(), old_capacity);
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(0, 10), Pair(1, 11), Pair(2, 12))));

  map->reserve(1);
  EXPECT_EQ(map->size(), 3);
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(0, 10), Pair(1, 11), Pair(2, 12))));
}

TYPED_TEST(UnorderedMap, RehashAfterEmpty) {
  TypeParam* map = this->map_;
  map->rehash(100);
  EXPECT_GE(map->bucket_count(), 100);
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

using flat_map_custom_hash =
    perroht::unordered_flat_map<int, int, CustomHash, std::equal_to<int>,
                                std::allocator<int>>;
using node_map_custom_hash =
    perroht::unordered_node_map<int, int, CustomHash, std::equal_to<int>,
                                std::allocator<int>>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using flat_map_custom_hash_metall =
    perroht::unordered_flat_map<int, int, CustomHash, std::equal_to<int>,
                                metall::manager::allocator_type<int>>;
using flat_map_custom_hash_bip =
    perroht::unordered_flat_map<int, int, CustomHash, std::equal_to<int>,
                                bip_allocator<int>>;
using node_map_custom_hash_metall =
    perroht::unordered_node_map<int, int, CustomHash, std::equal_to<int>,
                                metall::manager::allocator_type<int>>;
using node_map_custom_hash_bip =
    perroht::unordered_node_map<int, int, CustomHash, std::equal_to<int>,
                                bip_allocator<int>>;
#endif
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
using u_map_custom_hash =
    boost::unordered_map<int, int, CustomHash, std::equal_to<int>,
                         std::allocator<int>>;
using b_flat_map_custom_hash =
    boost::unordered_flat_map<int, int, CustomHash, std::equal_to<int>,
                              std::allocator<int>>;
using b_node_map_custom_hash =
    boost::unordered_node_map<int, int, CustomHash, std::equal_to<int>,
                              std::allocator<int>>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using u_map_custom_hash_metall =
    boost::unordered_map<int, int, CustomHash, std::equal_to<int>,
                         metall::manager::allocator_type<int>>;
using u_map_custom_hash_bip =
    boost::unordered_map<int, int, CustomHash, std::equal_to<int>,
                         bip_allocator<int>>;
using b_flat_map_custom_hash_metall =
    boost::unordered_flat_map<int, int, CustomHash, std::equal_to<int>,
                              metall::manager::allocator_type<int>>;
using b_flat_map_custom_hash_bip =
    boost::unordered_flat_map<int, int, CustomHash, std::equal_to<int>,
                              bip_allocator<int>>;
using b_node_map_custom_hash_metall =
    boost::unordered_node_map<int, int, CustomHash, std::equal_to<int>,
                              metall::manager::allocator_type<int>>;
using b_node_map_custom_hash_bip =
    boost::unordered_node_map<int, int, CustomHash, std::equal_to<int>,
                              bip_allocator<int>>;
#endif
#endif

template <typename T>
class UnorderedMapCustomHash : public ::testing::Test {
  void create(flat_map_custom_hash*& m) { m = new flat_map_custom_hash(); }
  void create(node_map_custom_hash*& m) { m = new node_map_custom_hash(); }
  void destroy(flat_map_custom_hash*& m) { delete m; }
  void destroy(node_map_custom_hash*& m) { delete m; }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  CTOR_DTOR_PERSISTENT(flat_map_custom_hash);
  CTOR_DTOR_PERSISTENT(node_map_custom_hash);
#endif

#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
  void create(u_map_custom_hash*& m) { m = new u_map_custom_hash(); }
  void create(b_flat_map_custom_hash*& m) { m = new b_flat_map_custom_hash(); }
  void create(b_node_map_custom_hash*& m) { m = new b_node_map_custom_hash(); }
  void destroy(u_map_custom_hash*& m) { delete m; }
  void destroy(b_flat_map_custom_hash*& m) { delete m; }
  void destroy(b_node_map_custom_hash*& m) { delete m; }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  CTOR_DTOR_PERSISTENT(u_map_custom_hash);
  CTOR_DTOR_PERSISTENT(b_flat_map_custom_hash);
  CTOR_DTOR_PERSISTENT(b_node_map_custom_hash);
#endif
#endif

 protected:
  void SetUp() override { create(map_); }
  void TearDown() override { destroy(map_); }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  metall::manager* manager;
  bip::managed_mapped_file* b_manager;
#endif
  T* map_;
};

using MapTypesCustomHash =
    ::testing::Types<flat_map_custom_hash, node_map_custom_hash
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
                     ,
                     flat_map_custom_hash_metall, flat_map_custom_hash_bip,
                     node_map_custom_hash_metall, node_map_custom_hash_bip
#endif
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_map_custom_hash_TEST
                     ,
                     u_map_custom_hash, b_flat_map_custom_hash,
                     b_node_map_custom_hash
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
                     ,
                     u_map_custom_hash_metall, u_map_custom_hash_bip,
                     b_flat_map_custom_hash_metall, b_flat_map_custom_hash_bip,
                     b_node_map_custom_hash_metall, b_node_map_custom_hash_bip
#endif
#endif
                     >;
TYPED_TEST_SUITE(UnorderedMapCustomHash, MapTypesCustomHash);

TYPED_TEST(UnorderedMapCustomHash, Insert) {
  TypeParam* map = this->map_;

  map->insert(std::make_pair<int, int>(0, 10));
  map->insert(std::make_pair<int, int>(1, 11));
  map->insert(std::make_pair<int, int>(2, 12));
  map->insert(std::make_pair<int, int>(3, 13));

  EXPECT_EQ(map->size(), 4);
  EXPECT_GE(map->bucket_count(), 4);

  auto ret = map->find(0);
  EXPECT_EQ(ret->first, 0);
  EXPECT_EQ(ret->second, 10);
  ret = map->find(1);
  EXPECT_EQ(ret->first, 1);
  EXPECT_EQ(ret->second, 11);
  ret = map->find(2);
  EXPECT_EQ(ret->first, 2);
  EXPECT_EQ(ret->second, 12);
  ret = map->find(3);
  EXPECT_EQ(ret->first, 3);
  EXPECT_EQ(ret->second, 13);
}

TYPED_TEST(UnorderedMap, RehashAfterinsertion) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(0, 10));
  map->insert(std::make_pair<int, int>(1, 11));
  map->rehash(100);
  EXPECT_GE(map->bucket_count(), 100);

  // Make sure existing elements are still there
  EXPECT_EQ(map->count(0), 1);
  EXPECT_EQ(map->count(1), 1);
  EXPECT_EQ(map->size(), 2);

  // Make sure we can still insert
  EXPECT_TRUE(map->insert(std::make_pair<int, int>(2, 12)).second);
  EXPECT_EQ(map->count(2), 1);

  // Make sure rehash() does not do anything if the capacity is already
  // large enough
  const auto old_capacity = map->bucket_count();
  map->rehash(old_capacity);
  EXPECT_EQ(map->bucket_count(), old_capacity);
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(0, 10), Pair(1, 11), Pair(2, 12))));

  map->rehash(1);
  EXPECT_GE(map->bucket_count(), 3);
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(0, 10), Pair(1, 11), Pair(2, 12))));
}
//*
TYPED_TEST(UnorderedMap, EraseUsingIteratorFind) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(0, 10));
  map->insert(std::make_pair<int, int>(1, 11));
  auto itr = map->erase(map->find(1));
  EXPECT_TRUE(itr == map->find(0) || itr == map->end());
  EXPECT_EQ(map->erase(map->find(0)), map->end());
}

TYPED_TEST(UnorderedMap, EraseUsingIteratorBegin) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair<int, int>(0, 10));
  map->insert(std::make_pair<int, int>(1, 11));
  EXPECT_NE(map->erase(map->cbegin()), map->end());
  EXPECT_EQ(map->erase(map->cbegin()), map->end());
}
//*/
TYPED_TEST(UnorderedMap, LoadFactor) {
  TypeParam* map = this->map_;
  EXPECT_DOUBLE_EQ(map->load_factor(), 0.0);

  map->insert(std::make_pair<int, int>(0, 10));
  EXPECT_GE(map->load_factor(), 0.0);
  EXPECT_LE(map->load_factor(), map->max_load_factor());

  map->insert(std::make_pair<int, int>(1, 11));
  EXPECT_GE(map->load_factor(), 0.0);
  EXPECT_LE(map->load_factor(), map->max_load_factor());
}

TYPED_TEST(UnorderedMap, Swap) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair(1, 11));
  map->insert(std::make_pair(2, 22));
  map->insert(std::make_pair(3, 33));
  TypeParam map1(*map);
  map->clear();
  map->insert(std::make_pair(4, 44));
  map->insert(std::make_pair(5, 55));
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(4, 44), Pair(5, 55))));
  EXPECT_THAT(map1,
              WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22), Pair(3, 33))));
  map->swap(map1);
  EXPECT_THAT(*map,
              WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22), Pair(3, 33))));
  EXPECT_THAT(map1, WhenSorted(ElementsAre(Pair(4, 44), Pair(5, 55))));
  swap(*map, map1);
  EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(4, 44), Pair(5, 55))));
  EXPECT_THAT(map1,
              WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22), Pair(3, 33))));
}

TYPED_TEST(UnorderedMap, Equality) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair(1, 11));
  map->insert(std::make_pair(2, 22));
  map->insert(std::make_pair(3, 33));
  TypeParam map1(*map);
  EXPECT_TRUE(*map == map1);
  EXPECT_TRUE(map1 == *map);
  EXPECT_TRUE(*map == *map);
  EXPECT_TRUE(map1 == map1);
}

TYPED_TEST(UnorderedMap, Inequality) {
  TypeParam* map = this->map_;
  map->insert(std::make_pair(1, 11));
  map->insert(std::make_pair(2, 22));
  map->insert(std::make_pair(3, 33));
  TypeParam map1(*map);
  map1.erase(3);
  EXPECT_TRUE(*map != map1);
  EXPECT_TRUE(map1 != *map);
  EXPECT_TRUE(*map == *map);
  EXPECT_TRUE(map1 == map1);
}

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using metall::container::scoped_allocator_adaptor;
using metall::container::vector;

template <typename T>
using flat_map_metall_scoped = perroht::unordered_flat_map<
    int, T, std::hash<int>, std::equal_to<int>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
template <typename T>
using node_map_metall_scoped = perroht::unordered_node_map<
    int, T, std::hash<int>, std::equal_to<int>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
template <typename T>
using u_map_metall_scoped = boost::unordered_map<
    int, T, std::hash<int>, std::equal_to<int>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
template <typename T>
using b_flat_map_metall_scoped = perroht::unordered_flat_map<
    int, T, std::hash<int>, std::equal_to<int>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
template <typename T>
using b_node_map_metall_scoped = perroht::unordered_node_map<
    int, T, std::hash<int>, std::equal_to<int>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
#endif

template <typename T>
class MetallNestedInnerMap : public ::testing::Test {};

#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
using InnerMetallMapTypes =
    ::testing::Types<flat_map_metall, node_map_metall, u_map_metall,
                     b_flat_map_metall, b_node_map_metall>;
#else
using InnerMetallMapTypes = ::testing::Types<flat_map_metall, node_map_metall>;
#endif

TYPED_TEST_SUITE(MetallNestedInnerMap, InnerMetallMapTypes);

TYPED_TEST(MetallNestedInnerMap, VectorOfMap) {
  using outer_vector_type = vector<
      TypeParam,
      scoped_allocator_adaptor<metall::manager::allocator_type<TypeParam>>>;

  metall::manager manager(metall::create_only, kMetallDataStorePath, 1 << 30);
  auto vec = manager.construct<outer_vector_type>("vec-of-map")(
      manager.get_allocator<>());

  auto inner_map_0 =
      manager.construct<TypeParam>("inner-map-0")(manager.get_allocator<>());
  vec->push_back(*inner_map_0);
  vec->at(0).insert(std::make_pair(1, 11));
  vec->at(0).insert(std::make_pair(2, 22));
  vec->at(0).insert(std::make_pair(3, 33));
  vec->at(0).insert(std::make_pair(4, 44));

  auto inner_map_1 =
      manager.construct<TypeParam>("inner-map-1")(manager.get_allocator<>());
  vec->push_back(*inner_map_1);
  vec->at(1).insert(std::make_pair(6, 66));
  vec->at(1).insert(std::make_pair(7, 77));
  vec->at(1).insert(std::make_pair(8, 88));

  EXPECT_THAT((*vec)[0], WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22),
                                                Pair(3, 33), Pair(4, 44))));
  EXPECT_THAT(vec->at(1),
              WhenSorted(ElementsAre(Pair(6, 66), Pair(7, 77), Pair(8, 88))));

  EXPECT_EQ((*vec)[0][1], 11);
  EXPECT_EQ((*vec)[0].at(1), 11);
  EXPECT_EQ(vec->at(0)[1], 11);
  EXPECT_EQ(vec->at(0).at(1), 11);
}

template <typename T>
class MetallNestedOuterMap : public ::testing::Test {};

using OuterMetallMapTypes =
    ::testing::Types<flat_map_metall_scoped<metall::container::vector<int>>,
                     node_map_metall_scoped<metall::container::vector<int>>
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
                     ,
                     u_map_metall_scoped<metall::container::vector<int>>,
                     b_flat_map_metall_scoped<metall::container::vector<int>>,
                     b_node_map_metall_scoped<metall::container::vector<int>>
#endif
                     >;

TYPED_TEST_SUITE(MetallNestedOuterMap, OuterMetallMapTypes);

TYPED_TEST(MetallNestedOuterMap, MapOfVector) {
  metall::manager manager(metall::create_only, kMetallDataStorePath, 1 << 30);
  auto map =
      manager.construct<TypeParam>("map-of-vec")(manager.get_allocator<>());

  (*map)[0];
  map->at(0).push_back(1);
  map->at(0).push_back(2);
  map->at(0).push_back(3);
  map->at(0).push_back(4);

  (*map)[5].push_back(6);
  (*map)[5].push_back(7);
  (*map)[5].push_back(8);

  EXPECT_THAT((*map)[0], WhenSorted(ElementsAre(1, 2, 3, 4)));
  EXPECT_THAT(map->at(5), WhenSorted(ElementsAre(6, 7, 8)));

  EXPECT_EQ((*map)[0][1], 2);
  EXPECT_EQ((*map)[0].at(1), 2);
  EXPECT_EQ(map->at(0)[1], 2);
  EXPECT_EQ(map->at(0).at(1), 2);
}

template <typename T>
class MetallNestedMap : public ::testing::Test {};

using MetallScopedMapTypes = ::testing::Types<
    flat_map_metall_scoped<flat_map_metall>,
    flat_map_metall_scoped<node_map_metall>,
    node_map_metall_scoped<flat_map_metall>,
    node_map_metall_scoped<node_map_metall>
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
    ,
    flat_map_metall_scoped<u_map_metall>,
    flat_map_metall_scoped<b_flat_map_metall>,
    flat_map_metall_scoped<b_node_map_metall>,
    node_map_metall_scoped<u_map_metall>,
    node_map_metall_scoped<b_flat_map_metall>,
    node_map_metall_scoped<b_node_map_metall>,
    u_map_metall_scoped<flat_map_metall>, u_map_metall_scoped<node_map_metall>,
    u_map_metall_scoped<u_map_metall>, u_map_metall_scoped<b_flat_map_metall>,
    u_map_metall_scoped<b_node_map_metall>,
    b_flat_map_metall_scoped<flat_map_metall>,
    b_flat_map_metall_scoped<node_map_metall>,
    b_flat_map_metall_scoped<u_map_metall>,
    b_flat_map_metall_scoped<b_flat_map_metall>,
    b_flat_map_metall_scoped<b_node_map_metall>,
    b_node_map_metall_scoped<flat_map_metall>,
    b_node_map_metall_scoped<node_map_metall>,
    b_node_map_metall_scoped<u_map_metall>,
    b_node_map_metall_scoped<b_flat_map_metall>,
    b_node_map_metall_scoped<b_node_map_metall>
#endif
    >;

TYPED_TEST_SUITE(MetallNestedMap, MetallScopedMapTypes);

TYPED_TEST(MetallNestedMap, MapOfContainer) {
  metall::manager manager(metall::create_only, kMetallDataStorePath, 1 << 30);
  auto map = manager.construct<TypeParam>("map-of-container")(
      manager.get_allocator<>());

  (*map)[0];
  map->at(0).insert(std::make_pair(1, 11));
  map->at(0).insert(std::make_pair(2, 22));
  map->at(0).insert(std::make_pair(3, 33));
  map->at(0).insert(std::make_pair(4, 44));

  (*map)[5][6] = 66;
  (*map)[5][7] = 77;
  (*map)[5][8] = 88;

  EXPECT_THAT((*map)[0], WhenSorted(ElementsAre(Pair(1, 11), Pair(2, 22),
                                                Pair(3, 33), Pair(4, 44))));
  EXPECT_THAT(map->at(5),
              WhenSorted(ElementsAre(Pair(6, 66), Pair(7, 77), Pair(8, 88))));

  EXPECT_EQ((*map)[0][1], 11);
  EXPECT_EQ((*map)[0].at(1), 11);
  EXPECT_EQ(map->at(0)[1], 11);
  EXPECT_EQ(map->at(0).at(1), 11);
}

template <typename T>
class MetallOffsetPointerTest : public ::testing::Test {};
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
using MetallTypes =
    ::testing::Types<flat_map_metall, node_map_metall, u_map_metall,
                     b_flat_map_metall, b_node_map_metall>;
#else
using MetallTypes = ::testing::Types<flat_map_metall, node_map_metall>;
#endif
TYPED_TEST_SUITE(MetallOffsetPointerTest, MetallTypes);

TYPED_TEST(MetallOffsetPointerTest, Insert) {
  {
    metall::manager manager(metall::create_only, kMetallDataStorePath, 1 << 30);
    auto map = manager.construct<TypeParam>("map")(manager.get_allocator<>());
    map->insert(std::make_pair(1, 2));
    map->insert(std::make_pair(3, 4));
    map->insert(std::make_pair(5, 6));
    map->insert(std::make_pair(7, 8));
    map->insert(std::make_pair(9, 10));
    EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 2), Pair(3, 4), Pair(5, 6),
                                             Pair(7, 8), Pair(9, 10))));
    map->find(1);
  }
  {
    metall::manager manager(metall::open_read_only, kMetallDataStorePath);
    auto map = manager.find<TypeParam>("map").first;
    EXPECT_THAT(*map, WhenSorted(ElementsAre(Pair(1, 2), Pair(3, 4), Pair(5, 6),
                                             Pair(7, 8), Pair(9, 10))));
    map->find(1);
    EXPECT_EQ(map->find(1)->first, 1);
    EXPECT_EQ(map->find(1)->second, 2);
    EXPECT_EQ((*map)[3], 4);
    EXPECT_EQ(map->at(5), 6);
  }
}
#endif
