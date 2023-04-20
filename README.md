Arithmetic HomFA (ArithHomFA)
=============================

Arithmetic HomFA (ArithHomFA) is a prototype toolkit for oblivious online STL (signal temporal logic) monitoring, i.e., it enables an STL monitoring without disclosing the log to the monitor.

Technically, Arithmetic HomFA combines two FHE schemes (CKKS and TFHE) to enjoy both of their advantages, i.e., linear arithmetic operations and logical operations.

Usage
-----

```sh
# Generate a secret key of CKKS
./build/ahomfa ckks genkey -f ./example/config.json -o /tmp/ckks.key

# Encrypt an input
./build/ahomfa ckks enc -f ./example/config.json -o /tmp/data.ckks < data.txt
```

Requirements
------------

- CMake
- Boost
- [Microsoft SEAL](https://github.com/microsoft/SEAL): An CKKS library. We tested with SEAL 4.1.1.

Installation
------------

```sh
git submodule update --init --recursive
```

See also
--------

- [HomFA](https://github.com/virtualsecureplatform/homfa): The original implementation for *oblivious* execution of DFAs.
- [TFHEpp](https://github.com/virtualsecureplatform/TFHEpp/tree/master/include): The TFHE library we are using.
