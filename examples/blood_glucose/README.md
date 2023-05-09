Blood Glucose Monitoring Example
================================

This example demonstrates the usage of Arithmetic HomFA (ArithHomFA) for blood glucose monitoring. The directory contains the following files:


File Descriptions
------------------

- `Makefile`: A makefile for building the blood glucose monitoring example.
- `blood_glucose_one.cc`: The C++ source code file containing the implementation of the `CKKSPredicate` class for the blood glucose monitoring example.
- `gp0.ltl`: The LTL formula file representing the STL formula for blood glucose monitoring.
- `input.txt`: A sample input file containing encrypted blood glucose data.
- `output.txt`: A sample output file containing the decrypted monitoring result.

Usage (Classical monitoring without FHE)
----------------------------------------

1. Build the blood glucose monitoring example by running the following command:

   ```
   make release
   ```

2. Convert the LTL formula to a DFA

    ```
    ../../cmake-build-release/ahomfa_util ltl2spec -n 1 -e $(cat gp0.ltl) -o gp0.spec
    
    ```

3. Conduct monitoring using the following command:

   ```
   ./blood_glucose_one plain -c ../config.json -f ./gp0.spec < ./input.txt
   ```

Usage (Pointwise evaluation)
----------------------------

1. Build the blood glucose monitoring example by running the following command:

   ```
   make release
   ```

2. Evaluate the predicate at each timepoint by the following command:

   ```
   ../../cmake-build-release/ahomfa_util ckks enc -c ../config.json -K ../ckks.key -i input.txt | ./blood_glucose_one pointwise -c ../config.json | ../../cmake-build-release/ahomfa_util ckks dec -c ../config.json -K ../ckks.key
   ```


Usage (oblivious monitoring with Block)
---------------------------------------

1. Build the blood glucose monitoring example by running the following command:

   ```
   make release
   ```

2. Convert the LTL formula to a DFA

    ```
    ../../cmake-build-release/ahomfa_util ltl2spec -n 1 -e $(cat gp0.ltl) -o gp0.spec
    
    ```

3. Conduct monitoring using the following command:

   ```
   ../../cmake-build-release/ahomfa_util ckks enc -c ../config.json -K ../ckks.key -i input.txt | ./blood_glucose_one block -c ../config.json -f ./gp0.spec -r ../ckks.relinkey -b ../tfhe.bkey > /tmp/result.tfhe
   ```

4. Decrypt the monitoring result with the following command:

   ```
   ./build/ahomfa_util tfhe dec -K /tmp/tfhe.key -i /tmp/result.tfhe
   ```
