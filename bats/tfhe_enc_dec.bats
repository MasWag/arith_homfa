#!/usr/bin/env bats

setup() {
    PROJECT_ROOT="${BATS_TEST_DIRNAME}/.."
    BUILD_DIR="${PROJECT_ROOT}/cmake-build-debug"
    EXAMPLE_DIR="${PROJECT_ROOT}/examples"
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"
    cmake --build "$BUILD_DIR" -- ahomfa_util
    PATH="$PATH:$BUILD_DIR"

    # Generate a random {0, 1} input
    cat /dev/urandom | od -An | grep -o [0-9] | awk '{print $0 % 2}' | head -n 10 > "$BATS_TMPDIR/random.input"
    # Convert to {true, false}
    sed 's/1/true/;s/0/false/;' "$BATS_TMPDIR/random.input" > "$BATS_TMPDIR/expected.result"

    # Generate the secret key
    ahomfa_util tfhe genkey -o "$BATS_TMPDIR/tfhe.key"
}

teardown() {
    # Remove the temporally keys
    rm -f "$BATS_TMPDIR/tfhe.key" "$BATS_TMPDIR/random.input" "$BATS_TMPDIR/random.tfhe" "$BATS_TMPDIR/random.result" "$BATS_TMPDIR/expected.result"
}

@test "Encrypt and decrypt TFHE ciphertexts" {
    run ahomfa_util tfhe enc -K "$BATS_TMPDIR/tfhe.key" -i "$BATS_TMPDIR/random.input" -o "$BATS_TMPDIR/random.tfhe"
    [ "$status" -eq 0 ]
    run ahomfa_util tfhe dec -K "$BATS_TMPDIR/tfhe.key" -i "$BATS_TMPDIR/random.tfhe" -o "$BATS_TMPDIR/random.result"
    [ "$status" -eq 0 ]
    diff "$BATS_TMPDIR/expected.result" "$BATS_TMPDIR/random.result"
}
