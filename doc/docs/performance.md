# 9. Performance and Reproducibility

- **Complexity.** CKKS operations scale with polynomial modulus degree; TFHE bootstrapping dominates when block sizes are large. Reverse mode improves latency at the cost of memory.
- **Benchmarking.** Reproduce the RV'24 numbers by running the blood glucose example plus your benchmark traces under varying block sizes. Capture runtimes with `/usr/bin/time -v`.
- **Tuning.** Adjust CKKS parameters (scale, decomposition bit count) in `examples/config.json`, modify block sizes in the example runner, and toggle `--reversed` to study latency/throughput trade-offs.
- **Determinism.** Record the git commit, CKKS config, Spot/SEAL versions, and seed any randomized tests to guarantee reviewers can match your numbers.
