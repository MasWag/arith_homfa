#!/bin/bash
# Build Docker images for ArithHomFA

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"

# Build tags
UBUNTU_TAG="maswag/arith_homfa:noble"
DEBIAN_TAG="maswag/arith_homfa:trixie"

# Build Ubuntu 24.04 image
echo "Building $UBUNTU_TAG..."
docker build \
  -f "$SCRIPT_DIR/ubuntu24.04/Dockerfile" \
  -t "$UBUNTU_TAG" \
  "$REPO_ROOT"

echo ""
echo "Building $DEBIAN_TAG..."
docker build \
  -f "$SCRIPT_DIR/debian-trixie/Dockerfile" \
  -t "$DEBIAN_TAG" \
  "$REPO_ROOT"

echo ""
echo "Done! Images built:"
echo "  - $UBUNTU_TAG"
echo "  - $DEBIAN_TAG"
