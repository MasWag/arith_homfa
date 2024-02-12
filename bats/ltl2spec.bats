#!/usr/bin/env bats

setup() {
    PROJECT_ROOT="${BATS_TEST_DIRNAME}/.."
    BUILD_DIR="${PROJECT_ROOT}/cmake-build-debug"
    BG_DIR="${PROJECT_ROOT}/examples/blood_glucose"
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"
    cmake --build "$BUILD_DIR" -- ahomfa_util
    PATH="$PATH:$BUILD_DIR"
}

teardown() {
    # Remove the temporally files
    rm -f "$BATS_TMPDIR/block.spec"
}

@test "Make spec for Block" {
    run ahomfa_util ltl2spec -e "$(cat "${BG_DIR}/gp0.ltl")" -o "$BATS_TMPDIR/block.spec" --num-vars 1
    [ "$status" -eq 0 ]
    [ -f "$BATS_TMPDIR/block.spec" ]
    diff "${BG_DIR}/gp0.spec" "$BATS_TMPDIR/block.spec"
}
