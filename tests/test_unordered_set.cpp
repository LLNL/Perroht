// Copyright 2023 Lawrence Livermore National Security, LLC and other
// Perroht Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <perroht/unordered_set.hpp>

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
#include <metall/metall.hpp>
#include <metall/container/vector.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#endif

#include <utility>
#include <memory>

#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered/unordered_node_set.hpp>
#include <boost/container_hash/hash.hpp>
#endif

using testing::ElementsAre;
using testing::Not;
using testing::WhenSorted;

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
namespace bip = boost::interprocess;
template <typename T>
using bip_allocator =
    bip::allocator<T, bip::managed_mapped_file::segment_manager>;
#endif

using flat_set =
    perroht::unordered_flat_set<int, std::hash<int>, std::equal_to<int>,
                                std::allocator<int>>;
using node_set =
    perroht::unordered_node_set<int, std::hash<int>, std::equal_to<int>,
                                std::allocator<int>>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using flat_set_metall =
    perroht::unordered_flat_set<int, std::hash<int>, std::equal_to<int>,
                                metall::manager::allocator_type<int>>;
using flat_set_bip =
    perroht::unordered_flat_set<int, std::hash<int>, std::equal_to<int>,
                                bip_allocator<int>>;
using node_set_metall =
    perroht::unordered_node_set<int, std::hash<int>, std::equal_to<int>,
                                metall::manager::allocator_type<int>>;
using node_set_bip =
    perroht::unordered_node_set<int, std::hash<int>, std::equal_to<int>,
                                bip_allocator<int>>;
#endif
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
using u_set = boost::unordered_set<int, std::hash<int>, std::equal_to<int>,
                                   std::allocator<int>>;
using b_flat_set =
    boost::unordered_flat_set<int, std::hash<int>, std::equal_to<int>,
                              std::allocator<int>>;
using b_node_set =
    boost::unordered_node_set<int, std::hash<int>, std::equal_to<int>,
                              std::allocator<int>>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using u_set_metall =
    boost::unordered_set<int, std::hash<int>, std::equal_to<int>,
                         metall::manager::allocator_type<int>>;
using u_set_bip = boost::unordered_set<int, std::hash<int>, std::equal_to<int>,
                                       bip_allocator<int>>;
using b_flat_set_metall =
    boost::unordered_flat_set<int, std::hash<int>, std::equal_to<int>,
                              metall::manager::allocator_type<int>>;
using b_flat_set_bip =
    boost::unordered_flat_set<int, std::hash<int>, std::equal_to<int>,
                              bip_allocator<int>>;
using b_node_set_metall =
    boost::unordered_node_set<int, std::hash<int>, std::equal_to<int>,
                              metall::manager::allocator_type<int>>;
using b_node_set_bip =
    boost::unordered_node_set<int, std::hash<int>, std::equal_to<int>,
                              bip_allocator<int>>;
#endif
#endif

static constexpr const char* kMetallDataStorePath = "./test-unordered-map";
static constexpr const char* kBIPDataStorePath = "./test-unordered-map-bip";

