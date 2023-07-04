Arithmetic HomFA (ArithHomFA)
=============================

Arithmetic HomFA (ArithHomFA) is a prototype toolkit for oblivious online STL (signal temporal logic) monitoring, i.e., it enables an STL monitoring without disclosing the log to the monitor. ArithHomFA consists of a command line tool `ahomfa_util` to handle input and output files and a library `libahomfa_runner.a` for monitoring.

Technically, Arithmetic HomFA combines two FHE schemes (CKKS and TFHE) to enjoy both of their advantages, i.e., linear arithmetic operations and logical operations.

Requirements
------------

- GCC
    - Currently, Clang is not supported because of an issue on link-time optimization of clang.
        - See [here](https://stackoverflow.com/questions/60225945/explicit-c-template-instantiation-with-clang) for the detail of the issue.
    - We tested with GCC 10
- CMake
- Boost
- [Microsoft SEAL](https://github.com/microsoft/SEAL): An CKKS library.
    - We tested with SEAL 4.1.1.
    - See [here](https://spot.lre.epita.fr/install.html) for the installation.
- [Spot](https://spot.lre.epita.fr/): A library to handle temporal logic and omega automata. 
    - We tested with Spot 2.10.4.
    - See [this page](https://spot.lre.epita.fr/install.html) for the installation.

How to build
------------

The following shows how to build `ahomfa_util` and `libahomfa_runner.a` using cmake.

```sh
git submodule update --init --recursive
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -- ahomfa_util libahomfa_runner.a
```

Usage Example
-------------

### Outline

A typical usage of ArithHomFA is as follows.

1. The server-side decides the following information.
    - The public information of the CKKS parameter.
    - The private monitored specification consisting of an LTL formula and the predicates.
2. The server-side builds a monitor using the predicate in C++ and `libahomfa_runner.a`
3. The server-side constructs a monitor DFA from the LTL formula using `ahomfa_util ltl2spec`
4. The client generates the following four keys:
    - Private key for the CKKS scheme with `ahomfa_util ckks genkey`.
    - Private key for the TFHE scheme with `ahomfa_util tfhe genkey`.
    - Relinearization key for the CKKS scheme with `ahomfa_util ckks genrelinkey`.
    - Bootstrapping key for the TFHE scheme with `ahomfa_util tfhe genbkey`.
5. The client encrypts a multi-dimensional signal using `ahomfa_util ckks enc`.
6. The client sends the encrypted signal, relinearization key, and the bootstrapping key to the server.
7. The server-side runs the monitor feeding the above information and the DFA obtained from the LTL formula.
    - The result is output as a stream of TLWE cipher texts encrypted by the TFHE scheme.
8. The server-side sends the resulting cipher texts to the client.
9. The client decrypts the result using `ahomfa_util tfhe dec`.

### Concrete example

The following shows a concrete example of ArithHomFA using the blood_glucose example under `./examples/blood_glucose`. We suppose that the current directory is the root of this repository.

#### Step 1: Information Decision

First, a user has to decide the CKKS parameter and the monitored specification.

In ArithHomFA, the CKKS parameter is given by a JSON file. An example is in `./examples/config.json`.

The monitored specification consists of the LTL formula and the predicate. An LTL formula is given by an format accepted by [Spot](https://spot.lre.epita.fr/) with atomic propositions p0, p1, ..., where pi is the i-th predicate. A concrete example is show in `examples/blood_glucose/gp0.ltl`.

The predicate is given by an implementation of a C++ class `ArithHomFA::CKKSPredicate` in `./src/ckks_predicate.hh`. Here, we also have to specify an approximate upper bound of the difference between the given value and the threshold. A concrete example is show in `examples/blood_glucose/blood_glucose_one.cc`.

#### Step 2: Monitor Building

Then, on the server-side, one builds a monitor using the predicate in C++ and `libahomfa_runner.a`. A concrete example of `Makefile` is shown in `examples/blood_glucose/Makefile`. For release build, one can build the monitor `blood_glucose_one` by `make -C ./examples/blood_glucose/ release` or `make release` at `examples/blood_glucose`.

#### Step 3: Spec Transformation

In this step, we transform the LTL formula into a DFA specification using the  `ltl2spec` subcommand of `ahomfa_util`. First, ensure that your LTL formula is properly formatted and saved (as shown in the blood glucose example). Then, run the following command:

```sh
./build/ahomfa_util ltl2spec -e $(cat ./examples/blood_glucose/gp0.ltl) --num-vars 1 > ./examples/blood_glucose/gp0.spec
```
This command will read the LTL formula from the provided file (`gp0.ltl`) with one predicate, convert it into a DFA specification, and save the output into `gp0.spec`.

#### Step 4: Key Generation

In the fourth step, the client generates four keys using the `ahomfa_util` command. The commands to generate these keys are as follows.

Generate the private key for the CKKS scheme:

```sh
./build/ahomfa_util ckks genkey -c ./example/config.json -o /tmp/ckks.key
```

Generate the private key for the TFHE scheme:

```sh
./build/ahomfa_util tfhe genkey -o /tmp/tfhe.key
```

Generate the relinearization key for the CKKS scheme:

```sh
./build/ahomfa_util ckks genrelinkey -c ./example/config.json -K /tmp/ckks.key -o /tmp/ckks.relinkey
```

Generate the bootstrapping key for the TFHE scheme:

```sh
./build/ahomfa_util tfhe genbkey -K /tmp/tfhe.key -o /tmp/tfhe.bkey -c ./example/config.json -S /tmp/ckks.key
```

All keys are saved to temporary files for later use.

#### Step 5: Signal Encryption

The client encrypts the multi-dimensional signal using the `ahomfa_util ckks enc` command. The command to run is as follows:

```sh
./build/ahomfa_util ckks enc -c ./example/config.json -K /tmp/ckks.key -o /tmp/data.ckks < ./examples/blood_glucose/input.txt
```

This command reads input data from `input.txt`, encrypts it using the CKKS scheme with the previously generated CKKS key, and saves the output to `data.ckks`.

#### Step 7: Monitor Execution

In this step, the server-side executes the monitor with the information obtained from previous steps. To do this, run:

```sh
./examples/blood_glucose/blood_glucose_one reverse -c ./example/config.json -f ./examples/blood_glucose/gp0.spec -r /tmp/ckks.relinkey -b /tmp/tfhe.bkey < /tmp/data.ckks > /tmp/result.tfhe
```

This command runs the monitoring process, which reads the encrypted data, relinearization key, bootstrapping key, and specification file, and produces a stream of TLWE cipher texts encrypted by the TFHE scheme.

#### Step 9: Result Decryption

Finally, the client decrypts the result using the `ahomfa_util tfhe dec` command:

```sh
./build/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe
```

This command will decrypt the results produced in the monitoring step using the TFHE private key. The decrypted output is then printed to the console.

Usage of `ahomfa_util`
----------------------



See also
--------

- [HomFA](https://github.com/virtualsecureplatform/homfa): The original implementation for *oblivious* execution of DFAs.
- [TFHEpp](https://github.com/virtualsecureplatform/TFHEpp/tree/master/include): The TFHE library we are using.
