# ArithHomFA Examples

This directory contains practical examples demonstrating the usage of ArithHomFA (Arithmetic Homomorphic Finite Automata) for oblivious runtime verification of numeric data using fully homomorphic encryption. These examples showcase how to monitor temporal properties over encrypted data streams without revealing the underlying values.

## Overview

ArithHomFA enables privacy-preserving monitoring by executing finite automata computations on encrypted data. The system combines:
- **CKKS encryption** for arithmetic operations on encrypted floating-point numbers
- **TFHE encryption** for Boolean operations and automata state transitions
- **Temporal logic monitoring** using Signal Temporal Logic (STL) formulas

## Available Examples

### 1. [Blood Glucose Monitoring](blood_glucose/)
Demonstrates continuous monitoring of blood glucose levels to detect hypoglycemia events (glucose ≤ 70 mg/dL) over encrypted patient data streams.

**Key Features:**
- Monitors temporal property: `G[100,250]G[0,250]G[0,200](glucose > 70)`
- Processes encrypted glucose readings
- Detects hypoglycemia events without decrypting individual measurements
- Suitable for privacy-preserving health monitoring applications

### 2. [Vehicle RSS (Responsibility-Sensitive Safety)](vehicle_rss/)
Implements safety monitoring for autonomous vehicles using RSS predicates to ensure safe driving behavior over encrypted sensor data.

**Key Features:**
- Monitors safety predicates: `G[0,15](p0 & p1 & p2 & p3)`
- Evaluates multiple safety conditions simultaneously
- Processes encrypted vehicle telemetry data
- Applicable to privacy-preserving autonomous vehicle systems

## Prerequisites

### System Requirements
- Linux-based system (Ubuntu 20.04+ recommended)
- C++ compiler with C++17 support (GCC 9+ or Clang 10+)
- CMake (>= 3.16)
- Make or Ninja build system
- OpenMP support for parallel processing

### Dependencies
```bash
# Install required packages (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libomp-dev \
    libtbb-dev \
    libssl-dev
```

### Building ArithHomFA
Before running the examples, build the main ArithHomFA project:

```bash
# From the project root directory
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## General Usage Pattern

Each example follows a similar workflow:

### 1. Key Generation
Generate encryption keys for both CKKS and TFHE schemes:
```bash
cd examples/<example_name>
make keys
```

### 2. Specification Generation
Convert temporal logic formulas to automata specifications:
```bash
make specs
```

### 3. Data Preparation
Create and encrypt sample data:
```bash
make sample_data
make encrypt_sample
```

### 4. Homomorphic Monitoring
Run the monitoring algorithm on encrypted data:
```bash
./run_example.sh
```

### 5. Result Decryption
Decrypt and interpret monitoring results:
```bash
../../build/src/ahomfa_util tfhe dec -K tfhe.key -i result.tfhe -o result.txt
```

## Quick Start

Each example includes a convenient script that automates the entire workflow:

```bash
# For Blood Glucose example
cd examples/blood_glucose
./run_example.sh

# For Vehicle RSS example
cd examples/vehicle_rss
./run_example.sh
```

## Configuration

Both examples use CKKS encryption with the following default parameters:

```json
{
    "SealConfig": {
        "poly_modulus_degree": 8192,
        "base_sizes": [60, 40, 60],
        "scale": 1099511627776
    }
}
```

These parameters provide:
- **Security level**: 128-bit
- **Precision**: ~40 bits for floating-point operations
- **Multiplicative depth**: 2 levels before bootstrapping
- **Ciphertext slots**: 4096 (for SIMD operations)

## Monitoring Modes

ArithHomFA supports different monitoring modes:

### Plain Mode
Classical monitoring without encryption (for testing/comparison):
```bash
./monitor plain -c config.json -f spec_file < input.txt
```

### Pointwise Mode
Evaluates predicates at each time point:
```bash
./monitor pointwise -c config.json < encrypted_input
```

### Block Mode
Processes data in blocks with periodic bootstrapping:
```bash
./monitor block -c config.json -f spec_file -r relinkey -b bootkey < encrypted_input
```

### Reverse Mode
Optimized monitoring with reversed automata:
```bash
./monitor reverse -c config.json -f spec_file -r relinkey -b bootkey --reversed < encrypted_input
```

## Performance Considerations

- **Bootstrapping frequency**: Adjust `--bootstrapping-freq` parameter based on circuit depth
- **Batch processing**: Use SIMD slots for parallel evaluation
- **Memory usage**: Monitor memory consumption for large data streams
- **Computation time**: Expect 10-100x overhead compared to plaintext monitoring

## Troubleshooting

### Common Issues

1. **Build failures**: Ensure all dependencies are installed and CMake version is >= 3.16
2. **Out of memory**: Reduce batch size or increase system RAM
3. **Slow performance**: Enable OpenMP and use Release build mode
4. **Key generation errors**: Check write permissions in the example directory

### Debug Mode

Enable debug output for detailed monitoring information:
```bash
export AHOMFA_DEBUG=1
./run_example.sh
```

## Further Reading

- [Blood Glucose Example Documentation](blood_glucose/README.md)
- [Vehicle RSS Example Documentation](vehicle_rss/README.md)
- [ArithHomFA Paper](https://doi.org/10.1007/978-3-031-74234-7_11) - RV'24 Conference
- [CKKS Encryption Scheme](https://eprint.iacr.org/2016/421.pdf)
- [TFHE Library Documentation](https://tfhe.github.io/tfhe/)

## License

This project is part of academic research. Please refer to the main project LICENSE file for usage terms.

## Contact

For questions or issues related to these examples, please open an issue in the main ArithHomFA repository or contact the maintainers through the project page.