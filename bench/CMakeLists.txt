add_basic_executable(bench_insert bench_insert.cpp)
setup_metall_target(bench_insert)

add_basic_executable(bench_find bench_find.cpp)
setup_metall_target(bench_find)

add_basic_executable(bench_erase bench_erase.cpp)
setup_metall_target(bench_erase)

add_basic_executable(gen_insert_dataset gen_insert_dataset.cpp)
add_basic_executable(gen_find_dataset gen_find_dataset.cpp)
add_basic_executable(gen_erase_dataset gen_erase_dataset.cpp)

# Copy file
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/run_bench.sh
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)