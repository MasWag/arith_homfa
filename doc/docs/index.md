# 1. Overview

**Citation.** See `CITATION.cff` for the RV'24 reference (_Oblivious Monitoring for Discrete-Time STL via Fully Homomorphic Encryption_, DOI: [10.1007/978-3-031-74234-7_4](https://doi.org/10.1007/978-3-031-74234-7_4)). Cite the paper together with this manual.

**License.** Code: GNU GPLv3 (`LICENSE`); examples/data share the same license unless noted.

**Repository.** https://github.com/MasWag/arith_homfa

**Archived snapshot.** Provide a Zenodo DOI when available.

**How to cite.** Cite the RV'24 paper listed in `CITATION.cff` (DOI: [10.1007/978-3-031-74234-7_4](https://doi.org/10.1007/978-3-031-74234-7_4)) together with this manual when referencing the artifact.

**Artifact contents.** This repository ships the `ahomfa_util` command-line utility, the `libahomfa_runner.a` monitoring library, sample STL formulas, build scripts, and the blood glucose tutorial dataset.

## 1.1 Purpose and use cases

Arithmetic HomFA (ArithHomFA) enables **oblivious online Signal Temporal Logic (STL)** monitoring by composing CKKS-based homomorphic arithmetic with TFHE-based boolean logic, so sensitive traces never leave the encrypted domain while temporal logic verdicts are produced in (near) real time. Typical use cases include:

- Privacy-preserving monitoring teams that need to outsource trace checking without leaking raw logs.
- Researchers comparing encrypted STL monitors on realistic workloads.
- Engineers integrating encrypted monitors into cyber-physical systems where encrypted telemetry must stay confidential.

These workflows cover offline trace verification, streaming monitors driven by encrypted sensor feeds, and incremental checking to triage regressions.

## 1.2 Supported specification language

ArithHomFA accepts discrete-time STL, expressed as Spot-compatible LTL formulas over arithmetic predicates (`p0`, `p1`, …). Predicates are implemented as C++ classes that evaluate encrypted CKKS slots. Specifications are compiled to DFA representations so that both forward and reversed monitoring are supported.

## 1.3 Key features

- **Dual FHE backend.** CKKS handles high-throughput linear arithmetic; TFHE supplies fast bootstrap-enabled boolean logic.
- **Reusable monitor runner.** `libahomfa_runner.a` links into custom monitors written in C++.
- **Specification tooling.** `ahomfa_util` handles LTL→DFA (`ltl2spec`) and DFA transforms (`spec2spec`).
- **Streaming-friendly modes.** Supports plain, pointwise, block-based, and reversed execution strategies.

## 1.4 Differentiators

Unlike plain TFHE-based monitors, ArithHomFA keeps arithmetic in CKKS to reduce bootstrapping pressure. Compared with prior HomFA work, ArithHomFA adds arithmetic predicates, exposes a reusable runner library, and focuses on online (not only batch) evaluation with encryption kept end-to-end.
