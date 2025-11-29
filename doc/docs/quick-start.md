# 2. Quick Start

The quickest way to see ArithHomFA work is to build `ahomfa_util`, compile the blood glucose tutorial monitor, and run the end-to-end encrypted workflow.

1. **Initialize submodules and build the toolkit.**
   ```
   git submodule update --init --recursive
   cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
   cmake --build cmake-build-release --target ahomfa_util libahomfa_runner.a
   ```
2. **Build the tutorial monitor.**
   ```
   make -C examples/blood_glucose release
   ```
3. **Generate keys and encoded data (client side).**
   ```
   ./cmake-build-release/ahomfa_util ckks genkey -c examples/config.json -o /tmp/ckks.key
   ./cmake-build-release/ahomfa_util tfhe genkey -o /tmp/tfhe.key
   ./cmake-build-release/ahomfa_util ckks genrelinkey -c examples/config.json -K /tmp/ckks.key -o /tmp/ckks.relinkey
   ./cmake-build-release/ahomfa_util tfhe genbkey -K /tmp/tfhe.key -c examples/config.json -S /tmp/ckks.key -o /tmp/tfhe.bkey
   ./cmake-build-release/ahomfa_util ckks enc -c examples/config.json -K /tmp/ckks.key -o /tmp/data.ckks < examples/blood_glucose/input.txt
   ```
4. **Produce forward and reversed specifications (server side).**
   ```
   ./cmake-build-release/ahomfa_util ltl2spec -n 1 -e "$(cat examples/blood_glucose/gp0.ltl)" -o /tmp/gp0.spec
   ./cmake-build-release/ahomfa_util spec2spec --reverse -i /tmp/gp0.spec -o /tmp/gp0.rev.spec
   ```
5. **Run the monitor and decrypt results.**
   ```
   ./examples/blood_glucose/blood_glucose_one reverse \
     --reversed \
     -c examples/config.json \
     -f /tmp/gp0.rev.spec \
     -r /tmp/ckks.relinkey \
     -b /tmp/tfhe.bkey \
     < /tmp/data.ckks > /tmp/result.tfhe
   ./cmake-build-release/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe
   ```

```text
Dataflow overview
(specification + predicate) --> ahomfa_util ltl2spec/spec2spec --> DFA
Encrypted trace + keys ----> CKKS/TFHE monitor (blood_glucose_one) ----> TFHE verdict --> tfhe dec
```

Expected output is a decrypted verdict stream (`0`/`1` values) reporting whether the STL obligations hold at every time step. Any deviation from the tutorial values indicates a setup problem.
