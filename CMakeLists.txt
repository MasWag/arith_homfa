cmake_policy(SET CMP0048 NEW)
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif()
project(ArithHomFA LANGUAGES CXX VERSION 0.0.1)

cmake_minimum_required(VERSION 3.16)

## Configure PATH
set(HOMFA_DIR "${CMAKE_SOURCE_DIR}/thirdparty/homfa")
set(CMAKE_MODULE_PATH
        ${CMAKE_SOURCE_DIR}
        "${HOMFA_DIR}/thirdparty/sanitizers-cmake/cmake"
        ${CMAKE_MODULE_PATH}
        )
set(Backward_DIR "${HOMFA_DIR}/thirdparty/backward-cpp")
find_package(Backward)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# C++ version (SEAL requires 17)
set(CMAKE_CXX_STANDARD 20) # C++ standard C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(COMPILE_DEFINITIONS )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# find_package (OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -fopenmp")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS} -fopenmp")

# Get git revision
# Thanks to: https://stackoverflow.com/a/6526533
set(GIT_REVISION "unknown")
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  find_package(Git)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE "HOMFA_GIT_REVISION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "Git revision: ${HOMFA_GIT_REVISION}")
  endif(GIT_FOUND)
endif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
list(APPEND COMPILE_DEFINITIONS GIT_REVISION="${GIT_REVISION}")

# add_subdirectory(thirdparty/homfa)
add_subdirectory(${HOMFA_DIR}/thirdparty/TFHEpp)

# Hack to disable -march=native if it is not supported
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if (COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif ()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DRELEASE")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# Import Microsoft SEAL
find_package(SEAL 4.1.1 EXACT REQUIRED)

# Import GSL
# find_package(Microsoft.GSL CONFIG REQUIRED)

# Import Eigen3
find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

# Import spot
find_package(PkgConfig)
pkg_check_modules(SPOT REQUIRED libspot)
pkg_check_modules(BDDX REQUIRED libbddx)

# Import Boost
find_package(Boost REQUIRED COMPONENTS
        program_options unit_test_framework iostreams graph)

include_directories(
        src/
        /usr/local/include
        /usr/local/include/SEAL-${SEAL_VERSION_MAJOR}.${SEAL_VERSION_MINOR}/
        ${PROJECT_BINARY_DIR}
        #${Boost_INCLUDE_DIRS}
        ${SEAL_INCLUDE_DIRS}
        ${HOMFA_DIR}/src
        ${HOMFA_DIR}/thirdparty/backward-cpp
        ${HOMFA_DIR}/thirdparty/CLI11/include
        ${HOMFA_DIR}/thirdparty/ThreadPool
        ${HOMFA_DIR}/thirdparty/spdlog/include
        ${HOMFA_DIR}/thirdparty/TFHEpp/include
        ${HOMFA_DIR}/thirdparty/TFHEpp/thirdparties/spqlios
        ${HOMFA_DIR}/thirdparty/TFHEpp/thirdparties/randen
        ${HOMFA_DIR}/thirdparty/TFHEpp/thirdparties/cereal/include)

link_directories(
        /usr/local/lib/)

link_libraries(
        seal
        hexl
        )

add_executable(ahomfa_util
        src/main.cc
        src/tfhe_lvl3_inst.cc
        thirdparty/homfa/src/tfhepp_util.cpp
        thirdparty/homfa/src/graph.cpp
        )

target_link_libraries(ahomfa_util
        seal
        hexl
        randen
        tfhe++
        spot
        bddx
        pthread
        )
target_compile_definitions(ahomfa_util PRIVATE ${COMPILE_DEFINITIONS})

add_library(ahomfa_runner
        src/ahomfa_runner.cc
        thirdparty/homfa/src/graph.cpp
  )

target_link_libraries(ahomfa_runner
        seal
        hexl
        randen
        tfhe++
        bddx
        pthread
        )
target_compile_definitions(ahomfa_runner PRIVATE ${COMPILE_DEFINITIONS})

## Config for Test
enable_testing()
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rapidcheck/")

add_executable(unit_test EXCLUDE_FROM_ALL
        test/unit_test.cc
        src/tfhe_lvl3_inst.cc
        test/ckks_to_tfhe_test.cc
        test/ckks_no_embed_test.cc
        test/seal_config_test.cc
        test/blood_glucose_one.cc
        test/ckks_predicate_test.cc
        test/tfhepp_keygen_test.cc
        test/plain_runner_test.cc
        thirdparty/homfa/src/graph.cpp
        )

target_include_directories(unit_test PUBLIC
        thirdparty/rapidcheck/include
        thirdparty/rapidcheck/extras/boost_test/include
        ${SPOT_INCLUDE_DIRS}
        ${BDDX_INCLUDE_DIRS}
        )

target_link_libraries(unit_test
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        SEAL::seal
        hexl
        randen
        rapidcheck
        pthread
        tfhe++
        ${SPOT_LIBRARIES}
        ${BDDX_LIBRARIES}
        )