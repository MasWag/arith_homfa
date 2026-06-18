cmake_policy(SET CMP0048 NEW)
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 OLD)
endif()
project(BloodGlucose LANGUAGES CXX VERSION 0.0.1)

cmake_minimum_required(VERSION 3.16)

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
