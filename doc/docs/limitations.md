# 11. Limitations and Known Issues

- Only discrete-time STL is supported; no dense-time semantics.
- Predicates must be implemented in C++ with SEAL; there is no high-level DSL yet.
- Currently requires GCC; Clang support is blocked by template instantiation bugs.
- Large formulas can produce huge DFAs; consider minimization via `spec2spec --minimize`.
- No officially supported Docker image yet (community contributions welcome).
