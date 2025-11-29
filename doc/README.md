# ArithHomFA Documentation Guide

This directory contains the MkDocs project for the ArithHomFA manual. The document is rooted at `/doc`, so all commands below assume you start in the repository root. The latest published version is automatically deployed to [https://maswag.github.io/arith_homfa](https://maswag.github.io/arith_homfa) via GitHub Actions.

## Prerequisites

- Python 3.9+ available as `/usr/bin/python3`
- `mkdocs` installed inside a virtual environment (recommended)

## Build steps

```sh
# 1. Create and activate the virtual environment (first time only)
/usr/bin/python3 -m venv .venv
source .venv/bin/activate

# 2. Install MkDocs inside the venv
pip install mkdocs

# 3. Build the site from /doc
cd doc
mkdocs build
```

The generated static site is written to `doc/site/` (ignored by git). To preview locally:

```sh
cd doc
mkdocs serve
```

This starts a local server (default `http://127.0.0.1:8000`) with live reload. Press `Ctrl+C` to stop it.

## Developer manual (Doxygen)

API-focused developer documentation is generated via Doxygen and hooked into CMake:

```sh
cmake -S . -B build
cmake --build build --target developer_manual
```

The HTML output is written to `build/doc/developer_manual/html/index.html`. The target is created only when `doxygen` is available in your `PATH`. The same developer manual is also published at [https://maswag.github.io/arith_homfa/developer_manual/](https://maswag.github.io/arith_homfa/developer_manual/) whenever the GitHub Actions workflow runs.