#define CTOR_DTOR_PERSISTENT(TYPE)                                           \
  void create(TYPE##_metall*& m) {                                           \
    manager = new metall::manager(metall::create_only, kMetallDataStorePath, \
                                  1 << 30);                                  \
    m = manager->construct<TYPE##_metall>("unordered_set")(                  \
        manager->get_allocator());                                           \
  }                                                                          \
  void create(TYPE##_bip*& m) {                                              \
    bip::file_mapping::remove(kBIPDataStorePath);                            \
    b_manager = new bip::managed_mapped_file(bip::create_only,               \
                                             kBIPDataStorePath, 1 << 23);    \
    m = b_manager->construct<TYPE##_bip>("unordered_set")(                   \
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
class UnorderedSet : public ::testing::Test {
  void create(flat_set*& m) { m = new flat_set(); }
  void create(node_set*& m) { m = new node_set(); }
  void destroy(flat_set*& m) { delete m; }
  void destroy(node_set*& m) { delete m; }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  CTOR_DTOR_PERSISTENT(flat_set)
  CTOR_DTOR_PERSISTENT(node_set)
#endif

#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
  void create(u_set*& m) { m = new u_set(); }
  void create(b_flat_set*& m) { m = new b_flat_set(); }
  void create(b_node_set*& m) { m = new b_node_set(); }
  void destroy(u_set*& m) { delete m; }
  void destroy(b_flat_set*& m) { delete m; }
  void destroy(b_node_set*& m) { delete m; }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  CTOR_DTOR_PERSISTENT(u_set)
  CTOR_DTOR_PERSISTENT(b_flat_set)
  CTOR_DTOR_PERSISTENT(b_node_set)
#endif
#endif

 protected:
  void SetUp() override { create(set_); }
  void TearDown() override { destroy(set_); }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  metall::manager* manager;
  bip::managed_mapped_file* b_manager;
#endif
  T* set_;
};

using SetTypes =
    ::testing::Types<flat_set, node_set
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
                     ,
                     flat_set_metall, flat_set_bip, node_set_metall,
                     node_set_bip
#endif
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
                     ,
                     u_set, b_flat_set, b_node_set
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
                     ,
                     u_set_metall, u_set_bip, b_flat_set_metall, b_flat_set_bip,
                     b_node_set_metall, b_node_set_bip
#endif
#endif
                     >;

TYPED_TEST_SUITE(UnorderedSet, SetTypes);

//TYPED_TEST(UnorderedSet, DefaultConstructor) { TypeParam* set = this->set_; }

TYPED_TEST(UnorderedSet, CopyConstructor) {
  TypeParam* set = this->set_;
  set->insert(1);
  set->insert(2);
  set->insert(3);
  set->insert(4);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2, 3, 4)));
  TypeParam copy_set(*set);
  EXPECT_THAT(copy_set, WhenSorted(ElementsAre(1, 2, 3, 4)));
}

TYPED_TEST(UnorderedSet, MoveConstructor) {
  TypeParam* set = this->set_;
  set->insert(1);
  set->insert(2);
  set->insert(3);
  set->insert(4);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2, 3, 4)));
  TypeParam move_set(std::move(*set));
  EXPECT_THAT(move_set, WhenSorted(ElementsAre(1, 2, 3, 4)));
}

TYPED_TEST(UnorderedSet, Insert) {
  TypeParam* set = this->set_;
  set->insert(1);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1)));
  auto kv = 2;
  set->insert(kv);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2)));
  EXPECT_EQ(set->size(), 2);
  EXPECT_TRUE(set->contains(1));
  EXPECT_TRUE(set->contains(2));
  // Duplicate element should not be assigned
  set->insert(1);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2)));
  EXPECT_EQ(set->size(), 2);
  EXPECT_TRUE(set->contains(1));
  EXPECT_TRUE(set->contains(2));
}

TYPED_TEST(UnorderedSet, Emplace) {
  TypeParam* set = this->set_;
  set->emplace(1);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1)));
  set->emplace(2);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2)));
  EXPECT_EQ(set->size(), 2);
  EXPECT_TRUE(set->contains(1));
  EXPECT_TRUE(set->contains(2));
  // Duplicate element should not be assigned
  set->emplace(1);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2)));
  EXPECT_EQ(set->size(), 2);
  EXPECT_TRUE(set->contains(1));
  EXPECT_TRUE(set->contains(2));
}

TYPED_TEST(UnorderedSet, Count) {
  TypeParam* set = this->set_;
  set->insert(1);
  set->insert(2);
  set->insert(3);
  EXPECT_EQ(set->count(1), 1);
  EXPECT_EQ(set->count(2), 1);
  EXPECT_EQ(set->count(3), 1);
  EXPECT_EQ(set->count(4), 0);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2, 3)));
  EXPECT_THAT(*set, Not(WhenSorted(ElementsAre(4))));
}

TYPED_TEST(UnorderedSet, Erase) {
  TypeParam* set = this->set_;
  set->insert(1);
  set->insert(2);
  set->insert(3);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2, 3)));
  EXPECT_THAT(*set, Not(WhenSorted(ElementsAre(4))));
  set->erase(1);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(2, 3)));
  EXPECT_THAT(*set, Not(WhenSorted(ElementsAre(1, 4))));
  set->erase(2);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(3)));
  EXPECT_THAT(*set, Not(WhenSorted(ElementsAre(1, 2, 4))));
  set->erase(3);
  EXPECT_THAT(*set, Not(WhenSorted(ElementsAre(1, 2, 3, 4))));
}

