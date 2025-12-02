# Blood Glucose Monitoring Example

## Overview

This example demonstrates the application of ArithHomFA for privacy-preserving blood glucose monitoring using fully homomorphic encryption. The system continuously monitors encrypted glucose readings to detect hypoglycemia events (glucose ≤ 70 mg/dL) without decrypting individual measurements, making it suitable for secure health monitoring applications where patient privacy is paramount.

## Medical Context

Hypoglycemia (low blood glucose) is a critical condition that requires immediate attention. The threshold of 70 mg/dL is clinically significant:
- **Normal range**: 70-140 mg/dL
- **Hypoglycemia**: ≤ 70 mg/dL (requires intervention)
- **Severe hypoglycemia**: < 54 mg/dL (medical emergency)

This monitoring system enables continuous surveillance of glucose levels while preserving patient privacy through encryption.

## Temporal Logic Formula

The monitoring uses Signal Temporal Logic (STL) to express safety properties over time windows:

### Primary Formula (bg1.ltl)
```
G[100,250]G[0,250]G[0,200](p0)
```

Where `p0` represents the predicate `glucose > 70`.

This nested temporal formula ensures:
1. **Inner G[0,200]**: The property holds for a 200-sample window
2. **Middle G[0,250]**: Extended monitoring over 250 samples
3. **Outer G[100,250]**: Delayed evaluation starting from sample 100

### Alternative Formulas
- **bg7.ltl**: Extended 7-predicate monitoring for comprehensive health metrics
- **gp0.ltl**: Simple globally formula for basic threshold monitoring

## Prerequisites

### Software Requirements
- C++ compiler with C++17 support
- CMake (>= 3.16)
- OpenMP for parallel processing
- Microsoft SEAL library (for CKKS encryption)
- TFHE library (for Boolean operations)

### Build Dependencies
```bash
# Install required packages (Ubuntu/Debian)
sudo apt-get install -y \
    build-essential \
    cmake \
    libomp-dev \
    libssl-dev
```

### Project Build
Ensure ArithHomFA is built before running this example:
```bash
cd ../../  # Navigate to project root
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## File Structure

```
blood_glucose/
├── bg_predicate.cc           # Predicate implementation for glucose threshold
├── blood_glucose_one.cc      # Main monitoring program (single predicate)
├── blood_glucose_seven.cc    # Extended monitoring (seven predicates)
├── blood_glucose_one_dummy.cc # Test implementation
├── bg1.ltl                   # Primary temporal logic formula
├── bg7.ltl                   # Extended formula with 7 predicates
├── gp0.ltl                   # Simple global formula
├── gp0.spec                  # Automaton specification
├── config.json               # CKKS encryption parameters
├── input.txt                 # Sample glucose readings
├── output.txt                # Expected output
├── Makefile                  # Build configuration
└── CMakeLists.txt            # CMake configuration
```
An orchestration script (`../run_bg.sh`) lives one directory up and drives the entire workflow end to end.

## Step-by-Step Usage Guide

### Quick Start (Automated)
```bash
cd ..  # from examples/blood_glucose to examples/
./run_bg.sh --formula 1 --mode fast
```

`run_bg.sh` automates the entire workflow. It builds binaries when needed, generates/reuses keys and DFA specs, converts the curated CSV traces in `homfa-experiment/`, runs plain/block/reverse monitoring, decrypts the TFHE verdicts, and compares the outputs. Use `./run_bg.sh --help` to discover additional options.

**Key flags**
- `--formula <id>` – choose among the shipped monitors (1, 2, 4, 5, 6, 7, 8, 10, 11). The script automatically maps each ID to the correct binary and dataset.
- `--dataset <name>` – override the default dataset. Input CSVs live under `homfa-experiment/ap9/`.
- `--mode <fast|normal|slow>` – propagate the performance profile to block/reverse runs (`fast` favors latency).
- `--block-size <n>` – size of ciphertext chunks for block mode.
- `--bootstrap <n>` – bootstrapping frequency for reverse mode (default 200).

### Manual Execution

#### 1. Generate Encryption Keys
```bash
make keys
```

This creates:
- `ckks.key`: Secret key for CKKS encryption
- `ckks.relinkey`: Relinearization key for homomorphic multiplication
- `tfhe.key`: Secret key for TFHE encryption
- `tfhe.bkey`: Bootstrapping key for circuit refresh

#### 2. Generate Automaton Specifications
```bash
make specs
```

Converts LTL formulas to deterministic finite automata (DFA):
```bash
../../build/src/ahomfa_util ltl2spec -n 1 -e "$(cat bg1.ltl)" -o bg1.spec
../../build/src/ahomfa_util ltl2spec -n 1 -e "$(cat bg1.ltl)" -o bg1.reversed.spec --reverse
```

#### 3. Prepare Sample Data
```bash
make sample_data
```

Generates realistic glucose readings:
- 500 samples with normal distribution (mean: 100 mg/dL, std: 15)
- Includes random hypoglycemia events for testing

#### 4. Encrypt Data
```bash
make encrypt_sample
```

Encrypts glucose readings using CKKS:
```bash
../../build/src/ahomfa_util ckks enc \
    -c config.json \
    -K ckks.key \
    -i adult_sample.bg.txt \
    -o adult_sample.bg.ckks
