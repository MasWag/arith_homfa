# 9. Troubleshooting and FAQ

- **`spot` missing during `ltl2spec`.** The converter links against Spot via `pkg-config`. Install the developer package (e.g., `sudo apt install libspot-dev spot`) or build Spot from source, then export `PKG_CONFIG_PATH=/path/to/spot/lib/pkgconfig:$PKG_CONFIG_PATH` so `pkg-config --libs spot` succeeds before reconfiguring CMake. Rerun `cmake -S . -B build ...` afterwards so the dependency cache picks up the new path.
- **Monitor prints nothing.** Encrypted modes stream ciphertext to stdout; redirect to a file or pipe into `tfhe dec`. Plain mode emits human-readable verdicts.
- **Verdict differs from expectation.** Double-check that client/server share the same CKKS config JSON, that the predicate’s tolerated error margins match the formula, and that you are feeding the correct spec variant (forward vs reversed).
- **How to enable logging?** spdlog is compiled in by default. Set `SPDLOG_LEVEL=debug ./examples/...` to expose predicate-level diagnostics; use `info` or `warn` for quieter runs.
- **Where to put keys?** Store CKKS/TFHE secrets under a tmpfs or encrypted directory (e.g., `/dev/shm/arith_homfa`) and delete them after the session to avoid accidental disclosure. Relinearization/bootstrapping keys can be shared with the untrusted monitor process.
