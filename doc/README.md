# ArithHomFA Documentation Guide

This directory contains the MkDocs project for the ArithHomFA manual. The document is rooted at `/doc`, so all commands below assume you start in the repository root.

The documentation tree is rooted at `/doc`, and GitHub Actions automatically deploys the rendered MkDocs site to [https://maswag.github.io/arith_homfa](https://maswag.github.io/arith_homfa) plus the Doxygen developer manual to [https://maswag.github.io/arith_homfa/developer_manual](https://maswag.github.io/arith_homfa/developer_manual).

## Layout

- `doc/docs/` — Markdown content consumed by MkDocs (see `mkdocs.yml` for the nav). Pages here must be more complete than the root `README.md`; treat the README as a teaser and this directory as the canonical manual.
- `doc/docs/*.md` naming mirrors section numbers (`index.md` for Overview, `installation.md`, `quick-start.md`, etc.).
- `doc/mkdocs.yml` — MkDocs configuration, theme, navigation, and site metadata.
- `doc/Doxyfile.in` — Template pulled by CMake to generate the developer manual.
- `doc/site/` — MkDocs build output (ignored by git).

Keep conceptual additions in `doc/docs/` so they automatically ship to the hosted site; only place contributor-centric notes (like this file) alongside the tooling.

## MkDocs site workflow

### Prerequisites

- Python 3.9+ available as `/usr/bin/python3`
- `mkdocs` installed inside a virtual environment (recommended)

### Build steps

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

The generated static site lives in `doc/site/` (git-ignored). To preview locally with live reload:

```sh
cd doc
mkdocs serve
```

This starts a local server (default `http://127.0.0.1:8000`) with live reload. Press `Ctrl+C` to stop it.

## Developer manual (Doxygen)

API-focused developer documentation is generated via Doxygen and integrated with CMake:

```sh
cmake -S . -B build
cmake --build build --target developer_manual
```

HTML output is placed at `build/doc/developer_manual/html/index.html`. The target is available only when `doxygen` is on your `PATH`, and the published version is refreshed by GitHub Actions alongside the MkDocs site.
