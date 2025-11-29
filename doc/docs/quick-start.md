# 3. Quick Start

This walkthrough assumes the [Installation](installation.md) steps are complete and focuses on validating an end-to-end encrypted run with the blood glucose tutorial. For a one-command smoke test, you can also run `./examples/run_bg.sh` (see `--help` for options), which wraps every step below (build, keys, specs, sample data, encryption, monitoring, and decryption).

## 3.1 Before you begin

- Linux host with the toolchain and dependencies from Section 2.
- `examples/blood_glucose/config.json` untouched (it matches the tutorial formulas).
- Scratch space under `/tmp` (or adjust the paths below when running the commands).

## 3.2 Build the toolkit and the tutorial monitor

Configure the examples workspace (first run only), then build the monitor target:

```sh
git submodule update --init --recursive
cmake -S ./examples -B ./examples/build -DCMAKE_BUILD_TYPE=Release
cmake --build ./examples/build --target ahomfa_util blood_glucose_one
```

If you have, using `ninja` by specifying `-GNinja` in configuring `cmake` may make the compilation much faster. The resulting `ahomfa_util` binary is generated at `examples/build/arith_homfa/ahomfa_util` and `blood_glucose_one` binary is generated at `examples/build/blood_glucose/blood_glucose_one`. `blood_glucose_one` exposes plain, block, pointwise, and reversed execution modes for experimentation.

## 3.3 Generate keys and encrypted data

```sh
./examples/build/arith_homfa/ahomfa_util ckks genkey -c examples/blood_glucose/config.json -o /tmp/ckks.key
./examples/build/arith_homfa/ahomfa_util tfhe genkey -o /tmp/tfhe.key
./examples/build/arith_homfa/ahomfa_util ckks genrelinkey -c examples/blood_glucose/config.json -K /tmp/ckks.key -o /tmp/ckks.relinkey
./examples/build/arith_homfa/ahomfa_util tfhe genbkey -K /tmp/tfhe.key -c examples/blood_glucose/config.json -S /tmp/ckks.key -o /tmp/tfhe.bkey
./examples/build/arith_homfa/ahomfa_util ckks enc -c examples/blood_glucose/config.json -K /tmp/ckks.key -o /tmp/data.ckks < examples/blood_glucose/input.txt
```

!!! Note
    `tfhe genbkey` emits a multi-gigabyte bootstrapping key and takes some time depending on CPU cores. Run it once and keep the resulting `/tmp/tfhe.bkey` for future trials.

## 3.4 Produce DFA from an LTL formula

```sh
./examples/build/arith_homfa/ahomfa_util ltl2spec -n 1 -e "$(cat examples/blood_glucose/bg1.ltl)" -o /tmp/bg1.spec
```

## 3.5 Run the monitor and decrypt verdicts

```sh
./examples/build/blood_glucose/blood_glucose_one block \
  -c examples/blood_glucose/config.json \
  -f /tmp/bg1.spec \
  -r /tmp/ckks.relinkey \
  -b /tmp/tfhe.bkey \
  --block-size 1 \
  < /tmp/data.ckks |
  ./examples/build/arith_homfa/ahomfa_util tfhe dec -K /tmp/tfhe.key
```

The decrypted TFHE stream (`false`/`true`) reports whether the monitored STL formula hold at each timestep.
