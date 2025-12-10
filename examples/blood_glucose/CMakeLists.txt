cmake_policy(SET CMP0048 NEW)
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 OLD)
endif()
project(BloodGlucose LANGUAGES CXX VERSION 0.0.1)

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
find_package(SEAL 4.1.1 REQUIRED)

link_libraries(
        SEAL::seal
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

add_executable(blood_glucose_one
  blood_glucose_one.cc)

add_executable(blood_glucose_two
  blood_glucose_two.cc)

add_executable(blood_glucose_four
  blood_glucose_four.cc)

add_executable(blood_glucose_five
  blood_glucose_five.cc)

add_executable(blood_glucose_six
  blood_glucose_six.cc)

add_executable(blood_glucose_seven
  blood_glucose_seven.cc)

add_executable(blood_glucose_eight
  blood_glucose_eight.cc)

add_executable(blood_glucose_ten
  blood_glucose_ten.cc)

add_executable(blood_glucose_eleven
  blood_glucose_eleven.cc)
