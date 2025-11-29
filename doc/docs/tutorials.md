# 5. Tutorials

Each tutorial lives under `examples/` and is runnable with the commands below. Replace `/tmp/...` paths if you prefer a different scratch location.

## 5.1 Basic monitoring (plain mode)

```sh
make -C examples/blood_glucose release
./cmake-build-release/ahomfa_util ltl2spec -n 1 -e "$(cat examples/blood_glucose/gp0.ltl)" -o /tmp/gp0.spec
./examples/blood_glucose/blood_glucose_one plain \
  -c examples/config.json \
  -f /tmp/gp0.spec \
  < examples/blood_glucose/input.txt
```
Inspect stdout for per-step verdicts and logging around predicate evaluations.

## 5.2 Debugging a failing trace

1. Modify `examples/blood_glucose/input.txt` to violate the STL constraint.
2. Re-run the plain tutorial; the monitor will log first-failure timestamps.
3. Use `--verbose` and the `pointwise` mode to inspect predicate-level outputs:
   ```sh
   ./examples/blood_glucose/blood_glucose_one pointwise -c examples/config.json --verbose < examples/blood_glucose/input.txt
   ```

## 5.3 Scaling up to block mode

```sh
./cmake-build-release/ahomfa_util ckks enc -c examples/config.json -K /tmp/ckks.key -i examples/blood_glucose/input.txt \
  | ./examples/blood_glucose/blood_glucose_one block \
      -c examples/config.json \
      -f /tmp/gp0.spec \
      -r /tmp/ckks.relinkey \
      -b /tmp/tfhe.bkey \
  > /tmp/result.tfhe
./cmake-build-release/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe
```
Tune block sizes inside the example to stress-test throughput and memory use.

## 5.4 Online / reversed monitoring

Run the `reverse` subcommand with the reversed specification:

```sh
./examples/blood_glucose/blood_glucose_one reverse --reversed \
  -c examples/config.json \
  -f /tmp/gp0.rev.spec \
  -r /tmp/ckks.relinkey \
  -b /tmp/tfhe.bkey \
  < /tmp/data.ckks > /tmp/result.tfhe
```
This mode emits verdicts as soon as sufficient future context becomes available, mimicking online monitoring on encrypted telemetry.

## 5.5 Domain case study (todo)

Add your domain-specific traces (e.g., CPS, security logs) under `examples/`. Reuse the Makefile template and tutorial scaffolding to document reproducible steps for reviewers.