```

#### 5. Run Homomorphic Monitoring

**Reverse Mode (Optimized)**
```bash
../../build/examples/blood_glucose/blood_glucose_one reverse \
    -c config.json \
    -r ckks.relinkey \
    -b tfhe.bkey \
    -f bg1.reversed.spec \
    -m normal \
    -i adult_sample.bg.ckks \
    -o result.tfhe \
    --bootstrapping-freq 200 \
    --reversed
```

**Block Mode (Standard)**
```bash
../../build/examples/blood_glucose/blood_glucose_one block \
    -c config.json \
    -r ckks.relinkey \
    -b tfhe.bkey \
    -f bg1.spec \
    -i adult_sample.bg.ckks \
    -o result.tfhe
```

#### 6. Decrypt Results
```bash
../../build/src/ahomfa_util tfhe dec \
    -K tfhe.key \
    -i result.tfhe \
    -o result.txt
```

### Monitoring Modes

#### Plain Mode (No Encryption)
For testing and comparison:
```bash
./blood_glucose_one plain -c config.json -f gp0.spec < input.txt
```

#### Pointwise Evaluation
Evaluates predicate at each time point:
```bash
../../build/src/ahomfa_util ckks enc -c config.json -K ckks.key -i input.txt | \
./blood_glucose_one pointwise -c config.json | \
../../build/src/ahomfa_util ckks dec -c config.json -K ckks.key
```

## Expected Output

### Successful Monitoring
```
1
1
1
0  # Hypoglycemia detected (glucose ≤ 70)
1
1
0  # Another hypoglycemia event
1
...
```

- `1`: Property satisfied (glucose > 70 mg/dL)
- `0`: Property violated (hypoglycemia detected)

### Performance Metrics
Typical execution on modern hardware:
- Key generation: ~5 seconds
- Encryption (500 samples): ~2 seconds
- Monitoring: ~30-60 seconds
- Decryption: ~1 second
- Total memory: ~2-4 GB

## Technical Details

### CKKS Parameters

The configuration in `config.json`:
```json
{
    "SealConfig": {
        "poly_modulus_degree": 8192,
        "base_sizes": [60, 40, 60],
        "scale": 1099511627776
    }
}
```

**Parameter Explanation:**
- **poly_modulus_degree**: 8192
  - Determines ciphertext size and security level
  - Supports 4096 SIMD slots for parallel operations
  - Provides 128-bit security

- **base_sizes**: [60, 40, 60]
  - Modulus chain for multiplicative depth
  - Total: 160 bits
  - Supports 2 multiplications before bootstrapping

- **scale**: 2^40 (1099511627776)
  - Precision for fixed-point arithmetic
  - ~12 decimal digits of precision
  - Balances accuracy vs. noise growth

### Bootstrapping Strategy

The `--bootstrapping-freq 200` parameter controls circuit refresh:
- Bootstrapping every 200 operations
- Prevents noise overflow
- Maintains computation accuracy
- Trade-off: Higher frequency = more overhead but better precision

### Memory Optimization

For large datasets:
```bash
# Process in chunks
split -l 1000 large_dataset.txt chunk_
for chunk in chunk_*; do
    ./process_chunk.sh $chunk
