#!/bin/sh
############################################################
# Name
#  build_and_test.sh - Build and test ArithHomFA for a specific SPOT version
#
# Usage
#  ./build_and_test.sh <SPOT_VERSION>
############################################################

set -ue

cd "$(dirname "$0")/.." || exit 1

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <SPOT_VERSION>"
    exit 1
fi

docker build \
    -f docker/ubuntu24.04/Dockerfile \
    --build-arg SPOT_VERSION="$1" \
    -t maswag/arith_homfa:"noble-spot$1" \
    .

docker run --rm -it maswag/arith_homfa:"noble-spot$1" run_vrss.sh
