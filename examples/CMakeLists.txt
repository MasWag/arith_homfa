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

project(ArithHomFABenchmarks LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# C++ version
set(CMAKE_CXX_STANDARD 20) # C++ standard C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# find_package (OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -fopenmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -fopenmp")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS} -fopenmp")

add_subdirectory(../ arith_homfa)
add_subdirectory(blood_glucose)
add_subdirectory(vehicle_rss)
add_subdirectory(../thirdparty/TFHEpp/thirdparties/hexl/ hexl)

# Hack to disable -march=native if it is not supported
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if (COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif ()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DRELEASE -DNDEBUG")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
