# 8. Instrumentation and Integration

Custom predicates live in classes derived from `ArithHomFA::CKKSPredicate` (see `src/ckks_predicate.hh`) and are compiled together with `libahomfa_runner.a`. Keep predicate logic free of side effects; rely on spdlog for diagnostics because stdout/stderr are reserved for ciphertext streaming (printing with `std::cout` will corrupt the protocol). `libahomfa_runner.a` already provides the `main` used by the example binaries (e.g., `blood_glucose_one`), so you typically build standalone monitors that read encrypted traces from stdin/stdout rather than embedding the runner into another host.

To exercise the toolchain in CI, build the utilities and run a plain-mode monitor against a known trace:

```sh
cmake --build cmake-build-release --target ahomfa_util
./cmake-build-release/ahomfa_util ltl2spec -n "$NUM_VARS" -e "$LTL_FORMULA" -o spec.tmp
./your_monitor plain -c config.json -f spec.tmp < trace.csv
```

Use spdlog sinks or existing logging frameworks to capture monitor verdicts, trace IDs, and timing metadata for integration dashboards.

When subclassing `ArithHomFA::CKKSPredicate`, you typically need to:

- Override both `evalPredicateInternal` overloads: one that consumes CKKS ciphertext inputs (`std::vector<seal::Ciphertext>`) and one for plaintext doubles so you can run plain/unit tests.
- Compose SEAL primitives (encode thresholds, subtract/add/multiply ciphertexts, rescale or mod-switch as needed) and ensure the resulting ciphertext is moved to the target level before returning it to the runner.
- Provide static metadata (`signalSize`, `predicateSize`, and the `references` vector) so the runner knows how many CKKS slots to allocate and what error margins apply; `references` captures an approximate upper bound on the difference between every predicate value and its threshold so that downstream TFHE comparisons can size their intervals conservatively.
- Keep the implementation side-effect free except for spdlog logging; the runner streams ciphertexts on stdout/stderr.

## Example: blood_glucose_one predicate

The `examples/blood_glucose/blood_glucose_one.cc` monitor implements `ArithHomFA::CKKSPredicate` by subtracting the threshold from the encrypted glucose sample and returning the difference as a CKKS ciphertext:

```cpp
void CKKSPredicate::evalPredicateInternal(const std::vector<seal::Ciphertext> &valuation,
                                          std::vector<seal::Ciphertext> &result) {
    seal::Plaintext plain;
    encoder.encode(70, scale, plain);
    evaluator.sub_plain(valuation.front(), plain, result.front());
    evaluator.mod_switch_to_inplace(result.front(), context.last_parms_id());
}
```

A plaintext overload mirrors the same logic for non-encrypted testing. This example shows the minimal plumbing needed to implement predicate arithmetic and keep ciphertexts at the proper level before passing them back to the runner.
