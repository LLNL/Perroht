# Download Google Test or use the installed package
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

function(add_gtest_executable test_name test_file)
    add_basic_test(${test_name} ${test_file})
    target_link_libraries(${test_name} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
    gtest_discover_tests(${test_name})
endfunction()

add_gtest_executable(test_key_value_traits test_key_value_traits.cpp)
add_gtest_executable(test_data_holder test_data_holder.cpp)
add_gtest_executable(test_header test_header.cpp)
add_gtest_executable(test_perroht test_perroht.cpp)
add_gtest_executable(test_unordered_map test_unordered_map.cpp)
add_gtest_executable(test_unordered_set test_unordered_set.cpp)

add_basic_test(random_insert_and_erase random_insert_and_erase.cpp)

if (BUILD_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST)
    target_include_directories(test_unordered_map
            PRIVATE ${Boost_INCLUDE_DIRS})
    target_include_directories(test_unordered_set
            PRIVATE ${Boost_INCLUDE_DIRS})
endif ()

if (BUILD_PERSISTENT_ALLOCATOR_TEST)
    setup_metall_target(test_data_holder)
    setup_metall_target(test_perroht)
    setup_metall_target(test_unordered_map)
    setup_metall_target(test_unordered_set)
    setup_metall_target(random_insert_and_erase)
endif ()
