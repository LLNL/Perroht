# Test Case Options
By default, only perroht flat/node maps/sets with std::allocator are tested.
* To enable Boost unordered map and flat/node map tests, enable the option: 
```
cmake -DBUILD_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST=ON ..
```
* To enable hash map/set tests using metall and boost interprocess allocators, enable the option:
```
cmake -DBUILD_PERSISTENT_ALLOCATOR_TEST=ON ..
```
* Both options can be enable to run all possible tests.
```
cmake -DBUILD_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST=ON DBUILD_PERSISTENT_ALLOCATOR_TEST=ON ..
```