#!/usr/bin/env bats

setup() {
    PROJECT_ROOT="${BATS_TEST_DIRNAME}/.."
    BUILD_DIR="${PROJECT_ROOT}/cmake-build-release"
    BG_DIR="${PROJECT_ROOT}/examples/blood_glucose"
    CONFIG="${BG_DIR}/../config.json"
    # Build ArithHomFA
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    cmake --build "$BUILD_DIR" -- ahomfa_util ahomfa_runner

    # Build the runners
    make clean -C "$BG_DIR"
    make release -C "$BG_DIR" -j4

    PATH="$PATH:$BUILD_DIR:$BG_DIR"

    # Options
    BOOTSTRAPPING_FREQ=200
    BLOCK_SIZE=1

    # Generate CKKS private key
    if ! [[ -f "$BATS_TMPDIR/ckks.key" ]]; then
        ahomfa_util ckks genkey -c "${CONFIG}" -o "$BATS_TMPDIR/ckks.key"
    fi
    # Generate CKKS relinkey
    if [[ ! -f "$BATS_TMPDIR/ckks.relinkey" ]] || [[ "$BATS_TMPDIR/ckks.key" -nt "$BATS_TMPDIR/ckks.relinkey" ]]; then
        ahomfa_util ckks genrelinkey -c "${CONFIG}" -K "$BATS_TMPDIR/ckks.key" -o "$BATS_TMPDIR/ckks.relinkey"
    fi
    # Generate TFHE private key
    if ! [[ -f "$BATS_TMPDIR/tfhe.key" ]]; then
        ahomfa_util tfhe genkey -o "$BATS_TMPDIR/tfhe.key"
    fi
    # Generate TFHE bootstrapping key
    if [[ ! -f "$BATS_TMPDIR/tfhe.bkey" ]] || [[ "$BATS_TMPDIR/tfhe.key" -nt "$BATS_TMPDIR/tfhe.bkey" ]]; then
        ahomfa_util tfhe genbkey -c "${CONFIG}" -K "$BATS_TMPDIR/tfhe.key" -S "$BATS_TMPDIR/ckks.key" -o "$BATS_TMPDIR/tfhe.bkey"
    fi

    # Encrypt the input with CKKS scheme
    ahomfa_util ckks enc -i "${BG_DIR}/input.txt" -o "$BATS_TMPDIR/input_data.ckks" -K "$BATS_TMPDIR/ckks.key" -c "$CONFIG"
}

teardown() {
    # Remove the temporally files
    rm -f "$BATS_TMPDIR/block.spec" "$BATS_TMPDIR/reverse.spec"
}

reverse() {
    if [[ "$#" -lt 3 ]]; then
        echo "Usage: $0 binary_name ltl_file num_vars"
        return 1
    fi

    # Generate .spec file
    ahomfa_util ltl2spec -e "$(cat "$2")" -o "$BATS_TMPDIR/block.spec" --num-vars "$3"

    # Generate the reversed.spec file
    ahomfa_util spec2spec -i "$BATS_TMPDIR/block.spec" -o "$BATS_TMPDIR/reverse.spec" --reverse

    run "$1" reverse -f "$BATS_TMPDIR/reverse.spec" -r "$BATS_TMPDIR/ckks.relinkey" -b "$BATS_TMPDIR/tfhe.bkey" -i "$BATS_TMPDIR/input_data.ckks" -o "$BATS_TMPDIR/result.reverse.tfhe" -c "$CONFIG" --bootstrapping-freq "$BOOTSTRAPPING_FREQ" --reversed

    return $?
}

block() {
    if [[ "$#" -lt 3 ]]; then
        echo "Usage: $0 binary_name ltl_file num_vars"
        return 1
    fi

    # Generate .spec file
    run ahomfa_util ltl2spec -e "$(cat "$2")" -o "$BATS_TMPDIR/block.spec" --num-vars "$3"

    run "$1" block -f "$BATS_TMPDIR/block.spec" -r "$BATS_TMPDIR/ckks.relinkey" -b "$BATS_TMPDIR/tfhe.bkey" -i "$BATS_TMPDIR/input_data.ckks" -o "$BATS_TMPDIR/result.block.tfhe" -c "$CONFIG"  --block-size "$BLOCK_SIZE"

    return $?
}

plain() {
    if [[ "$#" -lt 1 ]]; then
        echo "Usage: $0 binary_name"
        return 1
    fi

    run "$1" plain -f "$BATS_TMPDIR/block.spec" -i "$BG_DIR/input.txt" -o "$BATS_TMPDIR/result.plain.txt" -c "$CONFIG"

    return $?
}

dec() {
    if [[ "$#" -lt 2 ]]; then
        echo "Usage: $0 input output"
        return 1
    fi

    ahomfa_util tfhe dec -i "$1" -o "$2" -K "$BATS_TMPDIR/tfhe.key"
}

