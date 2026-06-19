Arithmetic HomFA (ArithHomFA)
=============================

[![Users Manual](https://img.shields.io/badge/docs-Users-yellow)](https://maswag.github.io/arith_homfa/)
[![Developers Manual](https://img.shields.io/badge/docs-Dev-green)](https://maswag.github.io/arith_homfa/developer_manual)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](./LICENSE)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.20754335.svg)](https://doi.org/10.5281/zenodo.20754335)
[![Docker Pulls](https://img.shields.io/docker/pulls/maswag/arith_homfa)](https://hub.docker.com/r/maswag/arith_homfa)


Arithmetic HomFA (ArithHomFA) is a prototype toolkit for oblivious online STL (signal temporal logic) monitoring, i.e., it enables STL monitoring without disclosing the log to the monitor. Note that we employ discrete-time semantics of STL, which is essentially LTL with arithmetic predicates.

Technically, ArithHomFA combines two FHE schemes (CKKS and TFHE) to enjoy their advantages, i.e., linear arithmetic operations and logical operations.

ArithHomFA consists of a command-line tool `ahomfa_util` and a library `libahomfa_runner.a` for monitoring. `ahomfa_util` is a command-line utility for the necessary FHE operations, such as key generation, encryption, and decryption. `ahomfa_util` also provides functionality for the transformation of an STL formula into a DFA specification. `libahomfa_runner.a` is a library to conduct the actual monitoring process. A user implements a class defining the predicate used in monitoring in C++, and by linking it with `libahomfa_runner.a`, the user can build a program for oblivious online monitoring.

Documentation
-------------
- User guide (MkDocs): [https://maswag.github.io/arith_homfa](https://maswag.github.io/arith_homfa)
- Developer manual (Doxygen): [https://maswag.github.io/arith_homfa/developer_manual/](https://maswag.github.io/arith_homfa/developer_manual/)

Both sites are rebuilt automatically from the files under `doc/` on every push to `master`.

Requirements
------------

### Supported platforms

The artifact is supported on Linux/x86-64 machines with AVX2 and FMA support. Use the distribution GCC from Ubuntu 24.04 or Debian 13.

macOS is not a supported platform for this artifact.

- Supported OS: 64-bit Ubuntu 24.04 LTS (noble) or Debian 13 (trixie).
- GCC
    - Use the distribution GCC from Ubuntu 24.04 or Debian 13. Clang is not supported (template instantiation issues with link-time optimization).
- CMake
- Boost
- [Microsoft SEAL](https://github.com/microsoft/SEAL): A CKKS library.
    - ArithHomFA works with SEAL >= 4.1.0; the Docker images build SEAL 4.3.2 by default.
    - See [here](https://github.com/microsoft/SEAL#getting-started) for the installation.
    - You may need to create a symbolic link with `sudo ln -s /usr/local/lib/libseal-4.3.a /usr/local/lib/libseal.a`.
- [Spot](https://spot.lre.epita.fr/): A library to handle temporal logic and omega automata. 
    - We tested with Spot ≥ 2.4.3.
    - See [this page](https://spot.lre.epita.fr/install.html) for the installation.

How to build
------------

The following shows how to build `ahomfa_util` and `libahomfa_runner.a` using CMake. See [this page](https://maswag.github.io/arith_homfa/installation/) for detailed instructions, especially dependency installation.

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ahomfa_util ahomfa_runner
```

### Native CPU tuning

By default, ArithHomFA uses `-march=native` for the best performance on the build machine. **If `-march=native` is not supported by the compiler, ArithHomFA automatically falls back to `-march=x86-64-v3` (x86-64-v3 baseline, AVX2 + FMA).** This is safe for local builds and benchmarks.

The published Docker images set `ARITHHOMFA_ENABLE_NATIVE_ARCH=OFF`, which also targets the **x86-64-v3** baseline for portable binaries.

To disable native tuning locally:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -D ARITHHOMFA_ENABLE_NATIVE_ARCH=OFF
```

If your compiler supports neither `-march=native` nor `-march=x86-64-v3`, check your toolchain and compiler version.

Check AVX2/FMA availability on Linux with:

```sh
lscpu | grep -i -E 'avx2|fma'
```

> [!WARNING]
> Do not use `-march=native` for binaries you plan to distribute to other machines.

Usage Example
-------------

### Ready-to-run demos

Each curated example under `examples/` ships an automation script that prepares the example workspace, generates or refreshes keys and DFA specifications, prepares plaintext data, encrypts it, runs the monitor in multiple modes, decrypts the TFHE verdicts, and checks that encrypted monitoring agrees with the plaintext baseline.

```sh
# Blood glucose monitoring (formula id is required; see --help for options)
./examples/run_bg.sh --formula 1 --mode fast

# Vehicle RSS monitoring (no script flags)
./examples/run_vrss.sh
```

`run_bg.sh` accepts `--formula`, `--dataset`, `--mode`, `--block-size`, and `--bootstrap` flags so you can choose among the curated blood-glucose formulas, datasets, and encrypted-monitor settings without editing example files. It maps the selected formula to a `blood_glucose_*` monitor, derives plaintext glucose samples from the CSV traces under `examples/blood_glucose/homfa-experiment/`, writes encrypted and decrypted outputs under `examples/blood_glucose/results/`, and prompts for result cleanup when run interactively.

`run_vrss.sh` has no command-line flags. It creates and builds `examples/build` when that directory is missing, prepares keys, specifications, plaintext traces, and CKKS inputs, runs plain, block, and reverse monitoring with the `vehicle_rss` binary, decrypts the block/reverse TFHE outputs, compares them with the plaintext baseline, writes `examples/vehicle_rss/result_*` files, and prompts for cleanup when run interactively.

Use the scripts for quick smoke tests, then switch to the manual steps below when adapting the workflow to your own predicates and traces.

### Outline

A typical usage of ArithHomFA is as follows.

1. The server side decides the following information.
    - The public information of the CKKS parameter.
    - The private monitored specification consisting of an LTL formula and the predicates.
2. The server side builds a monitor using the predicate in C++ and `libahomfa_runner.a`.
3. The server side constructs a monitor DFA from the LTL formula using `ahomfa_util ltl2spec`.
4. The client generates the following four keys:
    - Private key for the CKKS scheme with `ahomfa_util ckks genkey`.
    - Private key for the TFHE scheme with `ahomfa_util tfhe genkey`.
    - Relinearization key for the CKKS scheme with `ahomfa_util ckks genrelinkey`.
    - Bootstrapping key for the TFHE scheme with `ahomfa_util tfhe genbkey`.
5. The client encrypts a multi-dimensional signal using `ahomfa_util ckks enc`.
6. The client sends the encrypted signal, relinearization key, and the bootstrapping key to the server.
7. The server side runs the monitor feeding the above information and the DFA obtained from the LTL formula.
    - The result is output as a stream of TLWE ciphertexts encrypted by the TFHE scheme.
8. The server side sends the resulting ciphertexts to the client.
9. The client decrypts the result using `ahomfa_util tfhe dec`.

### Concrete example

The following shows a concrete example of ArithHomFA using the blood_glucose example under `./examples/blood_glucose`. We suppose that the current directory is the root of this repository.

#### Step 1: Information Decision

First, decide the CKKS parameter and the monitored specification.

In ArithHomFA, the CKKS parameter is given by a JSON file. Examples are provided in `./examples/blood_glucose/config.json` and `./examples/vehicle_rss/config.json`.

The monitored specification consists of the LTL formula and the predicate. An LTL formula is given in a format accepted by [Spot](https://spot.lre.epita.fr/) with atomic propositions p0, p1, ..., where pi is the i-th predicate. Concrete examples are shown in `examples/blood_glucose/bg1.ltl` and `examples/vehicle_rss/vrss.ltl`.

The predicate is given by an implementation of a C++ class `ArithHomFA::CKKSPredicate` in `./src/ckks_predicate.hh`. Here, we also have to specify an approximate upper bound of the difference between the given value and the threshold. A concrete example is shown in `examples/blood_glucose/blood_glucose_one.cc`.

#### Step 2: Monitor Building

Then, on the server side, build a monitor using the predicate in C++ and `libahomfa_runner.a`. A concrete CMake example is shown in `examples/blood_glucose/CMakeLists.txt`. For the blood-glucose example, run `cmake --build ./examples/build --target blood_glucose_one` after configuring `./examples/build`.

#### Step 3: Spec Transformation

In this step, we transform the LTL formula into a DFA specification using the `ltl2spec` subcommand of `ahomfa_util`. First, ensure that your LTL formula is properly formatted and saved (as shown in the blood glucose example). Then, run the following command:

```sh
./build/ahomfa_util ltl2spec \
  -e "$(cat ./examples/blood_glucose/bg1.ltl)" \
  --num-vars 1 \
  -o ./examples/blood_glucose/bg1.spec
```
This command reads the LTL formula from `bg1.ltl` (one predicate), converts it into a DFA specification, and saves the output into `bg1.spec`.

If you plan to use the reversed algorithm, convert the forward spec first (as above) and then run `spec2spec --reverse`:

```sh
./build/ahomfa_util spec2spec \
  --reverse \
  -i ./examples/blood_glucose/bg1.spec \
  -o ./examples/blood_glucose/bg1.reversed.spec
```

This command reads the DFA from `bg1.spec`, reverses it, and saves the result in `bg1.reversed.spec`. Use the reversed specification for the monitor’s `reverse` subcommand in Step 7.

#### Step 4: Key Generation

In the fourth step, the client generates four keys using the `ahomfa_util` command. The commands to generate these keys are as follows.

Generate the private key for the CKKS scheme:

```sh
./build/ahomfa_util ckks genkey -c ./examples/blood_glucose/config.json -o /tmp/ckks.key
```

Generate the private key for the TFHE scheme:

```sh
./build/ahomfa_util tfhe genkey -o /tmp/tfhe.key
```

Generate the relinearization key for the CKKS scheme:

```sh
./build/ahomfa_util ckks genrelinkey -c ./examples/blood_glucose/config.json -K /tmp/ckks.key -o /tmp/ckks.relinkey
```

Generate the bootstrapping key for the TFHE scheme:

```sh
./build/ahomfa_util tfhe genbkey -K /tmp/tfhe.key -o /tmp/tfhe.bkey -c ./examples/blood_glucose/config.json -S /tmp/ckks.key
```

All keys are saved to temporary files for later use.

#### Step 5: Signal Encryption

The client encrypts the multi-dimensional signal using the `ahomfa_util ckks enc` command. The command to run is as follows:

```sh
./build/ahomfa_util ckks enc -c ./examples/blood_glucose/config.json -K /tmp/ckks.key -o /tmp/data.ckks < ./examples/blood_glucose/input.txt
```

This command reads input data from `input.txt`, encrypts it using the CKKS scheme with the previously generated CKKS key, and saves the output to `data.ckks`.

#### Step 7: Monitor Execution

In this step, the server-side executes the monitor with the information obtained from previous steps. To do this, run:

```sh
./examples/build/blood_glucose/blood_glucose_one reverse \
  --reversed \
  -c ./examples/blood_glucose/config.json \
  -f ./examples/blood_glucose/bg1.reversed.spec \
  -r /tmp/ckks.relinkey \
  -b /tmp/tfhe.bkey \
  --bootstrapping-freq 200 \
  < /tmp/data.ckks > /tmp/result.tfhe
```

This command runs the monitoring process, which reads the encrypted data, relinearization key, bootstrapping key, and specification file, and produces a stream of TLWE ciphertexts encrypted by the TFHE scheme.

#### Step 9: Result Decryption

Finally, the client decrypts the result using the `ahomfa_util tfhe dec` command:

```sh
./build/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe
```

This command will decrypt the results produced in the monitoring step using the TFHE private key. The decrypted output is then printed to the console.

On the Predicate Definition in C++
----------------------------------

In ArithHomFA, a user has to provide an implementation of `ArithHomFA::CKKSPredicate` class in C++. This class specifies the arithmetic operations conducted in the monitor. This corresponds to the arithmetic expressions in the monitored STL formula. Concrete examples are in `examples/blood_glucose/blood_glucose_one.cc` and `examples/vehicle_rss/vrss_predicate.cc`. Here are some notes on its implementation. See [the example of Microsoft SEAL](https://github.com/microsoft/SEAL/blob/main/native/examples/5_ckks_basics.cpp) for the usage example of SEAL itself.

- Since we heavily use standard outputs, it is not allowed to print messages, for example, using `std::cout`. Instead, a user can use `spdlog` to print messages.
- Users must correctly implement arithmetic operations with Microsoft SEAL. For example, the user must correctly handle the scale of CKKS ciphertexts.
- We assume that the resulting CKKS ciphertext is in its last level. A user has to do it by `mod_switch` or `rescale` (depending on the internal status of the ciphertext).

See also
--------

- [HomFA](https://github.com/virtualsecureplatform/homfa): The original implementation for *oblivious* execution of DFAs.
- [TFHEpp](https://github.com/virtualsecureplatform/TFHEpp): The TFHE library we are using.
