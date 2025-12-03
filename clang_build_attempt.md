Clang build attempt - 2025-02-14
================================

Environment
-----------
- Host compiler: `clang 21.1.6`
- Configure command: `CC=clang CXX=clang++ cmake -S . -B build-clang -DCMAKE_BUILD_TYPE=Release`
- Build command: `cmake --build build-clang --target ahomfa_util ahomfa_runner`

Outcome
-------
- Configuration succeeds, but the build fails while compiling `src/main.cc`.
- Clang reports `error: call to consteval function 'fmt::basic_format_string<...>' is not a constant expression` coming from `spdlog::info` in `src/main.cc:481` and `src/main.cc:491`.
- The failure happens inside spdlog’s `SPDLOG_LOGGER_CATCH` path when `FMT_STRING("{} [{}({})]")` is evaluated, so `ahomfa_util` never links and `libahomfa_runner.a` is not produced.
- Because the binaries are missing, the example scripts (`examples/run_bg.sh`, `examples/run_vrss.sh`) were not executed.

Notes
-----
- No documentation was updated, per the request to leave clang marked as unsupported when the build does not succeed.
- The `build-clang` directory retains the failed configuration for future debugging.