TYPED_TEST(UnorderedSet, Clear) {
  TypeParam* set = this->set_;
  set->insert(0);
  set->insert(1);
  set->insert(2);
  set->insert(3);
  EXPECT_FALSE(set->empty());
  set->clear();
  EXPECT_EQ(set->size(), 0);
  EXPECT_GE(set->bucket_count(), 4);
  EXPECT_TRUE(set->empty());

  // Can insert again
  EXPECT_TRUE(set->insert(0).second);
  EXPECT_TRUE(set->insert(1).second);
}

template <typename T>
void TestIteratorHelper(T& it) {
  EXPECT_EQ(*it, 0);

  ++it;
  EXPECT_EQ(*it, 1);

  ++it;
  EXPECT_EQ(*it, 2);

  it++;
  EXPECT_EQ(*it, 3);

  const auto old_it = it++;
  EXPECT_EQ(*old_it, 3);
}

TYPED_TEST(UnorderedSet, Iterator) {
  TypeParam* set = this->set_;
  set->insert(0);
  set->insert(1);
  set->insert(2);
  set->insert(3);
  {
    auto it = set->begin();
    SCOPED_TRACE("begin/end");
    TestIteratorHelper(it);
    EXPECT_EQ(it, set->end());
  }

  {
    auto it = set->cbegin();
    SCOPED_TRACE("cbegin/cend");
    TestIteratorHelper(it);
    EXPECT_EQ(it, set->cend());
  }

  const auto& const_set = set;
  {
    auto it = const_set->begin();
    SCOPED_TRACE("begin/end (const)");
    TestIteratorHelper(it);
    EXPECT_EQ(it, const_set->end());
  }

  {
    auto it = const_set->cbegin();
    SCOPED_TRACE("cbegin/cend (const)");
    TestIteratorHelper(it);
    EXPECT_EQ(it, const_set->cend());
  }
}

TYPED_TEST(UnorderedSet, ReserveAfterEmpty) {
  TypeParam* set = this->set_;
  set->reserve(100);
  EXPECT_GE(set->bucket_count(), 100);
}

TYPED_TEST(UnorderedSet, reserveAfterinsertion) {
  TypeParam* set = this->set_;
  set->insert(0);
  set->insert(1);
  set->reserve(100);
  EXPECT_GE(set->bucket_count(), 100);

  // Make sure existing elements are still there
  EXPECT_THAT(*set, WhenSorted(ElementsAre(0, 1)));
  EXPECT_EQ(set->size(), 2);

  // Make sure we can still insert
  EXPECT_TRUE(set->insert(2).second);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(0, 1, 2)));

  // Reserve behavior should be using rehash() as defined in stl documentation
  const auto old_capacity = set->bucket_count();
  set->reserve(old_capacity);
  EXPECT_EQ(set->bucket_count(), old_capacity);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(0, 1, 2)));

  set->reserve(1);
  EXPECT_EQ(set->size(), 3);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(0, 1, 2)));
}

