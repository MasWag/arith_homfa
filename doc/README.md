# ArithHomFA Documentation Guide

This directory hosts everything needed to build and publish the user and developer manuals for Arithmetic HomFA (ArithHomFA). The documentation tree is rooted at `/doc`, so any path below assumes you start from the repository root. GitHub Actions automatically serves the latest site at [https://maswag.github.io/arith_homfa](https://maswag.github.io/arith_homfa) and the Doxygen manual at [https://maswag.github.io/arith_homfa/developer_manual](https://maswag.github.io/arith_homfa/developer_manual).

## Project overview

ArithHomFA is a prototype toolkit for *oblivious* online STL (signal temporal logic) monitoring with discrete-time semantics. It combines two FHE schemes—CKKS for arithmetic and TFHE for logic—to evaluate STL specifications on encrypted traces without revealing the data.

The toolkit offers:

- `ahomfa_util`: a command-line utility for key generation, encryption/decryption, and STL-to-DFA conversion.
- `libahomfa_runner.a`: a library you link with your predicate implementation to build an oblivious monitor.

## Requirements

The core project expects the following toolchain (tested versions in parentheses):

- GCC with LTO support (10); Clang is currently unsupported due to template instantiation issues.
- CMake
- Boost
- [Microsoft SEAL](https://github.com/microsoft/SEAL) (4.1.1) for CKKS operations.
- [Spot](https://spot.lre.epita.fr/) (2.10.4) for LTL/DFA transformations.

See [Spot’s install guide](https://spot.lre.epita.fr/install.html) for both SEAL and Spot instructions. After installing SEAL, you may need `sudo ln -s /usr/local/lib/libseal-4.1.a /usr/local/lib/libseal.a`.

## Building `ahomfa_util` and the runner library

```sh
git submodule update --init --recursive
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -- ahomfa_util libahomfa_runner.a
```

## Typical workflow

1. **Server preparation**
   - Choose CKKS parameters (JSON file, e.g., `examples/config.json`).
   - Define the monitored STL/LTL formula and corresponding predicates.
   - Build the monitor by linking your predicate implementation against `libahomfa_runner.a`.
   - Convert the STL/LTL formula to a DFA specification with `ahomfa_util ltl2spec`, optionally reverse it via `spec2spec --reverse`.
2. **Client preparation**
   - Generate CKKS/TFHE secret keys plus CKKS relinearization and TFHE bootstrapping keys via `ahomfa_util`.
   - Encrypt the multi-dimensional signal with `ahomfa_util ckks enc`.
3. **Monitoring loop**
   - Server runs the monitor with the encrypted data, DFA spec, relinearization key, and bootstrapping key to produce TFHE ciphertexts.
   - Client decrypts the TFHE stream using `ahomfa_util tfhe dec`.

The `examples/blood_glucose` directory provides a full walkthrough (formulas, predicates, Makefile targets, and CLI invocations) that mirrors the steps above.

### Predicate implementation notes

Predicates inherit from `ArithHomFA::CKKSPredicate` (`src/ckks_predicate.hh`) and must:

- Use Microsoft SEAL correctly (scale management, rescale/mod-switch to reach the last level).
- Avoid writing to `std::cout`; rely on `spdlog` instead.
- Specify an approximate bound on the difference between the signal and its threshold.

See `examples/blood_glucose/blood_glucose_one.cc` for a concrete predicate and Microsoft’s [CKKS example](https://github.com/microsoft/SEAL/blob/main/native/examples/5_ckks_basics.cpp) for SEAL usage.

## MkDocs site workflow

### Prerequisites

- Python 3.9+ available as `/usr/bin/python3`
- `mkdocs` installed inside a virtual environment (recommended)

### Build steps

```sh
# 1. Create and activate the virtual environment (first time only)
/usr/bin/python3 -m venv .venv
source .venv/bin/activate

# 2. Install MkDocs inside the venv
pip install mkdocs

# 3. Build the site from /doc
cd doc
mkdocs build
```

The generated static site lives in `doc/site/` (git-ignored). To preview locally with live reload:

```sh
cd doc
mkdocs serve
```

This serves `http://127.0.0.1:8000` until you press `Ctrl+C`.

## Developer manual (Doxygen)

API-focused developer documentation is generated via Doxygen and integrated with CMake:

```sh
cmake -S . -B build
cmake --build build --target developer_manual
```

HTML output is placed at `build/doc/developer_manual/html/index.html`. The target is available only when `doxygen` is on your `PATH`, and the published version is refreshed by GitHub Actions alongside the MkDocs site.
