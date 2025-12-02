Arithmetic HomFA (ArithHomFA)
=============================

[![Users Manual](https://img.shields.io/badge/docs-Users-blue)](https://maswag.github.io/arith_homfa/)
[![Developers Manual](https://img.shields.io/badge/docs-Dev-blue)](https://maswag.github.io/arith_homfa/developer_manual)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](./LICENSE)

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

- GCC
    - Currently, Clang is not supported because of an issue on link-time optimization of clang.
        - See [here](https://stackoverflow.com/questions/60225945/explicit-c-template-instantiation-with-clang) for the details of the issue.
    - We tested with GCC 10
- CMake
- Boost
- [Microsoft SEAL](https://github.com/microsoft/SEAL): A CKKS library.
    - We tested with SEAL 4.1.1.
    - See [here](https://spot.lre.epita.fr/install.html) for the installation.
    - You may need to create a symbolic link via `sudo ln -s /usr/local/lib/libseal-4.1.a /usr/local/lib/libseal.a`.
- [Spot](https://spot.lre.epita.fr/): A library to handle temporal logic and omega automata. 
    - We tested with Spot 2.10.4.
    - See [this page](https://spot.lre.epita.fr/install.html) for the installation.

How to build
------------

The following shows how to build `ahomfa_util` and `libahomfa_runner.a` using CMake.

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ahomfa_util libahomfa_runner.a
```

Usage Example
-------------

### Ready-to-run demos

Each curated example under `examples/` ships an automation script that builds the toolkit (if needed), generates keys and DFA specifications, produces sample data, encrypts it, runs the monitor, and decrypts the verdicts.

```sh
# Blood glucose monitoring (formula 1 by default; see --help for options)
./examples/run_bg.sh --formula 1 --mode fast

# Vehicle RSS monitoring
./examples/run_vrss.sh
```
`run_bg.sh` accepts `--formula`, `--dataset`, `--mode`, `--block-size`, and `--bootstrap` flags so you can swap between the curated STL formulas and clinical traces without re-editing the example files. `run_vrss.sh` rebuilds (if needed), runs the RSS workflow, decrypts the TFHE outputs, and sanity-checks that the plain, block, and reverse modes agree.

Under the hood, the scripts call the shared Make targets defined in every example directory:

- `make keys` – create CKKS/TFHE secret keys plus the relinearization/bootstrapping keys.
- `make specs` – transform the bundled LTL formulas (e.g., `bg1.ltl`, `vrss.ltl`) into DFA specs and reversed specs.
- `make sample_data` / `make encrypt_sample` – synthesize representative traces and encrypt them with CKKS.
- `make release` (blood_glucose) – build the monitor binary without having to rebuild the entire repository.

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

Then, on the server side, build a monitor using the predicate in C++ and `libahomfa_runner.a`. A concrete example of a `Makefile` is shown in `examples/blood_glucose/Makefile`. For a release build, run `make -C ./examples/blood_glucose release` or `make release` inside `examples/blood_glucose`.

#### Step 3: Spec Transformation

In this step, we transform the LTL formula into a DFA specification using the `ltl2spec` subcommand of `ahomfa_util`. First, ensure that your LTL formula is properly formatted and saved (as shown in the blood glucose example). Then, run the following command:

```sh
./build/ahomfa_util ltl2spec \
  -e "$(cat ./examples/blood_glucose/bg1.ltl)" \
  --num-vars 1 \
  -o ./examples/blood_glucose/bg1.spec
```
This command reads the LTL formula from `bg1.ltl` (one predicate), converts it into a DFA specification, and saves the output into `bg1.spec`.

If you are planning to use the reversed algorithm, it's also recommended to generate the reversed specification. The `ltl2spec` subcommand provides an option to emit reversed specifications directly via the `--reverse` flag. The command would be:

```sh
./build/ahomfa_util ltl2spec \
  -e "$(cat ./examples/blood_glucose/bg1.ltl)" \
  --num-vars 1 \
  --reverse \
  -o ./examples/blood_glucose/bg1.reversed.spec
```

This command reads the LTL formula, outputs the DFA, and saves the reversed version in `bg1.reversed.spec`. This reversed specification should be used for running the monitor in reverse mode in Step 7.

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
./examples/blood_glucose/blood_glucose_one reverse \
  --reversed \
  -c ./examples/blood_glucose/config.json \
  -f ./examples/blood_glucose/bg1.reversed.spec \
  -r /tmp/ckks.relinkey \
  -b /tmp/tfhe.bkey \
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
