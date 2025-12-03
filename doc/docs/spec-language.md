# 6. Specification Language Reference

1. **Grammar overview.** Formulas follow Spot’s LTL grammar with STL-inspired bounded temporal operators. Predicates are named `p<i>` and refer to indexed CKKS predicate slots.
2. **Operators.** Standard LTL (`X`, `G`, `F`, `U`, `R`) plus bounded variants using `[a,b]` intervals. Boolean connectives (`!`, `&`, `|`, `->`) are available.
3. **Types.** Predicates evaluate to boolean verdicts computed via CKKS arithmetic inside `ArithHomFA::CKKSPredicate`. Numeric thresholds live in C++ code, not in the formula.
4. **Idioms.** Prefer explicit bounds to keep DFA sizes manageable. Use derived formulas to capture sliding-window constraints and reuse them through macros in your C++ predicates.
5. **Pitfalls.** Ensure `--num-vars` matches the highest predicate index + 1; otherwise `ltl2spec` will misinterpret formulas. Avoid mixing continuous-time semantics; ArithHomFA assumes discrete ticks.
6. **Compatibility.** Spot handles parsing, so any syntax accepted by Spot should work. For fragments unsupported by the monitor (e.g., counting modalities), fall back to equivalent automata encodings.

## CLI cheat sheet

Translate a formula into the DFA format consumed by `libahomfa_runner` with `ltl2spec`:

```sh
./build/ahomfa_util ltl2spec \
  -e 'G[100,250](p0 -> F[0,5] p1)' \
  -n 2 \
  -o /tmp/my.spec
```

Key options:

- `-e/--formula` accepts any Spot-compatible LTL string that references predicates `p0`, `p1`, … matching your CKKS predicate indices.
- `-n/--num-vars` must be one plus the highest predicate index present in `--formula`.
- `--make-all-live-states-final` keeps automaton states that Spot would normally prune. Use it when you need DFA states to remain accepting for reversed monitoring workflows.

Post-process DFA files with `spec2spec` instead of additional `ltl2spec` flags:

```sh
./build/ahomfa_util spec2spec --reverse -i /tmp/my.spec -o /tmp/my.rev.spec
./build/ahomfa_util spec2spec --negate  -i /tmp/my.spec -o /tmp/my.neg.spec
./build/ahomfa_util spec2spec --minimize -i /tmp/my.spec -o /tmp/my.min.spec
```

These commands are covered by the regression tests in `bats/ltl2spec.bats`, which recompiles `ahomfa_util`, invokes `ltl2spec`, and compares the output against the canonical blood-glucose specs. Run `bats bats/ltl2spec.bats` after modifying the CLI to ensure the examples (and this documentation) stay in sync.
