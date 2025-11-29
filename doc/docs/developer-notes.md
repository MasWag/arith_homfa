# 12. Developer Notes

- **Repository layout.** `src/` holds library code, `examples/` contains runnable monitors, `doc/` holds legacy markdown, `docs/` this manual, `scripts/` automation helpers, and `thirdparty/` vendored dependencies.
- **Build/test commands.** Use the CMake flow described earlier; run `ctest` or `make test` inside the build tree. For example-specific builds, call `make` inside the example directory.
- **Adding operators.** Extend the STL handling inside `src/` and regenerate automata; keep parser/runner changes in sync.
- **Coding guidelines.** Follow existing clang-format style, rely on spdlog, and keep ciphertext copies minimal to avoid expensive relinearizations.
