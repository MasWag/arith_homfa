# 4. Tutorials

Each tutorial lives under `examples/` and is runnable with the commands below. Replace `/tmp/...` paths if you prefer a different scratch location. For a fully automated walkthrough, use `./examples/run_bg.sh` (blood glucose) or `./examples/run_vrss.sh` (Vehicle RSS), which wrap all of the commands shown here.

## 4.1 Basic monitoring (plain mode)

```sh
cmake -S examples -B examples/build -DCMAKE_BUILD_TYPE=Release
cmake --build examples/build --target ahomfa_util blood_glucose_one
./examples/build/arith_homfa/ahomfa_util ltl2spec -n 1 -e "$(cat examples/blood_glucose/bg1.ltl)" -o /tmp/bg1.spec
./examples/build/blood_glucose/blood_glucose_one plain \
  -c examples/blood_glucose/config.json \
  -f /tmp/bg1.spec \
  < examples/blood_glucose/input.txt
```
Inspect stdout for per-step verdicts and logging around predicate evaluations.

## 4.2 Debugging a failing trace

1. Modify `examples/blood_glucose/input.txt` to violate the STL constraint.
2. Re-run the plain tutorial; the monitor will log first-failure timestamps.
3. Use `--verbose` and the `pointwise` mode to inspect predicate-level outputs:
   ```sh
   ./examples/build/blood_glucose/blood_glucose_one pointwise -c examples/blood_glucose/config.json --verbose < examples/blood_glucose/input.txt
   ```

## 4.3 Block mode

Run the `block` subcommand for an FHE-based online monitoring:

```sh
./examples/build/arith_homfa/ahomfa_util ckks enc -c examples/blood_glucose/config.json -K /tmp/ckks.key -i examples/blood_glucose/input.txt \
  | ./examples/build/blood_glucose/blood_glucose_one block \
      -c examples/blood_glucose/config.json \
      -f /tmp/bg1.spec \
      -r /tmp/ckks.relinkey \
      -b /tmp/tfhe.bkey \
      --block-size 1 \
  | ./examples/build/arith_homfa/ahomfa_util tfhe dec -K /tmp/tfhe.key
```

## 4.4 Reversed mode

Run the `reverse` subcommand, which is another mode for an FHE-based online monitoring:

!!! Note
    The construction of the reversed DFA needs huge time and memory for some specifications, including `bg1`.

```sh
cmake --build examples/build --target ahomfa_util blood_glucose_seven
./examples/build/arith_homfa/ahomfa_util ltl2spec -n 2 -e "$(cat examples/blood_glucose/bg7.ltl)" -o /tmp/bg7.spec
./examples/build/arith_homfa/ahomfa_util spec2spec -i /tmp/bg7.spec --reverse -o /tmp/bg7.rev.spec --minimize
./examples/build/arith_homfa/ahomfa_util ckks enc -c examples/blood_glucose/config.json -K /tmp/ckks.key -i examples/blood_glucose/input.txt \
  | ./examples/build/blood_glucose/blood_glucose_seven reverse --reversed \
      -c examples/blood_glucose/config.json \
      -f /tmp/bg7.rev.spec \
      -r /tmp/ckks.relinkey \
      -b /tmp/tfhe.bkey \
      --bootstrapping-freq 200 \
  | ./examples/build/arith_homfa/ahomfa_util tfhe dec -K /tmp/tfhe.key
```

## 4.5 Vehicle RSS case study

```sh
cmake --build examples/build --target vehicle_rss
./examples/run_vrss.sh  # or run make keys/specs/sample_data/encrypt_sample manually
```

The RSS example monitors eight simultaneous predicates derived from `vrss.ltl`, which is generated from `vrss.xltl` using [`ltlconv`](https://github.com/gfngfn/ltlconv/). Inspect `examples/vehicle_rss/README.md` for the predicate definitions and data format, then adapt the Make targets when bringing your own traces.
