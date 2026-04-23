# 10. Limitations and Known Issues

- Only discrete-time STL is supported; no dense-time semantics.
- Predicates must be implemented in C++ with SEAL; there is no high-level DSL yet.
- Currently requires GCC; Clang support is blocked by template instantiation bugs.
- Supported operating systems are limited to Ubuntu 24.04 LTS and Debian 13 (trixie). Ubuntu 22.04 and older releases are not supported.
- Large formulas can produce huge DFAs; consider minimization via `spec2spec --minimize`.
