set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0167 OLD)
set(CMAKE_POLICY_DEFAULT_CMP0175 OLD)

cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0048 NEW)
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif()
if(POLICY CMP0175)
  cmake_policy(SET CMP0175 OLD)
endif()
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()
project(ArithHomFA LANGUAGES CXX VERSION 0.0.1)

# Allow bundled third-party projects that still declare very old
# cmake_minimum_required values to configure under CMake >= 4.0.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

## Configure PATH
set(CMAKE_MODULE_PATH
        ${CMAKE_SOURCE_DIR} "thirdparty/sanitizers-cmake/cmake"
        ${CMAKE_MODULE_PATH})
set(Backward_DIR "thirdparty/backward-cpp")
find_package(Backward)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

# C++ version (SEAL requires 17)
set(CMAKE_CXX_STANDARD 20) # C++ standard C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# find_package (OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -fopenmp")
set(CMAKE_EXE_LINKER_FLAGS
  "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS} -fopenmp")

# Get git revision
# Thanks to: https://stackoverflow.com/a/6526533
set(GIT_REVISION "unknown")
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  find_package(Git)
  if(Git_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "GIT_REVISION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git revision: ${GIT_REVISION}")
  endif()
endif()
list(APPEND COMPILE_DEFINITIONS GIT_REVISION="${GIT_REVISION}")

add_subdirectory(thirdparty/TFHEpp)

# Hack to disable -march=native if it is not supported
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DRELEASE")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# Import Microsoft SEAL
find_package(SEAL 4.1.1)
if(NOT SEAL_FOUND)
        add_subdirectory(thirdparty/SEAL)
        # Import Intel HEXL
        add_subdirectory(thirdparty/TFHEpp/thirdparties/hexl)
else()
        if(SEAL_STATIC_FOUND)
        set(SEAL_LIB SEAL::seal)
        elseif(SEAL_SHARED_FOUND)
        set(SEAL_LIB SEAL::seal_shared)
        endif()
endif()

# Import TBB
find_package(TBB REQUIRED)

# Import GSL
# find_package(Microsoft.GSL CONFIG REQUIRED)

# Import spot
find_package(PkgConfig)
pkg_check_modules(SPOT REQUIRED libspot)
pkg_check_modules(BDDX REQUIRED libbddx)

# Import Boost
find_package(Boost REQUIRED COMPONENTS
        unit_test_framework iostreams graph)

include_directories(
        src/
        /usr/local/include
        /usr/local/include/SEAL-${SEAL_VERSION_MAJOR}.${SEAL_VERSION_MINOR}/
        ${PROJECT_BINARY_DIR}
        #${Boost_INCLUDE_DIRS}
        ${SEAL_INCLUDE_DIRS}
        thirdparty/backward-cpp
        thirdparty/CLI11/include
        thirdparty/ThreadPool
        thirdparty/spdlog/include
        thirdparty/TFHEpp/include
        thirdparty/TFHEpp/thirdparties/spqlios
        thirdparty/TFHEpp/thirdparties/randen
        thirdparty/TFHEpp/thirdparties/cereal/include)

link_directories(
        /usr/local/lib/)

link_libraries(
        ${SEAL_LIB}
        TBB::tbb
        )


add_executable(ahomfa_util
        src/main.cc
        src/tfhepp_util.cpp
        src/graph.cpp
        )

target_link_libraries(ahomfa_util
        ${SEAL_LIB}
        TBB::tbb
        randen
        pthread
        tfhe++
        ${SPOT_LIBRARIES}
        ${BDDX_LIBRARIES}
        )
target_compile_definitions(ahomfa_util PRIVATE ${COMPILE_DEFINITIONS})

add_library(ahomfa_runner
        src/ahomfa_runner.cc
        src/graph.cpp
        src/offline_dfa.cpp
        src/online_dfa.cpp
        src/backstream_dfa_runner.cpp
        src/timeit.cpp
        src/tfhepp_util.cpp
        )

target_compile_definitions(ahomfa_runner PRIVATE ${COMPILE_DEFINITIONS})

## Config for Test
enable_testing()
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rapidcheck/")

add_executable(unit_test EXCLUDE_FROM_ALL
        test/unit_test.cc
        test/ckks_to_tfhe_test.cc
        test/ckks_no_embed_test.cc
        test/seal_config_test.cc
        test/blood_glucose_one.cc
        test/ckks_predicate_test.cc
        test/tfhepp_keygen_test.cc
        test/plain_runner_test.cc
        test/tfhe_lvl3_to_lvl1_test.cc
        src/graph.cpp
        test/offline_runner_test.cc
        src/offline_dfa.cpp
        src/online_dfa.cpp
        src/backstream_dfa_runner.cpp
        src/timeit.cpp
        src/tfhepp_util.cpp
        test/reverse_runner_test.cc
        test/block_runner_test.cc
        test/tlwe_reader_writer_test.cc
        test/pointwise_runner_test.cc
        test/ckks_reader_writer_test.cc
        test/secret_key_test.cc
        test/bootstrapping_key_test.cc
        )

target_include_directories(unit_test PUBLIC
        thirdparty/rapidcheck/include
        thirdparty/rapidcheck/extras/boost_test/include
        ${SPOT_INCLUDE_DIRS}
        ${BDDX_INCLUDE_DIRS}
        )

target_link_libraries(unit_test
        # profiler
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${SEAL_LIB}
        TBB::tbb
        randen
        rapidcheck
        pthread
        tfhe++
        ${SPOT_LIBRARIES}
        ${BDDX_LIBRARIES}
        )
target_compile_definitions(unit_test PRIVATE ${COMPILE_DEFINITIONS})

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc)
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

  add_custom_target(developer_manual
          COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          COMMENT "Generating the ArithHomFA developer manual with Doxygen"
          VERBATIM)
else()
  message(STATUS "Doxygen not found. Target 'developer_manual' will not be available.")
endif()
