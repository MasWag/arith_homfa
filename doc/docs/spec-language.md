# 6. Specification Language Reference

1. **Grammar overview.** Formulas follow Spot’s LTL grammar with STL-inspired bounded temporal operators. Predicates are named `p<i>` and refer to indexed CKKS predicate slots.
2. **Operators.** Standard LTL (`X`, `G`, `F`, `U`, `R`) plus bounded variants using `[a,b]` intervals. Boolean connectives (`!`, `&`, `|`, `->`) are available.
3. **Types.** Predicates evaluate to boolean verdicts computed via CKKS arithmetic inside `ArithHomFA::CKKSPredicate`. Numeric thresholds live in C++ code, not in the formula.
4. **Idioms.** Prefer explicit bounds to keep DFA sizes manageable. Use derived formulas to capture sliding-window constraints and reuse them through macros in your C++ predicates.
5. **Pitfalls.** Ensure `--num-vars` matches the highest predicate index + 1; otherwise `ltl2spec` will misinterpret formulas. Avoid mixing continuous-time semantics; ArithHomFA assumes discrete ticks.
6. **Compatibility.** Spot handles parsing, so any syntax accepted by Spot should work. For fragments unsupported by the monitor (e.g., counting modalities), fall back to equivalent automata encodings.
