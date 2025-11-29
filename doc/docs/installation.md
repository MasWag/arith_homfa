# 2. Installation

## 2.1 System requirements

- 64-bit Linux (tested on recent Ubuntu releases). GCC is required; Clang currently fails due to template instantiation issues.
- CMake ≥ 3.16, make, and a C++17-capable GCC (tested with GCC 10).
- Boost headers/libraries.
- [Microsoft SEAL 4.1.1](https://github.com/microsoft/SEAL) installed system-wide (`libseal.a` symlink may be required).
- [Spot](https://spot.lre.epita.fr/) for LTL parsing and automata construction (tested with 2.10.4+, newer releases should work).
- Optional: bats for shell tests.

For encrypted workloads, expect multi-minute builds and high RAM consumption (≥16 GB) during bootstrapping-heavy runs.

## 2.2 Build from source

```sh
git clone https://github.com/MasWag/arith_homfa.git
cd arith_homfa
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ahomfa_util libahomfa_runner.a
```

## 2.3 Sanity checks

After building, run:

```sh
./build/ahomfa_util --help
```

At least one tutorial (`examples/blood_glucose`) should run end-to-end as described in the Quick Start section. This doubles as a sanity check that the CKKS and TFHE dependencies are configured correctly.
