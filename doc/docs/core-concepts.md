# 5. Core Concepts and Workflow

- **Input model.** Traces are vectors of reals encrypted with CKKS. Each predicate consumes a CKKS ciphertext slot per timestamp. Time is discrete and monotonically increasing.
- **Specification model.** Users describe STL properties as Spot-compatible LTL formulas. `ahomfa_util ltl2spec` translates them into DFA specifications; `spec2spec` can reverse, negate, or minimize automata to match a chosen execution mode.
- **Monitor architecture.** Users implement the CKKS predicate logic by subclassing `ArithHomFA::CKKSPredicate` (see `src/ckks_predicate.hh`), compile that C++ code into their monitor binary, and link it with `libahomfa_runner.a`. The runner library handles DFA progression, TFHE transitions, and repeatedly calls the user predicates for each timestep.
- **Execution modes.** `blood_glucose_one` demonstrates plain (unencrypted), pointwise predicate evaluation, block-based oblivious execution, and fully reversed monitoring for online verdicts.
- **Outputs.** Monitoring emits TFHE ciphertexts encoding boolean verdicts. `tfhe dec` produces cleartext verdict streams plus optional debug logs via spdlog.
- **Keys and trust.** Clients generate CKKS/TFHE keys and send only the public artifacts (relinearization and bootstrapping keys, encrypted trace) to the server. Private keys stay client-side for the final decryption.

## CKKS configuration (`config.json`)

ArithHomFA tools expect a JSON file describing the CKKS parameters. The samples at `examples/blood_glucose/config.json` and `examples/vehicle_rss/config.json` look like this:

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

## End-to-end monitoring workflow

ArithHomFA splits responsibilities between a **server** (who owns the monitor and DFA) and a **client** (who owns the sensitive trace). The numbered steps below expand the high-level bullets from the repository README and tie them to the manual sections.

1. **Decide public parameters and a private specification (server).** Pick CKKS parameters (a JSON file such as `examples/blood_glucose/config.json`) and specify the monitoring property as an STL/LTL formula together with its predicates. Store formulas as Spot-compatible strings (e.g., `examples/blood_glucose/bg1.ltl`) and implement predicates by subclassing `ArithHomFA::CKKSPredicate` (see [Integration](integration.md)).
2. **Build a monitor binary (server).** Link your predicate implementation with `libahomfa_runner.a` to produce a monitor (the tutorial uses `examples/blood_glucose/blood_glucose_one`). See [Quick Start §3.3](quick-start.md#33-compile-the-tutorial-monitor) for Makefile targets.
3. **Transform STL/LTL into DFA specs (server).** Use `ahomfa_util ltl2spec` on the plaintext formula and optionally reverse the resulting DFA for backward monitoring:

    ```sh
    ./build/ahomfa_util ltl2spec \
      -n 1 \
      -e "$(cat examples/blood_glucose/bg1.ltl)" \
      -o /tmp/bg1.spec
    ./build/ahomfa_util spec2spec \
      --reverse \
      -i /tmp/bg1.spec \
      -o /tmp/bg1.rev.spec
    ```

    (See [Specification Language](spec-language.md) for flags such as `--num-vars`.)

4. **Generate encryption keys (client).** The client keeps the CKKS/TFHE secret keys private but shares the relinearization and bootstrapping keys with the server:

    ```sh
    ./build/ahomfa_util ckks genkey \
      -c examples/blood_glucose/config.json -o /tmp/ckks.key
    ./build/ahomfa_util tfhe genkey \
      -o /tmp/tfhe.key
    ./build/ahomfa_util ckks genrelinkey \
      -c examples/blood_glucose/config.json -K /tmp/ckks.key \
      -o /tmp/ckks.relinkey
    ./build/ahomfa_util tfhe genbkey \
      -K /tmp/tfhe.key -c examples/blood_glucose/config.json \
      -S /tmp/ckks.key -o /tmp/tfhe.bkey
    ```

5. **Encrypt the trace (client).** The client feeds plaintext samples into `ahomfa_util ckks enc` and ships only the ciphertext to the server:

    ```sh
    ./build/ahomfa_util ckks enc \
      -c examples/blood_glucose/config.json \
      -K /tmp/ckks.key \
      -o /tmp/data.ckks < examples/blood_glucose/input.txt
    ```

6. **Run the encrypted monitor (server).** The server executes the monitor with the DFA, encrypted trace, and public keys. The reversed mode shown below matches the tutorial:

    ```sh
    ./examples/build/blood_glucose/blood_glucose_one reverse \
      --reversed \
      -c examples/blood_glucose/config.json \
      -f /tmp/bg1.rev.spec \
      -r /tmp/ckks.relinkey \
      -b /tmp/tfhe.bkey \
      --bootstrapping-freq 200 \
      < /tmp/data.ckks > /tmp/result.tfhe
    ```

    The monitor streams TFHE-encrypted verdicts (`/tmp/result.tfhe`) while logging diagnostics via spdlog.

7. **Decrypt the verdicts (client).** Only the TFHE secret key holder can reveal the boolean results:

    ```sh
    ./build/ahomfa_util tfhe dec \
      -K /tmp/tfhe.key \
      -i /tmp/result.tfhe
    ```

Steps 4–7 can be repeated for fresh traces without touching the private specification, while Steps 1–3 change only when predicates or STL formulas evolve. Tutorials (§4) provide additional execution modes (plain/pointwise/block) that plug into the same workflow.
