# 5. Core Concepts and Workflow

- **Input model.** Traces are vectors of reals encrypted with CKKS. Each predicate consumes a CKKS ciphertext slot per timestamp. Time is discrete and monotonically increasing.
- **Specification model.** Users describe STL properties as Spot-compatible LTL formulas. `ahomfa_util ltl2spec` translates them into DFA specifications; `spec2spec` can reverse, negate, or minimize automata to match a chosen execution mode.
- **Monitor architecture.** Users implement the CKKS predicate logic by subclassing `ArithHomFA::CKKSPredicate` (see `src/ckks_predicate.hh`), compile that C++ code into their monitor binary, and link it with `libahomfa_runner.a`. The runner library handles DFA progression, TFHE transitions, and repeatedly calls the user predicates for each timestep.
- **Execution modes.** `blood_glucose_one` demonstrates plain (unencrypted), pointwise predicate evaluation, block-based oblivious execution, and fully reversed monitoring for online verdicts.
- **Outputs.** Monitoring emits TFHE ciphertexts encoding boolean verdicts. `tfhe dec` produces cleartext verdict streams plus optional debug logs via spdlog.
- **Keys and trust.** Clients generate CKKS/TFHE keys and send only the public artifacts (relinearization and bootstrapping keys, encrypted trace) to the server. Private keys stay client-side for the final decryption.

## CKKS configuration (`config.json`)

ArithHomFA tools expect a JSON file describing the CKKS parameters. The sample at `examples/config.json` looks like this:

```json
{
  "SealConfig": {
    "poly_modulus_degree": 8192,
    "base_sizes": [60, 40, 60],
    "scale": 1099511627776
  }
}
```

- `poly_modulus_degree` controls slot capacity and noise budget; pick powers of two supported by SEAL (e.g., 4096, 8192, 16384) based on accuracy/performance needs.
- `base_sizes` lists the coefficient modulus bit-lengths for each level. You can extend or shrink the list for deeper circuits or faster evaluation.
- `scale` is the default CKKS scale applied when encoding plaintexts. Match it to the modulus sizes so that ciphertexts stay normalized after multiplications.

All `ahomfa_util ckks ...` commands and monitor binaries take `-c /path/to/config.json` to load these parameters, ensuring the predicates and DFA runner share the same CKKS context.