@test "Make spec for Block" {
    run ahomfa_util ltl2spec -e "$(cat "${BG_DIR}/gp0.ltl")" -o "$BATS_TMPDIR/block.spec" --num-vars 1
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/block.spec" ]
    diff "${BG_DIR}/gp0.spec" "$BATS_TMPDIR/block.spec"
}

#########################################################
########################## BG1 ##########################
#########################################################

@test "Compute pointwise difference for bg1" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    # Conduct pointwise computation
    run blood_glucose_one pointwise -i "$BATS_TMPDIR/input_data.ckks" -o "$BATS_TMPDIR/pointwise.ckks" -c "$CONFIG" -r "$BATS_TMPDIR/ckks.relinkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.ckks" ]
    run ahomfa_util ckks dec -i "$BATS_TMPDIR/pointwise.ckks" -o "$BATS_TMPDIR/pointwise.ckks.plain" -K "$BATS_TMPDIR/ckks.key" -c "${CONFIG}"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.ckks.plain" ]

    # Conduct pointwise + scheme switching
    run blood_glucose_one pointwise-tfhe -i "$BATS_TMPDIR/input_data.ckks" -o "$BATS_TMPDIR/pointwise.tfhe" -c "$CONFIG" -r "$BATS_TMPDIR/ckks.relinkey" -b "$BATS_TMPDIR/tfhe.bkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.tfhe" ]
    # At this stage, we need to split the torus at 0 and 1/2
    run ahomfa_util tfhe dec -i "$BATS_TMPDIR/pointwise.tfhe" -o "$BATS_TMPDIR/pointwise.tfhe.plain" -K "$BATS_TMPDIR/tfhe.key" --vertical
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.tfhe.plain" ]

    # Compare the plaintext
    awk '$0 < 0 {print "false"} $0 >= 0 {print "true"}' < "$BATS_TMPDIR/pointwise.ckks.plain" > "$BATS_TMPDIR/pointwise.ckks.plain.bool"
    diff "$BATS_TMPDIR/pointwise.ckks.plain.bool" "$BATS_TMPDIR/pointwise.tfhe.plain"
}

@test "Compute reverse difference for bg1" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    run reverse blood_glucose_one "${BG_DIR}/gp0.ltl" 1
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/result.reverse.tfhe" ]
    # Assume that the specs are generated
    [ -f "$BATS_TMPDIR/block.spec" ]
    [ -f "$BATS_TMPDIR/reverse.spec" ]
    dec "$BATS_TMPDIR/result.reverse.tfhe" "$BATS_TMPDIR/result.reverse.txt"
    [ -f "$BATS_TMPDIR/result.reverse.txt" ]

    plain blood_glucose_one && [ -f "$BATS_TMPDIR/result.plain.txt" ]

    diff "$BATS_TMPDIR/result.reverse.txt" "$BATS_TMPDIR/result.plain.txt"
}

@test "Compute block difference for bg1" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    run block blood_glucose_one "${BG_DIR}/gp0.ltl" 1
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/result.block.tfhe" ]
    # Assume that the spec is generated
    [ -f "$BATS_TMPDIR/block.spec" ]
    dec "$BATS_TMPDIR/result.block.tfhe" "$BATS_TMPDIR/result.block.txt"
    [ -f "$BATS_TMPDIR/result.block.txt" ]

    plain blood_glucose_one && [ -f "$BATS_TMPDIR/result.plain.txt" ]

    diff "$BATS_TMPDIR/result.block.txt" "$BATS_TMPDIR/result.plain.txt"
}

#########################################################
########################## BG7 ##########################
#########################################################

@test "Compute pointwise difference for bg7" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    # Conduct pointwise computation
    run blood_glucose_seven pointwise -i "$BATS_TMPDIR/input_data.ckks" -o "$BATS_TMPDIR/pointwise.ckks" -c "$CONFIG" -r "$BATS_TMPDIR/ckks.relinkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.ckks" ]
    run ahomfa_util ckks dec -i "$BATS_TMPDIR/pointwise.ckks" -o "$BATS_TMPDIR/pointwise.ckks.plain" -K "$BATS_TMPDIR/ckks.key" -c "${CONFIG}"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.ckks.plain" ]

    # Conduct pointwise + scheme switching
    run blood_glucose_seven pointwise-tfhe -i "$BATS_TMPDIR/input_data.ckks" -o "$BATS_TMPDIR/pointwise.tfhe" -c "$CONFIG" -r "$BATS_TMPDIR/ckks.relinkey" -b "$BATS_TMPDIR/tfhe.bkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.tfhe" ]
    # At this stage, we need to split the torus at 0 and 1/2
    run ahomfa_util tfhe dec -i "$BATS_TMPDIR/pointwise.tfhe" -o "$BATS_TMPDIR/pointwise.tfhe.plain" -K "$BATS_TMPDIR/tfhe.key" --vertical
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.tfhe.plain" ]

    # Compare the plaintext
    awk '$0 < 0 {print "false"} $0 >= 0 {print "true"}' < "$BATS_TMPDIR/pointwise.ckks.plain" > "$BATS_TMPDIR/pointwise.ckks.plain.bool"
    diff "$BATS_TMPDIR/pointwise.ckks.plain.bool" "$BATS_TMPDIR/pointwise.tfhe.plain"
}

