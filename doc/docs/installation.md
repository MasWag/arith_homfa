# 2. Installation

## 2.1 System requirements

- 64-bit Linux (tested on recent Ubuntu releases). GCC is required; Clang currently fails due to template instantiation issues.
- CMake ≥ 3.16, make, and a C++17-capable GCC (tested with GCC 10).
- Boost headers/libraries.
- [Microsoft SEAL 4.1.1](https://github.com/microsoft/SEAL) installed system-wide (`libseal.a` symlink may be required).
- [Spot](https://spot.lre.epita.fr/) for LTL parsing and automata construction (tested with 2.10.4+, newer releases should work).
- Optional: TFHEpp submodule (already vendored), spdlog for logging, and bats for shell tests.

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
ctest --output-on-failure
```

At least one tutorial (`examples/blood_glucose`) should run end-to-end as described in the Quick Start section. This doubles as a sanity check that the CKKS and TFHE dependencies are configured correctly.

## 2.4 Common install problems

- **Clang link failures.** Use GCC 10+ until the explicit template instantiation issue is resolved.
- **Missing `libseal.a`.** Create a symlink (`sudo ln -s /usr/local/lib/libseal-4.1.a /usr/local/lib/libseal.a`).
- **Spot headers not found.** Ensure `PKG_CONFIG_PATH` contains Spot’s `pkgconfig` directory before invoking CMake.
- **Slow bootstrapping.** Confirm CPU supports AVX2; otherwise expect significantly longer runtimes.
