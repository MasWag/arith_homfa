# ahomfa_util Usage Manual

## Name

ahomfa_util - Utility for ArithHomFA, a project for oblivious online Signal Temporal Logic (STL) monitoring.

## Synopsis

    ahomfa_util [OPTIONS] [SUBCOMMAND]

## Description

**ahomfa_util** is a command-line utility that provides functionalities such as key generation, encryption, and decryption using CKKS and TFHE schemes, as well as converting LTL formulas to specifications in the context of oblivious online STL monitoring.

## Global Options

**-h**, **--help**
: Print a help message.

**--verbose**
: Detailed mode. Provides additional output.

**--quiet**
: Quiet mode. Suppresses most output.

## Subcommands

### ckks

**Usage**: `./ahomfa_util ckks [OPTIONS] [SUBCOMMAND]`

Options related to the CKKS scheme in SEAL.

**Subcommands**:

- **genkey**: Generate a CKKS secret key.
- **genrelinkey**: Generate a CKKS relinearization key.
- **enc**: Encrypt an input file under the CKKS scheme.
- **dec**: Decrypt an input file under the CKKS scheme.

### tfhe

**Usage**: `./ahomfa_util tfhe [OPTIONS] [SUBCOMMAND]`

Options related to the TFHE scheme.

**Subcommands**:

- **genkey**: Generate a TFHE secret key.
- **genbkey**: Generate a TFHE bootstrap key.
- **enc**: Encrypt an input file under the TFHE scheme.
- **dec**: Decrypt an input file under the TFHE scheme.

### ltl2spec

**Usage**: `./ahomfa_util ltl2spec [OPTIONS]`

Convert an LTL formula to the specification format suitable for ArithHomFA.

### spec2spec

**Usage**: `./ahomfa_util spec2spec [OPTIONS]`

Modify a specification for ArithHomFA.

## Options and Subcommands Details

### CKKS Subcommands Options

#### genkey

**Usage**: `./ahomfa_util ckks genkey [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-c**, **--config** (REQUIRED): The path to the configuration file of SEAL
- **-o**, **--output**: The path to the output file for writing the result

#### genrelinkey

**Usage**: `./ahomfa_util ckks genrelinkey [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-c**, **--config** (REQUIRED): The path to the configuration file of SEAL
- **-o**, **--output**: The path to the output file for writing the result
- **-K**, **--secret-key** (REQUIRED): The path to the secret key of SEAL

#### enc

**Usage**: `./ahomfa_util ckks enc [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-c**, **--config** (REQUIRED): The path to the configuration file of SEAL
- **-o**, **--output**: The path to the output file for writing the result
- **-K**, **--secret-key** (REQUIRED): The path to the secret key of SEAL
- **-i**, **--input**: The path to the input file

#### dec

**Usage**: `./ahomfa_util ckks dec [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-c**, **--config** (REQUIRED): The path to the configuration file of SEAL
- **-o**, **--output**: The path to the output file for writing the result
- **-K**, **--secret-key** (REQUIRED): The path to the secret key of SEAL
- **-i**, **--input**: The path to the input file

### TFHE Subcommands Options

#### genkey

**Usage**: `./ahomfa_util tfhe genkey [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-o**, **--output**: The path to the output file for writing the result

#### genbkey

**Usage**: `./ahomfa_util tfhe genbkey [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-o**, **--output**: The path to the output file for writing the result
- **-K**, **--secret-key** (REQUIRED): The path to the secret key of TFHEpp
- **-c**, **--config** (REQUIRED): The path to the configuration file of SEAL
- **-S**, **--seal-secret-key** (REQUIRED): The path to the secret key of SEAL

#### enc

**Usage**: `./ahomfa_util tfhe enc [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-o**, **--output**: The path to the output file for writing the result
- **-K**, **--secret-key** (REQUIRED): The path to the secret key of TFHEpp
- **-i**, **--input**: The path to the input file

#### dec

**Usage**: `./ahomfa_util tfhe dec [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-o**, **--output**: The path to the output file for writing the result
- **-K**, **--secret-key** (REQUIRED): The path to the secret key of TFHEpp
- **-i**, **--input**: The path to the input file

### ltl2spec Options

**Usage**: `./ahomfa_util ltl2spec [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-e**, **--formula** (REQUIRED): The LTL formula
- **-o**, **--output**: The path to the output file for writing the result
- **-n**, **--num-vars** (REQUIRED): The number of variables in the given LTL formula
- **--make-all-live-states-final**: If make all live states final

### spec2spec Options

**Usage**: `./ahomfa_util spec2spec [OPTIONS]`

- **-h**, **--help**: Print a help message and exit
- **-i**, **--input**: The file to load the input
- **--reverse** BOOLEAN: Reverse the given specification
- **--negate** BOOLEAN: Negate the given specification
- **--minimize** BOOLEAN: Minimize the given specification
- **-o**, **--output**: The file to write the result

## Exit Status

0
: if there is no error.

1
: if an error has occurred.

## Example

The following is an example of generating a CKKS secret key using a configuration file **config.json** and writing the output to **ckks.key**.

`./build/ahomfa_util ckks genkey -c ./example/config.json -o /tmp/ckks.key`

The command to decrypt the result of the monitor execution is as follows.

`./build/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe` 

This command reads the TFHE secret key from **/tmp/tfhe.key**, the input to decrypt from **/tmp/result.tfhe**, and outputs the result to stdout. 

Here is an example of using the spec2spec command:

`./build/ahomfa_util spec2spec --reverse true --input ./example/specification.spec --output /tmp/specification_reversed.spec`

This command reads the specification from **./example/specification.spec**, reverses the specification, and writes the result to **/tmp/specification_reversed.spec**.

By default, ahomfa_util reads input from **stdin** and writes output to **stdout** if no input or output file is specified.