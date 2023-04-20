project(ArithHomFA LANGUAGES CXX)

cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated-register")

# C++ version (SEAL requires 17)
set(CMAKE_CXX_STANDARD 17) # C++ standard C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# find_package (OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

add_subdirectory(thirdparty/homfa)
add_subdirectory(thirdparty/TFHEpp)

# Hack to disable -march=native if it is not supported
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-flto -O3 -DRELEASE")
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)

# Import Microsoft SEAL
find_package(SEAL 4.1.1 EXACT REQUIRED)

# Import GSL
# find_package(Microsoft.GSL CONFIG REQUIRED)

# Import Eigen3
find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

find_package(Boost REQUIRED COMPONENTS
  program_options unit_test_framework iostreams graph)

include_directories(
  src/
  /usr/local/include
  /usr/local/include/SEAL-${SEAL_VERSION_MAJOR}.${SEAL_VERSION_MINOR}/
  ${PROJECT_BINARY_DIR}
  #${Boost_INCLUDE_DIRS}
  ${SEAL_INCLUDE_DIRS}
  thirdparties/TFHEpp/include
  thirdparties/TFHEpp/thirdparties/spqlios/
  thirdparties/TFHEpp/thirdparties/cereal/include)

link_directories(
  /usr/local/lib/)

add_executable(ckks_simple_example
  src/ckks_simple_example.cc)

target_link_libraries(ckks_simple_example
  # profiler
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_GRAPH_LIBRARY}
  seal
  hexl)

add_executable(bfv_simple_example
  src/bfv_simple_example.cc)

target_link_libraries(bfv_simple_example
  # profiler
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_GRAPH_LIBRARY}
  seal
  hexl)

add_executable(ckks_simple_example_symmetric
        src/ckks_simple_example_symmetric.cc)

target_link_libraries(ckks_simple_example_symmetric
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(ckks_encrypt_cipher_text_symmetric
        src/ckks_encrypt_cipher_text_symmetric.cc)

target_link_libraries(ckks_encrypt_cipher_text_symmetric
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(ckks_save_secret_key
        src/ckks_save_secret_key.cc)

target_link_libraries(ckks_save_secret_key
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(ckks_decrypt_cipher_text
        src/ckks_decrypt_cipher_text.cc)

target_link_libraries(ckks_decrypt_cipher_text
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(ckks_encrypt_cipher_text_sequence_symmetric
        src/ckks_encrypt_cipher_text_sequence_symmetric.cc)

target_link_libraries(ckks_encrypt_cipher_text_sequence_symmetric
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(ckks_decrypt_cipher_text_sequence
        src/ckks_decrypt_cipher_text_sequence.cc)

target_link_libraries(ckks_decrypt_cipher_text_sequence
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(tfhe_simple_example
        src/tfhe_simple_example.cc)

target_link_libraries(tfhe_simple_example
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl
        tfhe++)

add_executable(tfhe_lvl3_simple_example
        src/tfhe_lvl3_simple_example.cc
        src/tfhe_lvl3_inst.cc)

target_link_libraries(tfhe_lvl3_simple_example
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl
        tfhe++)

add_executable(ckks_to_tfhe_lvl3
        src/ckks_to_tfhe_lvl3.cc
        src/tfhe_lvl3_inst.cc)

target_link_libraries(ckks_to_tfhe_lvl3
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl
        pegasus
        tfhe++)

add_executable(ckks_to_tfhe_lvl3_no_embed
        src/ckks_to_tfhe_lvl3_no_embed.cc
        src/tfhe_lvl3_inst.cc)

target_link_libraries(ckks_to_tfhe_lvl3_no_embed
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl
        pegasus
        tfhe++)

add_executable(ckks_simple_example_with_ntt
        src/ckks_simple_example_with_ntt.cc)

target_link_libraries(ckks_simple_example_with_ntt
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(ckks_check_key_equivalence
        src/ckks_check_key_equivalence.cc)

target_link_libraries(ckks_check_key_equivalence
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(uintarith_trial
        src/uintarith_trial.cc)

target_link_libraries(uintarith_trial
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)

add_executable(ckks_no_embed
        src/ckks_no_embed.cc)

target_link_libraries(ckks_no_embed
        # profiler
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_GRAPH_LIBRARY}
        seal
        hexl)