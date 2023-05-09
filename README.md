Arithmetic HomFA (ArithHomFA)
=============================

Arithmetic HomFA (ArithHomFA) is a prototype toolkit for oblivious online STL (signal temporal logic) monitoring, i.e., it enables an STL monitoring without disclosing the log to the monitor. ArithHomFA consists of a command line tool `ahomfa_util` to handle input and output files and a library `libahomfa_runner.a` for monitoring.

Technically, Arithmetic HomFA combines two FHE schemes (CKKS and TFHE) to enjoy both of their advantages, i.e., linear arithmetic operations and logical operations.

Usage
-----

```sh
# Build homfa_util and libhomfa_runner
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -- homfa_util libhomfa_runner.a

# Generate a secret key of CKKS
./build/ahomfa_util ckks genkey -c ./example/config.json -o /tmp/ckks.key
# Generate a relinearlization key of CKKS
./build/ahomfa_util ckks genrelinkey -c ./example/config.json -K /tmp/ckks.key -o /tmp/ckks.relinkey

# Generate a secret key of TFHE
./build/ahomfa_util tfhe genkey -o /tmp/tfhe.key
# Generate a bootstrapping key of TFHE
./build/ahomfa_util tfhe genbkey -K /tmp/tfhe.key -o /tmp/tfhe.bkey

# Encrypt an input
./build/ahomfa_util ckks enc -c ./example/config.json -K /tmp/ckks.key -o /tmp/data.ckks < ./examples/blood_glucose/input.txt

# Make the specification from the formula
./build/ahomfa_util ltl2spec -e $(cat ./examples/blood_glucose/gp0.ltl) > ./examples/blood_glucose/gp0.spec

# Conduct monitoring
make -C ./examples/blood_glucose/ release
./examples/blood_glucose/blood_glucose_one block -c ./example/config.json -f ./examples/blood_glucose/gp0.spec -r /tmp/ckks.relinkey -b /tmp/tfhe.bkey < /tmp/data.ckks > /tmp/result.tfhe

# Decrypt the result
./build/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe
```

Requirements
------------

- CMake
- Boost
- [Microsoft SEAL](https://github.com/microsoft/SEAL): An CKKS library. We tested with SEAL 4.1.1.
- [Spot](https://spot.lre.epita.fr/): A library to handle temporal logic and omega automata. We tested with Spot 2.10.4.

Installation
------------

```sh
git submodule update --init --recursive
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -- homfa_util libhomfa_runner.a
```

Detailed usage
--------------

The detailed usage of Arithmetic HomFA (ArithHomFA) involves the following steps:

1. Represent the STL formula as an LTL formula in a .ltl file and a C++ class representing the arithmetic propositions. A concrete example can be found in the `./examples/blood_glucose/` directory.
2. Convert the LTL formula in the .ltl file to a .spec file using the `ahomfa_util ltl2spec` command.
3. Create a C++ class for the arithmetic propositions called `CKKSPredicate`, which will be compiled and linked with the `libahomfa_runner.a` library. The resulting executable file serves as the monitor.
4. Feed the .spec file to the monitor. The following shows a concrete example: `./examples/blood_glucose/blood_glucose_one block -c ./example/config.json -f ./examples/blood_glucose/gp0.spec -r /tmp/ckks.relinkey -b /tmp/tfhe.bkey < /tmp/data.ckks > /tmp/result.tfhe`


See also
--------

- [HomFA](https://github.com/virtualsecureplatform/homfa): The original implementation for *oblivious* execution of DFAs.
- [TFHEpp](https://github.com/virtualsecureplatform/TFHEpp/tree/master/include): The TFHE library we are using.
