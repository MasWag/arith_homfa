Arithmetic HomFA
================

Arithmetic HomFA (arith-homfa) is a prototype toolkit for oblivious online STL (signal temporal logic) monitoring, i.e., it enables an STL monitoring without disclosing the log to the monitor.

Technically, Arithmetic HomFA combines two FHE schemes (CKKS and TFHE) to enjoy both of their advantages, i.e., linear arithmetic operations and logical operations.

Usage
-----

Requirements
------------

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
