# Appendices

## Appendix A — Runnable examples catalog

| Directory | Highlights |
| --- | --- |
| `examples/blood_glucose` | Full plain/block/reverse tutorials, predicates for glucose monitoring, Makefile template. |
| `examples/vehicle_rss` | RSS-based automotive safety predicates, data synthesizers, and monitoring scripts. |
| `examples/` (add yours) | Follow the README template, include traces, specs, and expected verdict logs. |

## Appendix B — Paper cross-reference

| Paper section | Manual location |
| --- | --- |
| RV'24 system overview | §1 Overview + §4 Core Concepts |
| Algorithm details | §7 Specification Language + `src/` comments |
| Evaluation | §5 Tutorials + §9 Performance |

Populate this table once the camera-ready paper numbers are public.

## Appendix C — Artifact checklist

- Tool version + git commit recorded in Front Matter.
- Build scripts (`CMakeLists.txt`, `examples/*/Makefile`).
- Sample traces/specs/keys under `examples/`.
- Reproduction commands in §2 and §5.

## Appendix D — Third-party licenses

- Microsoft SEAL (MIT) — see upstream repo.
- Spot (GPLv3 compatible) — bundled via system install.
- TFHEpp (Apache-2.0).
- spdlog (MIT), Boost (BSL-1.0).

Ensure compatibility when redistributing binaries; GPLv3 of ArithHomFA applies to combined works.
