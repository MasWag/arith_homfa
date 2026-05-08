#!/bin/bash
# Build Docker images for ArithHomFA

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

# Build tags
UBUNTU_TAG="maswag/arith_homfa:noble"
DEBIAN_TAG="maswag/arith_homfa:trixie"
SPOT_VERSION="${SPOT_VERSION:-2.15.1}"
SEAL_VERSION="${SEAL_VERSION:-4.3.2}"

BUILD_ARGS=(
  --build-arg "SPOT_VERSION=${SPOT_VERSION}"
  --build-arg "SEAL_VERSION=${SEAL_VERSION}"
)

# Build Ubuntu 24.04 image
echo "Building $UBUNTU_TAG with SEAL ${SEAL_VERSION} and Spot ${SPOT_VERSION}..."
docker build \
  -f "$SCRIPT_DIR/ubuntu24.04/Dockerfile" \
  "${BUILD_ARGS[@]}" \
  -t "$UBUNTU_TAG" \
  "$REPO_ROOT"

echo ""
echo "Building $DEBIAN_TAG with SEAL ${SEAL_VERSION} and Spot ${SPOT_VERSION}..."
docker build \
  -f "$SCRIPT_DIR/debian-trixie/Dockerfile" \
  "${BUILD_ARGS[@]}" \
  -t "$DEBIAN_TAG" \
  "$REPO_ROOT"

echo ""
echo "Done! Images built:"
echo "  - $UBUNTU_TAG"
echo "  - $DEBIAN_TAG"
