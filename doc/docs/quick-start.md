# 3. Quick Start

This walkthrough assumes the [Installation](installation.md) steps are complete and focuses on validating an end-to-end encrypted run with the blood glucose tutorial. For a one-command smoke test, you can also run `./examples/run_bg.sh` (see `--help` for options), which wraps every step below (build, keys, specs, sample data, encryption, monitoring, and decryption).

## 3.1 Before you begin

- Linux host with the toolchain and dependencies from Section 2.
- `examples/blood_glucose/config.json` untouched (it matches the tutorial formulas).
- Scratch space under `/tmp` (or adjust the paths below when running the commands).

## 3.2 Build the toolkit

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ahomfa_util libahomfa_runner.a
```

## 3.3 Compile the tutorial monitor

```sh
cmake --build build --target blood_glucose_one
```

The resulting `blood_glucose_one` binary exposes plain, block, pointwise, and reversed execution modes for experimentation.

## 3.4 Generate keys and encrypted data

```sh
./build/ahomfa_util ckks genkey -c examples/blood_glucose/config.json -o /tmp/ckks.key
./build/ahomfa_util tfhe genkey -o /tmp/tfhe.key
./build/ahomfa_util ckks genrelinkey -c examples/blood_glucose/config.json -K /tmp/ckks.key -o /tmp/ckks.relinkey
./build/ahomfa_util tfhe genbkey -K /tmp/tfhe.key -c examples/blood_glucose/config.json -S /tmp/ckks.key -o /tmp/tfhe.bkey
./build/ahomfa_util ckks enc -c examples/blood_glucose/config.json -K /tmp/ckks.key -o /tmp/data.ckks < examples/blood_glucose/input.txt
```

## 3.5 Produce forward and reversed specifications

```sh
./build/ahomfa_util ltl2spec -n 1 -e "$(cat examples/blood_glucose/bg1.ltl)" -o /tmp/bg1.spec
./build/ahomfa_util ltl2spec -n 1 -e "$(cat examples/blood_glucose/bg1.ltl)" --reverse -o /tmp/bg1.rev.spec
```

## 3.6 Run the monitor and decrypt verdicts

```sh
./examples/blood_glucose/blood_glucose_one reverse \
  --reversed \
  -c examples/blood_glucose/config.json \
  -f /tmp/bg1.rev.spec \
  -r /tmp/ckks.relinkey \
  -b /tmp/tfhe.bkey \
  < /tmp/data.ckks > /tmp/result.tfhe
./build/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe
```

```text
Dataflow overview
(specification + predicate) --> ahomfa_util ltl2spec/spec2spec --> DFA
Encrypted trace + keys ----> CKKS/TFHE monitor (blood_glucose_one) ----> TFHE verdict --> tfhe dec
```

The decrypted TFHE stream (`0`/`1`) reports whether the STL obligations hold at each timestep. If the output diverges from the documented tutorial values, revisit Installation (toolchain), Tutorials §4.2 (debugging failing traces), or Troubleshooting §9 before integrating custom predicates.