@test "Compute reverse difference for bg7" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    run reverse blood_glucose_seven "${BG_DIR}/bg7.ltl" 2
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/result.reverse.tfhe" ]
    # Assume that the specs are generated
    [ -f "$BATS_TMPDIR/block.spec" ]
    [ -f "$BATS_TMPDIR/reverse.spec" ]
    dec "$BATS_TMPDIR/result.reverse.tfhe" "$BATS_TMPDIR/result.reverse.txt"
    [ -f "$BATS_TMPDIR/result.reverse.txt" ]

    plain blood_glucose_seven && [ -f "$BATS_TMPDIR/result.plain.txt" ]

    diff "$BATS_TMPDIR/result.reverse.txt" "$BATS_TMPDIR/result.plain.txt"
}

@test "Compute block difference for bg7" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    run block blood_glucose_seven "${BG_DIR}/bg7.ltl" 2
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/result.block.tfhe" ]
    # Assume that the spec is generated
    [ -f "$BATS_TMPDIR/block.spec" ]
    dec "$BATS_TMPDIR/result.block.tfhe" "$BATS_TMPDIR/result.block.txt"
    [ -f "$BATS_TMPDIR/result.block.txt" ]

    plain blood_glucose_seven && [ -f "$BATS_TMPDIR/result.plain.txt" ]

    diff "$BATS_TMPDIR/result.block.txt" "$BATS_TMPDIR/result.plain.txt"
}

#########################################################
########################## BG1' #########################
#########################################################

@test "Compute pointwise difference for bg1'" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    # Conduct pointwise computation
    run blood_glucose_one_dummy pointwise -i "$BATS_TMPDIR/input_data.ckks" -o "$BATS_TMPDIR/pointwise.ckks" -c "$CONFIG" -r "$BATS_TMPDIR/ckks.relinkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.ckks" ]
    run ahomfa_util ckks dec -i "$BATS_TMPDIR/pointwise.ckks" -o "$BATS_TMPDIR/pointwise.ckks.plain" -K "$BATS_TMPDIR/ckks.key" -c "${CONFIG}"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.ckks.plain" ]

    # Conduct pointwise + scheme switching
    run blood_glucose_one_dummy pointwise-tfhe -i "$BATS_TMPDIR/input_data.ckks" -o "$BATS_TMPDIR/pointwise.tfhe" -c "$CONFIG" -r "$BATS_TMPDIR/ckks.relinkey" -b "$BATS_TMPDIR/tfhe.bkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.tfhe" ]
    # At this stage, we need to split the torus at 0 and 1/2
    run ahomfa_util tfhe dec -i "$BATS_TMPDIR/pointwise.tfhe" -o "$BATS_TMPDIR/pointwise.tfhe.plain" -K "$BATS_TMPDIR/tfhe.key" --vertical
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/pointwise.tfhe.plain" ]

    # Compare the plaintext
    awk '$0 < 0 {print "false"} $0 >= 0 {print "true"}' < "$BATS_TMPDIR/pointwise.ckks.plain" > "$BATS_TMPDIR/pointwise.ckks.plain.bool"
    diff "$BATS_TMPDIR/pointwise.ckks.plain.bool" "$BATS_TMPDIR/pointwise.tfhe.plain"
}

@test "Compute reverse difference for bg1'" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    run reverse blood_glucose_one_dummy "${BG_DIR}/gp0.ltl" 1
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/result.reverse.tfhe" ]
    # Assume that the specs are generated
    [ -f "$BATS_TMPDIR/block.spec" ]
    [ -f "$BATS_TMPDIR/reverse.spec" ]
    dec "$BATS_TMPDIR/result.reverse.tfhe" "$BATS_TMPDIR/result.reverse.txt"
    [ -f "$BATS_TMPDIR/result.reverse.txt" ]

    plain blood_glucose_one_dummy && [ -f "$BATS_TMPDIR/result.plain.txt" ]

    diff "$BATS_TMPDIR/result.reverse.txt" "$BATS_TMPDIR/result.plain.txt"
}

@test "Compute block difference for bg1'" {
    # Assume that the input data is already encrypted
    [ -f "$BATS_TMPDIR/input_data.ckks" ]

    run block blood_glucose_one_dummy "${BG_DIR}/gp0.ltl" 1
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/result.block.tfhe" ]
    # Assume that the spec is generated
    [ -f "$BATS_TMPDIR/block.spec" ]
    dec "$BATS_TMPDIR/result.block.tfhe" "$BATS_TMPDIR/result.block.txt"
    [ -f "$BATS_TMPDIR/result.block.txt" ]

    plain blood_glucose_one_dummy && [ -f "$BATS_TMPDIR/result.plain.txt" ]

    diff "$BATS_TMPDIR/result.block.txt" "$BATS_TMPDIR/result.plain.txt"
}
