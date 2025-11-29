# 11. Developer Notes

- **Repository layout.** `src/` holds library code, `examples/` contains runnable monitors, `doc/` hosts the MkDocs sources plus the Doxygen config, `scripts/` provides automation helpers, and `thirdparty/` stores vendored dependencies.
- **Build/test commands.** Use the CMake flow described earlier; run `make unit_test` inside the build tree. For example-specific builds, call `make` inside the example directory.
- **Coding guidelines.** Follow existing clang-format style, rely on spdlog, and keep ciphertext copies minimal to avoid expensive relinearizations.
