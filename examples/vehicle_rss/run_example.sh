#!/bin/bash
set -e

# Vehicle RSS Example Execution Script
# This script demonstrates the complete workflow for homomorphic monitoring

echo "========================================="
echo "Vehicle RSS Homomorphic Monitoring Example"
echo "========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
EXAMPLE_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${EXAMPLE_DIR}/../../build"
AHOMFA_UTIL="${BUILD_DIR}/src/ahomfa_util"
MONITOR="${BUILD_DIR}/examples/vehicle_rss/vehicle_rss"

# Check if build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo -e "${YELLOW}Build directory not found. Creating and building...${NC}"
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j4
    cd "${EXAMPLE_DIR}"
fi

# Step 1: Generate keys
echo -e "\n${GREEN}Step 1: Generating encryption keys...${NC}"
make -C "${EXAMPLE_DIR}" keys
echo "✓ Keys generated successfully"

# Step 2: Generate spec files
echo -e "\n${GREEN}Step 2: Generating specification files...${NC}"
make -C "${EXAMPLE_DIR}" specs
echo "✓ Specification files generated"

# Step 3: Generate move15.vrss.txt
echo -e "\n${GREEN}Step 3: Generating move15.vrss.txt...${NC}"
make -C "${EXAMPLE_DIR}" move15.vrss.txt
echo "✓ move15.vrss.txt generated"

# Step 4: Encrypt data
echo -e "\n${GREEN}Step 4: Encrypting vehicle data...${NC}"
make -C "${EXAMPLE_DIR}" move15.vrss.ckks
echo "✓ Data encrypted"

# Step 5: Run monitoring (if monitor executable exists)
if [ -f "${MONITOR}" ]; then
    echo -e "\n${GREEN}Step 5: Running homomorphic monitoring...${NC}"
    
    # Run reverse monitoring
    echo "Running reverse monitoring (normal mode)..."
    "${MONITOR}" reverse \
        -c "${EXAMPLE_DIR}/config.json" \
        -r "${EXAMPLE_DIR}/ckks.relinkey" \
        -b "${EXAMPLE_DIR}/tfhe.bkey" \
        -f "${EXAMPLE_DIR}/vrss.reversed.spec" \
        -m normal \
        -i "${EXAMPLE_DIR}/move15.vrss.ckks" \
        -o "${EXAMPLE_DIR}/result.tfhe" \
        --bootstrapping-freq 200 \
        --reversed
    
    echo "✓ Monitoring completed"
    
    # Step 6: Decrypt results
    echo -e "\n${GREEN}Step 6: Decrypting results...${NC}"
    "${AHOMFA_UTIL}" tfhe dec \
        -K "${EXAMPLE_DIR}/tfhe.key" \
        -i "${EXAMPLE_DIR}/result.tfhe" \
        -o "${EXAMPLE_DIR}/result.txt"
    
    echo "✓ Results decrypted"
    
    # Display results
    echo -e "\n${GREEN}Monitoring Results:${NC}"
    cat "${EXAMPLE_DIR}/result.txt"
else
    echo -e "\n${YELLOW}Note: Monitor executable not found. Build the full project to run monitoring.${NC}"
    echo "To build: cd ${BUILD_DIR} && cmake .. && make"
fi

echo -e "\n${GREEN}=========================================${NC}"
echo -e "${GREEN}Example completed successfully!${NC}"
echo -e "${GREEN}=========================================${NC}"

# Cleanup option
read -p "Do you want to clean up generated files? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    make -C "${EXAMPLE_DIR}" clean
    rm -f "${EXAMPLE_DIR}/result.tfhe" "${EXAMPLE_DIR}/result.txt"
    echo "✓ Cleanup completed"
fi
