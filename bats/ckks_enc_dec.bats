#!/usr/bin/env bats

setup() {
    PROJECT_ROOT="${BATS_TEST_DIRNAME}/.."
    BUILD_DIR="${PROJECT_ROOT}/cmake-build-debug"
    EXAMPLE_DIR="${PROJECT_ROOT}/examples"
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    cmake --build "$BUILD_DIR" -- ahomfa_util
    PATH="$PATH:$BUILD_DIR"

    # Generate a random input
    cat /dev/urandom | od | xargs -n 1 | sed 's/^0*//;/^$/d;' | head -n 10 | awk '$0*=0.01' > "$BATS_TMPDIR/random.input"

    # Generate the secret key
    run ahomfa_util ckks genkey -c "${EXAMPLE_DIR}/config.json" -o "$BATS_TMPDIR/ckks.key"
}

teardown() {
    # Remove the temporally keys
    rm -f "$BATS_TMPDIR/ckks.key" "$BATS_TMPDIR/ckks.pkey" "$BATS_TMPDIR/random.input" "$BATS_TMPDIR/random.ckks" "$BATS_TMPDIR/random.result"
}

@test "Encrypt and decrypt using CKKS private key" {
    run ahomfa_util ckks enc -c "${EXAMPLE_DIR}/config.json" -K "$BATS_TMPDIR/ckks.key" -i "$BATS_TMPDIR/random.input" -o "$BATS_TMPDIR/random.ckks"
    [ "$status" -eq 0 ]
    run ahomfa_util ckks dec -c "${EXAMPLE_DIR}/config.json" -K "$BATS_TMPDIR/ckks.key" -i "$BATS_TMPDIR/random.ckks" -o "$BATS_TMPDIR/random.result"
    [ "$status" -eq 0 ]
    diff "$BATS_TMPDIR/random.input" "$BATS_TMPDIR/random.result"
}

@test "Encrypt with a public key and decrypt with the private key" {
    run ahomfa_util ckks genpkey -c "${EXAMPLE_DIR}/config.json" -K "$BATS_TMPDIR/ckks.key" -o "$BATS_TMPDIR/ckks.pkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/ckks.pkey" ]
    run ahomfa_util ckks enc -c "${EXAMPLE_DIR}/config.json" -k "$BATS_TMPDIR/ckks.pkey" -i "$BATS_TMPDIR/random.input" -o "$BATS_TMPDIR/random.ckks"
    [ "$status" -eq 0 ]
    run ahomfa_util ckks dec -c "${EXAMPLE_DIR}/config.json" -K "$BATS_TMPDIR/ckks.key" -i "$BATS_TMPDIR/random.ckks" -o "$BATS_TMPDIR/random.result"
    [ "$status" -eq 0 ]
    diff "$BATS_TMPDIR/random.input" "$BATS_TMPDIR/random.result"
}
