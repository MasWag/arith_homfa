# 9. Troubleshooting and FAQ

- **`spot` not found during `ltl2spec`.** Install Spot libraries and ensure `pkg-config` sees `spot.pc`.
- **Monitor prints nothing.** Remember that encrypted modes write ciphertext to stdout; pipe into `tfhe dec` or redirect to a file.
- **Verdict differs from expectation.** Verify that client/server use identical CKKS configs and that the predicate implementation’s threshold tolerances are correct.
- **How to enable logging?** Compile with spdlog (default) and set `SPDLOG_LEVEL=debug`.
- **Where to put keys?** Use secure tmpfs paths; delete keys after experiments to avoid leakage.