TYPED_TEST(UnorderedSet, RehashAfterEmpty) {
  TypeParam* set = this->set_;
  set->rehash(100);
  EXPECT_GE(set->bucket_count(), 100);
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

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
namespace bip = boost::interprocess;
template <typename T>
using bip_allocator =
    bip::allocator<T, bip::managed_mapped_file::segment_manager>;
#endif

using flat_set_custom_hash =
    perroht::unordered_flat_set<int, CustomHash, std::equal_to<int>,
                                std::allocator<int>>;
using node_set_custom_hash =
    perroht::unordered_node_set<int, CustomHash, std::equal_to<int>,
                                std::allocator<int>>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using flat_set_custom_hash_metall =
    perroht::unordered_flat_set<int, CustomHash, std::equal_to<int>,
                                metall::manager::allocator_type<int>>;
using flat_set_custom_hash_bip =
    perroht::unordered_flat_set<int, CustomHash, std::equal_to<int>,
                                bip_allocator<int>>;
using node_set_custom_hash_metall =
    perroht::unordered_node_set<int, CustomHash, std::equal_to<int>,
                                metall::manager::allocator_type<int>>;
using node_set_custom_hash_bip =
    perroht::unordered_node_set<int, CustomHash, std::equal_to<int>,
                                bip_allocator<int>>;
#endif
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
using u_set_custom_hash =
    boost::unordered_set<int, CustomHash, std::equal_to<int>,
                         std::allocator<int>>;
using b_flat_set_custom_hash =
    boost::unordered_flat_set<int, CustomHash, std::equal_to<int>,
                              std::allocator<int>>;
using b_node_set_custom_hash =
    boost::unordered_node_set<int, CustomHash, std::equal_to<int>,
                              std::allocator<int>>;
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using u_set_custom_hash_metall =
    boost::unordered_set<int, CustomHash, std::equal_to<int>,
                         metall::manager::allocator_type<int>>;
using u_set_custom_hash_bip =
    boost::unordered_set<int, CustomHash, std::equal_to<int>,
                         bip_allocator<int>>;
using b_flat_set_custom_hash_metall =
    boost::unordered_flat_set<int, CustomHash, std::equal_to<int>,
                              metall::manager::allocator_type<int>>;
using b_flat_set_custom_hash_bip =
    boost::unordered_flat_set<int, CustomHash, std::equal_to<int>,
                              bip_allocator<int>>;
using b_node_set_custom_hash_metall =
    boost::unordered_node_set<int, CustomHash, std::equal_to<int>,
                              metall::manager::allocator_type<int>>;
using b_node_set_custom_hash_bip =
    boost::unordered_node_set<int, CustomHash, std::equal_to<int>,
                              bip_allocator<int>>;
#endif
#endif

template <typename T>
class UnorderedSetCustomHash : public ::testing::Test {
  void create(flat_set_custom_hash*& m) { m = new flat_set_custom_hash(); }
  void create(node_set_custom_hash*& m) { m = new node_set_custom_hash(); }
  void destroy(flat_set_custom_hash*& m) { delete m; }
  void destroy(node_set_custom_hash*& m) { delete m; }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  CTOR_DTOR_PERSISTENT(flat_set_custom_hash)
  CTOR_DTOR_PERSISTENT(node_set_custom_hash)
#endif

#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
  void create(u_set_custom_hash*& m) { m = new u_set_custom_hash(); }
  void create(b_flat_set_custom_hash*& m) { m = new b_flat_set_custom_hash(); }
  void create(b_node_set_custom_hash*& m) { m = new b_node_set_custom_hash(); }
  void destroy(u_set_custom_hash*& m) { delete m; }
  void destroy(b_flat_set_custom_hash*& m) { delete m; }
  void destroy(b_node_set_custom_hash*& m) { delete m; }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  CTOR_DTOR_PERSISTENT(u_set_custom_hash)
  CTOR_DTOR_PERSISTENT(b_flat_set_custom_hash)
  CTOR_DTOR_PERSISTENT(b_node_set_custom_hash)
#endif
#endif

 protected:
  void SetUp() override { create(set_); }
  void TearDown() override { destroy(set_); }
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
  metall::manager* manager;
  bip::managed_mapped_file* b_manager;
#endif
  T* set_;
};

using SetTypesCustomHash =
    ::testing::Types<flat_set_custom_hash, node_set_custom_hash
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
                     ,
                     flat_set_custom_hash_metall, flat_set_custom_hash_bip,
                     node_set_custom_hash_metall, node_set_custom_hash_bip
#endif
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_SET_CUSTOM_HASH_TEST
                     ,
                     u_set_custom_hash, b_flat_set_custom_hash,
                     b_node_set_custom_hash
#ifdef USE_PERSISTENT_ALLOCATOR_TEST
                     ,
                     u_set_custom_hash_metall, u_set_custom_hash_bip,
                     b_flat_set_custom_hash_metall, b_flat_set_custom_hash_bip,
                     b_node_set_custom_hash_metall, b_node_set_custom_hash_bip
#endif
#endif
                     >;
TYPED_TEST_SUITE(UnorderedSetCustomHash, SetTypesCustomHash);

TYPED_TEST(UnorderedSetCustomHash, Insert) {
  TypeParam* set = this->set_;

  set->insert(0);
  set->insert(1);
  set->insert(2);
  set->insert(3);

  EXPECT_EQ(set->size(), 4);
  EXPECT_GE(set->bucket_count(), 4);

  auto ret = set->find(0);
  EXPECT_EQ(*ret, 0);
  ret = set->find(1);
  EXPECT_EQ(*ret, 1);
  ret = set->find(2);
  EXPECT_EQ(*ret, 2);
  ret = set->find(3);
  EXPECT_EQ(*ret, 3);
}

TYPED_TEST(UnorderedSet, RehashAfterinsertion) {
  TypeParam* set = this->set_;
  set->insert(0);
  set->insert(1);
  set->rehash(100);
  EXPECT_GE(set->bucket_count(), 100);

  // Make sure existing elements are still there
  EXPECT_EQ(set->count(0), 1);
  EXPECT_EQ(set->count(1), 1);
  EXPECT_EQ(set->size(), 2);

  // Make sure we can still insert
  EXPECT_TRUE(set->insert(2).second);
  EXPECT_EQ(set->count(2), 1);

  // Make sure rehash() does not do anything if the capacity is already
  // large enough
  const auto old_capacity = set->bucket_count();
  set->rehash(old_capacity);
  EXPECT_EQ(set->bucket_count(), old_capacity);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(0, 1, 2)));

  set->rehash(1);
  EXPECT_GE(set->bucket_count(), 3);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(0, 1, 2)));
}
//*
TYPED_TEST(UnorderedSet, EraseUsingIteratorFind) {
  TypeParam* set = this->set_;
  set->insert(0);
  set->insert(1);
  auto itr = set->erase(set->find(1));
  EXPECT_TRUE(itr == set->find(0) || itr == set->end());
  EXPECT_EQ(set->erase(set->find(0)), set->end());
}