done
```

## Troubleshooting

### Common Issues and Solutions

#### 1. Out of Memory
**Symptom**: Process killed or segmentation fault
**Solution**: 
- Reduce `poly_modulus_degree` to 4096
- Process smaller data batches
- Increase system swap space

#### 2. Decryption Failures
**Symptom**: Garbage values or all zeros in output
**Solution**:
- Verify key consistency
- Check bootstrapping frequency
- Ensure proper scale management

#### 3. Slow Performance
**Symptom**: Monitoring takes > 5 minutes
**Solution**:
```bash
# Enable OpenMP parallelization
export OMP_NUM_THREADS=$(nproc)
# Use Release build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

#### 4. Compilation Errors
**Symptom**: Missing headers or undefined references
**Solution**:
```bash
# Clean and rebuild
make clean
rm -rf ../../build
mkdir -p ../../build && cd ../../build
cmake .. && make -j$(nproc)
```

### Debug Mode

Enable verbose output for troubleshooting:
```bash
export SPDLOG_LEVEL=debug
export SEAL_THROW_ON_TRANSPARENT=1
cd .. && ./run_bg.sh --formula 1 2>&1 | tee debug.log
```

## Advanced Usage

### Custom Predicates

Modify [`bg_predicate.cc`](bg_predicate.cc:1) to implement custom thresholds:
```cpp
class CustomPredicate : public CKKSPredicate {
    seal::Ciphertext operator()(const Variables& vars) {
        // Custom logic here
        auto glucose = vars.at("glucose");
        auto threshold = encode(85.0);  // Custom threshold
        return compare_greater(glucose, threshold);
    }
};
```

### Batch Processing

Process multiple patients simultaneously:
```bash
for patient in patient_*.txt; do
    echo "Processing $patient"
    ./process_patient.sh $patient &
done
wait
```

### Integration with Health Systems

Example HL7 FHIR integration:
```python
# Pseudo-code for FHIR integration
def process_glucose_observation(observation):
    glucose_value = observation.value
    encrypted = encrypt_ckks(glucose_value)
    result = monitor_homomorphic(encrypted)
    if decrypt_tfhe(result) == 0:
        trigger_hypoglycemia_alert(observation.patient_id)
```

## Performance Analysis

### Benchmarking
```bash
# Run performance tests
make benchmark

# Profile execution
perf record ./blood_glucose_one reverse -c config.json ...
perf report
```

### Optimization Tips
1. Use SIMD slots efficiently (batch multiple readings)
2. Minimize bootstrapping operations
3. Optimize predicate evaluation order
4. Use appropriate parallelization strategies

## References

1. **ArithHomFA Paper**: "Oblivious Monitoring for Discrete-Time STL via Fully Homomorphic Encryption" (RV'24)
2. **CKKS Scheme**: Cheon et al., "Homomorphic Encryption for Arithmetic of Approximate Numbers"
3. **Medical Guidelines**: American Diabetes Association Standards of Medical Care
4. **SEAL Library**: [Microsoft SEAL Documentation](https://github.com/microsoft/SEAL)
5. **TFHE Library**: [TFHE Documentation](https://tfhe.github.io/tfhe/)

## License

This example is part of the ArithHomFA project. See the main project LICENSE for terms.

## Support

For issues or questions:
- Open an issue in the ArithHomFA repository
- Contact the maintainers via the project page
- Consult the [main documentation](../../README.md)
