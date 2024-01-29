#!/usr/bin/env bats

setup() {
    PROJECT_ROOT="${BATS_TEST_DIRNAME}/.."
    BUILD_DIR="${PROJECT_ROOT}/cmake-build-debug"
    EXAMPLE_DIR="${PROJECT_ROOT}/examples"
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"
    cmake --build "$BUILD_DIR" -- ahomfa_util
    PATH="$PATH:$BUILD_DIR"
}

teardown() {
    # Remove the temporally keys
    rm -f "$BATS_TMPDIR/ckks.key" "$BATS_TMPDIR/ckks.pkey" "$BATS_TMPDIR/tfhe.key" "$BATS_TMPDIR/ckks.relinkey" "$BATS_TMPDIR/tfhe.bkey"
}

@test "Generate CKKS private key" {
    run ahomfa_util ckks genkey -c "${EXAMPLE_DIR}/config.json" -o "$BATS_TMPDIR/ckks.key"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/ckks.key" ]
}

@test "Generate CKKS public key" {
    run ahomfa_util ckks genkey -c "${EXAMPLE_DIR}/config.json" -o "$BATS_TMPDIR/ckks.key"
    [ "$status" -eq 0 ]
    run ahomfa_util ckks genpkey -c "${EXAMPLE_DIR}/config.json" -K "$BATS_TMPDIR/ckks.key" -o "$BATS_TMPDIR/ckks.pkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/ckks.pkey" ]
}

@test "Generate CKKS relinkey" {
    run ahomfa_util ckks genkey -c "${EXAMPLE_DIR}/config.json" -o "$BATS_TMPDIR/ckks.key"
    [ "$status" -eq 0 ]
    run ahomfa_util ckks genrelinkey -c "${EXAMPLE_DIR}/config.json" -K "$BATS_TMPDIR/ckks.key" -o "$BATS_TMPDIR/ckks.relinkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/ckks.relinkey" ]
}

@test "Generate TFHE private key" {
    run ahomfa_util tfhe genkey -o "$BATS_TMPDIR/tfhe.key"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/tfhe.key" ]
}

@test "Generate TFHE bootstrapping key" {
    run ahomfa_util tfhe genkey -o "$BATS_TMPDIR/tfhe.key"
    [ "$status" -eq 0 ]
    run ahomfa_util ckks genkey -c "${EXAMPLE_DIR}/config.json" -o "$BATS_TMPDIR/ckks.key"
    [ "$status" -eq 0 ]
    run ahomfa_util tfhe genbkey -c "${EXAMPLE_DIR}/config.json" -K "$BATS_TMPDIR/tfhe.key" -S "$BATS_TMPDIR/ckks.key" -o "$BATS_TMPDIR/tfhe.bkey"
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/tfhe.bkey" ]
}