TYPED_TEST(UnorderedSet, EraseUsingIteratorBegin) {
  TypeParam* set = this->set_;
  set->insert(0);
  set->insert(1);
  EXPECT_NE(set->erase(set->cbegin()), set->end());
  EXPECT_EQ(set->erase(set->cbegin()), set->end());
}
//*/
TYPED_TEST(UnorderedSet, LoadFactor) {
  TypeParam* set = this->set_;
  EXPECT_DOUBLE_EQ(set->load_factor(), 0.0);

  set->insert(0);
  EXPECT_GE(set->load_factor(), 0.0);
  EXPECT_LE(set->load_factor(), set->max_load_factor());

  set->insert(1);
  EXPECT_GE(set->load_factor(), 0.0);
  EXPECT_LE(set->load_factor(), set->max_load_factor());
}
//*
TYPED_TEST(UnorderedSet, Swap) {
  TypeParam* set = this->set_;
  set->insert(1);
  set->insert(2);
  set->insert(3);
  TypeParam set1(*set);
  set->clear();
  set->insert(4);
  set->insert(5);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(4, 5)));
  EXPECT_THAT(set1, WhenSorted(ElementsAre(1, 2, 3)));
  set->swap(set1);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 2, 3)));
  EXPECT_THAT(set1, WhenSorted(ElementsAre(4, 5)));
  swap(*set, set1);
  EXPECT_THAT(*set, WhenSorted(ElementsAre(4, 5)));
  EXPECT_THAT(set1, WhenSorted(ElementsAre(1, 2, 3)));
}
//*/
TYPED_TEST(UnorderedSet, Equality) {
  TypeParam* set = this->set_;
  set->insert(1);
  set->insert(2);
  set->insert(3);
  TypeParam set1(*set);
  EXPECT_TRUE(*set == set1);
  EXPECT_TRUE(set1 == *set);
  EXPECT_TRUE(*set == *set);
  EXPECT_TRUE(set1 == set1);
}

TYPED_TEST(UnorderedSet, Inequality) {
  TypeParam* set = this->set_;
  set->insert(1);
  set->insert(2);
  set->insert(3);
  TypeParam set1(*set);
  set1.erase(3);
  EXPECT_TRUE(*set != set1);
  EXPECT_TRUE(set1 != *set);
  EXPECT_TRUE(*set == *set);
  EXPECT_TRUE(set1 == set1);
}

#ifdef USE_PERSISTENT_ALLOCATOR_TEST
using metall::container::scoped_allocator_adaptor;
using metall::container::vector;

template <typename T, typename H>
using flat_set_metall_scoped = perroht::unordered_flat_set<
    T, H, std::equal_to<T>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
template <typename T, typename H>
using node_set_metall_scoped = perroht::unordered_node_set<
    T, H, std::equal_to<T>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
