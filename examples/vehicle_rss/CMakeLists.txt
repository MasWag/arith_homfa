cmake_policy(SET CMP0048 NEW)
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 OLD)
endif()
project(VehicleRSS LANGUAGES CXX VERSION 0.0.1)

cmake_minimum_required(VERSION 3.16)

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
find_package(SEAL REQUIRED)
if(SEAL_VERSION VERSION_LESS 4.1.0)
    message(FATAL_ERROR "SEAL < 4.1.0 is not supported")
elseif(SEAL_VERSION VERSION_GREATER_EQUAL 5.0.0)
    message(FATAL_ERROR "SEAL >= 5.0.0 is not supported")
endif()
if(SEAL_STATIC_FOUND)
    set(SEAL_LIB SEAL::seal)
elseif(SEAL_SHARED_FOUND)
    set(SEAL_LIB SEAL::seal_shared)
endif()

# Import spot
find_package(PkgConfig)
pkg_check_modules(SPOT libspot REQUIRED)
if(SPOT_VERSION VERSION_LESS 2.4.3)
    message(FATAL_ERROR "Spot < 2.4.3 is not supported")
endif()
pkg_check_modules(BDDX REQUIRED libbddx)

link_libraries(
        ${SEAL_LIB}
        hexl
        randen
        tbb
        pthread
        ahomfa_runner
        tfhe++
        ${SPOT_LIBRARIES}
        ${BDDX_LIBRARIES})

include_directories(
        ../../src
        ../../thirdparty/TFHEpp/include
        ../../thirdparty/TFHEpp/thirdparties/cereal/include)

add_executable(vehicle_rss
        vrss_predicate.cc)
