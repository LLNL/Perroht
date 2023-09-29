cmake_minimum_required(VERSION 3.20)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchContent)

# -------------------------------------------------------------------------------- #
# CMake policy
# -------------------------------------------------------------------------------- #
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
    cmake_policy(SET CMP0077 NEW)
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    cmake_policy(SET CMP0135 NEW)
endif()


# -------------------------------------------------------------------------------- #
# Perroht general configuration
# -------------------------------------------------------------------------------- #
project(Perroht VERSION 0.1 LANGUAGES CXX
        DESCRIPTION "Persistent Robin Hood Hash Table")

configure_file(PerrohtConfig.h.in PerrohtConfig.h)

# ----- Setting up a INTERFACE library to install header files ----- #
include(GNUInstallDirs)
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
        INTERFACE $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT ${PROJECT_NAME}_Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/perroht DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# -------------------------------------------------------------------------------- #
# Generate and install the package configuration and package version files
# -------------------------------------------------------------------------------- #
include(CMakePackageConfigHelpers)

# generate the version file for the config file
write_basic_package_version_file(
        "${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY ExactVersion)

# create config file
configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

# install config files
install(FILES
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)
# -------------------------------------------------------------------------------- #


#
#  Options
#
option(JUST_INSTALL_PERROHT_HEADER "Just install Perroht headers, not build" OFF)
option(SEPARATE_HEADER "Separate header from data" OFF)
option(BUILD_TEST "Build the test" OFF)
option(BUILD_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST "Building Boost Unordered Closed/Flat/Node Map Test" OFF)
option(BUILD_PERSISTENT_ALLOCATOR_TEST "Building Metall and Boost Interprocess Allocator Test" OFF)
option(BUILD_BENCH "Build the benchmark" OFF)

if ((NOT ${BUILD_TEST}) AND (NOT {$BUILD_BENCH}))
    return()
endif ()

if (JUST_INSTALL_PERROHT_HEADER)
    return()
endif()


#
#  Common configurations for all executables
#
function(setup_common_config name)
    # Common
    target_compile_options(${name} PRIVATE -Wall)

    # Debug
    target_compile_options(${name} PRIVATE $<$<CONFIG:Debug>:-Og>)
    target_compile_options(${name} PRIVATE $<$<CONFIG:Debug>:-g3>)
    target_compile_options(${name} PRIVATE $<$<CONFIG:Debug>:-Wextra>)
    if (Linux)
        target_compile_options(${name} PRIVATE $<$<CONFIG:Debug>:-pg>)
    endif ()

    # Release
    target_compile_options(${name} PRIVATE $<$<CONFIG:Release>:-Ofast>)
    target_compile_options(${name} PRIVATE $<$<CONFIG:Release>:-DNDEBUG>)

    # Release with debug info
    target_compile_options(${name} PRIVATE $<$<CONFIG:RelWithDebInfo>:-Ofast>)
    target_compile_options(${name} PRIVATE $<$<CONFIG:RelWithDebInfo>:-g3>)
    if (Linux)
        target_compile_options(${name} PRIVATE $<$<CONFIG:RelWithDebInfo>:-pg>)
    endif ()

    if (SEPARATE_HEADER)
        target_compile_definitions(${name} PRIVATE PERROHT_SEPARATE_HEADER)
    endif ()
endfunction()

#
# Function for setting up a target that uses Metall
#
function(setup_metall_target exe_name)
    target_include_directories(${exe_name} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${exe_name} PRIVATE Threads::Threads)
    # TODO: link stdc++fs for old compilers
    # target_link_libraries(${exe_name} PRIVATE stdc++fs)
    target_link_libraries(${exe_name} PRIVATE Metall::Metall)
endfunction()

#
# Function for basic executable
#
function(add_basic_executable target source)
    add_executable(${target} ${source})
    target_include_directories(${target}
            PRIVATE "${PROJECT_SOURCE_DIR}/include")
    setup_common_config(${target})
endfunction()

#
# Function for basic test executable
#
function(add_basic_test target source)
    add_basic_executable(${target} ${source})

    if (BUILD_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST)
        list(APPEND TEST_EXE_DEFS "USE_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST")
    endif ()

    if (BUILD_PERSISTENT_ALLOCATOR_TEST)
        list(APPEND TEST_EXE_DEFS "USE_PERSISTENT_ALLOCATOR_TEST")
    endif ()

    foreach(X IN LISTS TEST_EXE_DEFS)
        target_compile_definitions(${target} PRIVATE ${X})
    endforeach()
endfunction()

if (BUILD_TEST OR BUILD_BENCH)
    if (BUILD_BOOST_CLOSED_AND_OPEN_ADDRESS_MAP_TEST OR BUILD_PERSISTENT_ALLOCATOR_TEST OR BUILD_BENCH)
        # ---------- Boost ---------- #
        # Disable the boost-cmake feature (BoostConfig.cmake or boost-config.cmake) since
        # there is a tricky behavior/issue especially in Boost 1.70.0.
        set(Boost_NO_BOOST_CMAKE ON)

        find_package(Boost 1.82)
        if (NOT Boost_FOUND)
            message(STATUS "Download and setup Boost")
            FetchContent_Declare(Boost
                    URL https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.bz2)
            FetchContent_GetProperties(Boost)
            if (NOT Boost_POPULATED)
                FetchContent_Populate(Boost)
            endif ()
            set(BOOST_ROOT ${boost_SOURCE_DIR})
            find_package(Boost 1.82)
        endif ()
    endif ()

    if (BUILD_PERSISTENT_ALLOCATOR_TEST OR BUILD_BENCH)
        #
        #  Threads
        #
        find_package(Threads REQUIRED)

        #
        #  Metall
        #
        find_package(Metall QUIET)
        if (NOT Metall_FOUND)
            set(JUST_INSTALL_METALL_HEADER TRUE)
            FetchContent_Declare(Metall
                    GIT_REPOSITORY https://github.com/LLNL/metall.git
                    GIT_TAG develop
            )
            FetchContent_MakeAvailable(Metall)
        endif ()
    endif ()

    if (BUILD_TEST)
        # Add tests
        enable_testing()
        add_subdirectory(tests)
    endif ()

    if (BUILD_BENCH)
        # Add benchmark
        add_subdirectory(bench)
    endif ()
endif ()