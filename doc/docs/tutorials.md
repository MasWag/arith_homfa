# 4. Tutorials

Each tutorial lives under `examples/` and is runnable with the commands below. Replace `/tmp/...` paths if you prefer a different scratch location. For a fully automated walkthrough, use `./examples/run_bg.sh` (blood glucose) or `./examples/run_vrss.sh` (Vehicle RSS), which wrap all of the commands shown here.

## 4.1 Basic monitoring (plain mode)

```sh
cmake --build build --target blood_glucose_one
./build/ahomfa_util ltl2spec -n 1 -e "$(cat examples/blood_glucose/bg1.ltl)" -o /tmp/bg1.spec
./examples/blood_glucose/blood_glucose_one plain \
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
   ./examples/blood_glucose/blood_glucose_one pointwise -c examples/blood_glucose/config.json --verbose < examples/blood_glucose/input.txt
   ```

## 4.3 Scaling up to block mode

```sh
./build/ahomfa_util ckks enc -c examples/blood_glucose/config.json -K /tmp/ckks.key -i examples/blood_glucose/input.txt \
  | ./examples/blood_glucose/blood_glucose_one block \
      -c examples/blood_glucose/config.json \
      -f /tmp/bg1.spec \
      -r /tmp/ckks.relinkey \
      -b /tmp/tfhe.bkey \
  > /tmp/result.tfhe
./build/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe
```
Tune block sizes inside the example to stress-test throughput and memory use.

## 4.4 Online / reversed monitoring

Run the `reverse` subcommand with the reversed specification:

```sh
./examples/blood_glucose/blood_glucose_one reverse --reversed \
  -c examples/blood_glucose/config.json \
  -f /tmp/bg1.rev.spec \
  -r /tmp/ckks.relinkey \
  -b /tmp/tfhe.bkey \
  < /tmp/data.ckks > /tmp/result.tfhe
```
This mode emits verdicts as soon as sufficient future context becomes available, mimicking online monitoring on encrypted telemetry.

## 4.5 Vehicle RSS case study

```sh
cmake --build build --target vehicle_rss
./examples/run_vrss.sh  # or run make keys/specs/sample_data/encrypt_sample manually
```

The RSS example monitors four simultaneous predicates derived from `vrss.ltl`. Inspect `examples/vehicle_rss/README.md` for the predicate definitions and data format, then adapt the Make targets when bringing your own traces.

## 4.6 Domain case study (todo)

Add your domain-specific traces (e.g., CPS, security logs) under `examples/`. Reuse the Makefile template and tutorial scaffolding to document reproducible steps for reviewers.
