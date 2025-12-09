set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0167 OLD)
set(CMAKE_POLICY_DEFAULT_CMP0175 OLD)

cmake_minimum_required(VERSION 3.16...3.28)

# CMake >= 4.2 removes compatibility for projects targeting < 3.5.
# Some bundled third-parties (cpu-features via HEXL) still declare VERSION 3.0,
# so force the minimum policy version high enough to keep them building.
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif()

project(ArithHomFABenchmarks LANGUAGES C CXX VERSION 0.0.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(HOMFA_OPENMP_FLAG "-fopenmp")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(HOMFA_OPENMP_FLAG "-fopenmp=libgomp")
endif()

# C++ version
set(CMAKE_CXX_STANDARD 20) # C++ standard C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# find_package (OpenMP REQUIRED)
if(DEFINED OpenMP_C_FLAGS AND NOT OpenMP_C_FLAGS MATCHES "NOTFOUND")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
endif()
if(DEFINED OpenMP_CXX_FLAGS AND NOT OpenMP_CXX_FLAGS MATCHES "NOTFOUND")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()
if(DEFINED OpenMP_EXE_LINKER_FLAGS AND NOT OpenMP_EXE_LINKER_FLAGS MATCHES "NOTFOUND")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${HOMFA_OPENMP_FLAG}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HOMFA_OPENMP_FLAG}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${HOMFA_OPENMP_FLAG}")

add_subdirectory(../ arith_homfa)
add_subdirectory(blood_glucose)
add_subdirectory(vehicle_rss)

# TFHEpp's bundled HEXL writes generated headers into its source tree, which
# lives outside the examples sandbox. Copy it into the build tree so the files
# remain writable before adding the subdirectory.
set(TFHEPP_HEXL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/TFHEpp/thirdparties/hexl")
set(TFHEPP_HEXL_STAGING_DIR "${CMAKE_BINARY_DIR}/tfhepp_hexl-src")
execute_process(
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${TFHEPP_HEXL_STAGING_DIR}"
        COMMAND_ERROR_IS_FATAL ANY)
execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${TFHEPP_HEXL_SOURCE_DIR}" "${TFHEPP_HEXL_STAGING_DIR}"
        COMMAND_ERROR_IS_FATAL ANY)
add_subdirectory(${TFHEPP_HEXL_STAGING_DIR} hexl)

# Hack to disable -march=native if it is not supported
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if (COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif ()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DRELEASE -DNDEBUG")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