template <typename T, typename H>
using u_set_metall_scoped = boost::unordered_set<
    T, H, std::equal_to<T>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
template <typename T, typename H>
using b_flat_set_metall_scoped = perroht::unordered_flat_set<
    T, H, std::equal_to<T>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
template <typename T, typename H>
using b_node_set_metall_scoped = perroht::unordered_node_set<
    T, H, std::equal_to<T>,
    scoped_allocator_adaptor<metall::manager::allocator_type<T>>>;
#endif

template <typename T>
class MetallNestedInnerSet : public ::testing::Test {};

#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
using InnerMetallSetTypes =
    ::testing::Types<flat_set_metall, node_set_metall, u_set_metall,
                     b_flat_set_metall, b_node_set_metall>;
#else
using InnerMetallSetTypes = ::testing::Types<flat_set_metall, node_set_metall>;
#endif

TYPED_TEST_SUITE(MetallNestedInnerSet, InnerMetallSetTypes);

TYPED_TEST(MetallNestedInnerSet, VectorOfSet) {
  using outer_vector_type = vector<
      TypeParam,
      scoped_allocator_adaptor<metall::manager::allocator_type<TypeParam>>>;

  metall::manager manager(metall::create_only, kMetallDataStorePath, 1 << 30);
  auto vec = manager.construct<outer_vector_type>("vec-of-set")(
      manager.get_allocator<>());

  auto inner_set_0 =
      manager.construct<TypeParam>("inner-set-0")(manager.get_allocator<>());
  inner_set_0->insert(1);
  inner_set_0->insert(2);
  inner_set_0->insert(3);
  inner_set_0->insert(4);
  vec->push_back(*inner_set_0);

  auto inner_set_1 =
      manager.construct<TypeParam>("inner-set-1")(manager.get_allocator<>());
  inner_set_1->insert(6);
  inner_set_1->insert(7);
  inner_set_1->insert(8);
  vec->push_back(*inner_set_1);

  EXPECT_THAT((*vec)[0], WhenSorted(ElementsAre(1, 2, 3, 4)));
  EXPECT_THAT(vec->at(1), WhenSorted(ElementsAre(6, 7, 8)));
}

template <typename T>
class MetallNestedOuterSet : public ::testing::Test {};

using OuterMetallSetTypes = ::testing::Types<
    flat_set_metall_scoped<vector<int>, boost::hash<vector<int>>>,
    node_set_metall_scoped<vector<int>, boost::hash<vector<int>>>
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
    ,
    u_set_metall_scoped<vector<int>, boost::hash<vector<int>>>,
    b_flat_set_metall_scoped<vector<int>, boost::hash<vector<int>>>,
    b_node_set_metall_scoped<vector<int>, boost::hash<vector<int>>>
#endif
    >;

TYPED_TEST_SUITE(MetallNestedOuterSet, OuterMetallSetTypes);

TYPED_TEST(MetallNestedOuterSet, SetOfVector) {
  metall::manager manager(metall::create_only, kMetallDataStorePath, 1 << 30);
  auto set =
      manager.construct<TypeParam>("set-of-vec")(manager.get_allocator<>());

  auto inner_vec_0 =
      manager.construct<vector<int>>("inner-vec-0")(manager.get_allocator<>());
  inner_vec_0->push_back(1);
  inner_vec_0->push_back(2);
  inner_vec_0->push_back(3);
  inner_vec_0->push_back(4);
  set->insert(*inner_vec_0);

  auto inner_vec_5 =
      manager.construct<vector<int>>("inner-vec-5")(manager.get_allocator<>());
  inner_vec_5->push_back(6);
  inner_vec_5->push_back(7);
  inner_vec_5->push_back(8);
  set->insert(*inner_vec_5);

  EXPECT_THAT(*set, WhenSorted(ElementsAre(*inner_vec_0, *inner_vec_5)));
}

template <typename T>
class MetallNestedSet : public ::testing::Test {};

using MetallScopedSetTypes = ::testing::Types<
    flat_set_metall_scoped<flat_set_metall, boost::hash<flat_set_metall>>,
    flat_set_metall_scoped<node_set_metall, boost::hash<node_set_metall>>,
    node_set_metall_scoped<flat_set_metall, boost::hash<flat_set_metall>>,
    node_set_metall_scoped<node_set_metall, boost::hash<node_set_metall>>
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
    ,
    flat_set_metall_scoped<u_set_metall, boost::hash<u_set_metall>>,
    flat_set_metall_scoped<b_flat_set_metall, boost::hash<b_flat_set_metall>>,
    flat_set_metall_scoped<b_node_set_metall, boost::hash<b_node_set_metall>>,
    node_set_metall_scoped<u_set_metall, boost::hash<u_set_metall>>,
    node_set_metall_scoped<b_flat_set_metall, boost::hash<b_flat_set_metall>>,
    node_set_metall_scoped<b_node_set_metall, boost::hash<b_node_set_metall>>,
    u_set_metall_scoped<flat_set_metall, boost::hash<flat_set_metall>>,
    u_set_metall_scoped<node_set_metall, boost::hash<node_set_metall>>,
    u_set_metall_scoped<u_set_metall, boost::hash<u_set_metall>>,
    u_set_metall_scoped<b_flat_set_metall, boost::hash<b_flat_set_metall>>,
    u_set_metall_scoped<b_node_set_metall, boost::hash<b_node_set_metall>>,
    b_flat_set_metall_scoped<flat_set_metall, boost::hash<flat_set_metall>>,
    b_flat_set_metall_scoped<node_set_metall, boost::hash<node_set_metall>>,
    b_flat_set_metall_scoped<u_set_metall, boost::hash<u_set_metall>>,
    b_flat_set_metall_scoped<b_flat_set_metall, boost::hash<b_flat_set_metall>>,
    b_flat_set_metall_scoped<b_node_set_metall, boost::hash<b_node_set_metall>>,
    b_node_set_metall_scoped<flat_set_metall, boost::hash<flat_set_metall>>,
    b_node_set_metall_scoped<node_set_metall, boost::hash<node_set_metall>>,
    b_node_set_metall_scoped<u_set_metall, boost::hash<u_set_metall>>,
    b_node_set_metall_scoped<b_flat_set_metall, boost::hash<b_flat_set_metall>>,
    b_node_set_metall_scoped<b_node_set_metall, boost::hash<b_node_set_metall>>
#endif
    >;

TYPED_TEST_SUITE(MetallNestedSet, MetallScopedSetTypes);

TYPED_TEST(MetallNestedSet, SetOfContainer) {
  metall::manager manager(metall::create_only, kMetallDataStorePath, 1 << 30);
  auto set = manager.construct<TypeParam>("set-of-container")(
      manager.get_allocator<>());

  auto inner_set_0 = manager.construct<typename TypeParam::key_type>(
      "inner-set-0")(manager.get_allocator<>());
  inner_set_0->insert(1);
  inner_set_0->insert(2);
  inner_set_0->insert(3);
  inner_set_0->insert(4);
  set->insert(*inner_set_0);

  auto inner_set_5 = manager.construct<typename TypeParam::key_type>(
      "inner-set-5")(manager.get_allocator<>());
  inner_set_5->insert(6);
  inner_set_5->insert(7);
  inner_set_5->insert(8);
  set->insert(*inner_set_5);

  EXPECT_EQ(set->count(*inner_set_0), 1);
  EXPECT_EQ(set->count(*inner_set_5), 1);
}

template <typename T>
class MetallOffsetPointerTest : public ::testing::Test {};
#ifdef USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST
using MetallTypes =
    ::testing::Types<flat_set_metall, node_set_metall, u_set_metall,
                     b_flat_set_metall, b_node_set_metall>;
#else
using MetallTypes = ::testing::Types<flat_set_metall, node_set_metall>;
#endif
TYPED_TEST_SUITE(MetallOffsetPointerTest, MetallTypes);

TYPED_TEST(MetallOffsetPointerTest, Insert) {
  {
    metall::manager manager(metall::create_only, kMetallDataStorePath, 1 << 30);
    auto set = manager.construct<TypeParam>("set")(manager.get_allocator<>());
    set->insert(1);
    set->insert(3);
    set->insert(5);
    set->insert(7);
    set->insert(9);
    EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 3, 5, 7, 9)));
  }
  {
    metall::manager manager(metall::open_read_only, kMetallDataStorePath);
    auto set = manager.find<TypeParam>("set").first;
    EXPECT_THAT(*set, WhenSorted(ElementsAre(1, 3, 5, 7, 9)));
  }
}
#endif
