#!/bin/bash
################################################
# NAME
#  run_vrs
################################################

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
EXAMPLE_DIR="$(cd "$(dirname "$0")" && pwd)/vehicle_rss"
BUILD_DIR="${EXAMPLE_DIR}/../build"
AHOMFA_UTIL="${BUILD_DIR}/arith_homfa/ahomfa_util"
MONITOR="${BUILD_DIR}/vehicle_rss/vehicle_rss"

# Output locations
PLAIN_RESULT="${EXAMPLE_DIR}/result_plain.txt"
BLOCK_RESULT_TFHE="${EXAMPLE_DIR}/result_block.tfhe"
BLOCK_RESULT="${EXAMPLE_DIR}/result_block.txt"
REVERSE_RESULT_TFHE="${EXAMPLE_DIR}/result_reverse.tfhe"
REVERSE_RESULT="${EXAMPLE_DIR}/result_reverse.txt"
MONITOR_MODE="fast"
BLOCK_SIZE=1

# Check if build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo -e "${YELLOW}Build directory not found. Creating and building...${NC}"
    cmake -S "${BUILD_DIR}/.." -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}"
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

# Step 5: Run monitoring across plain, block, and reverse algorithms
echo -e "\n${GREEN}Step 5: Running monitoring in plain, block, and reverse modes...${NC}"

echo "Running plain monitoring (baseline)..."
"${MONITOR}" plain \
             -c "${EXAMPLE_DIR}/config.json" \
             -f "${EXAMPLE_DIR}/vrss.spec" \
             -i "${EXAMPLE_DIR}/move15.vrss.txt" \
             -o "${PLAIN_RESULT}"
echo "✓ Plain monitoring completed"

echo "Running block monitoring (${MONITOR_MODE} mode)..."
"${MONITOR}" block \
             -c "${EXAMPLE_DIR}/config.json" \
             -r "${EXAMPLE_DIR}/ckks.relinkey" \
             -b "${EXAMPLE_DIR}/tfhe.bkey" \
             -f "${EXAMPLE_DIR}/vrss.spec" \
             -l "${BLOCK_SIZE}" \
             -m "${MONITOR_MODE}" \
             -i "${EXAMPLE_DIR}/move15.vrss.ckks" \
             -o "${BLOCK_RESULT_TFHE}"
echo "✓ Block monitoring completed"

echo "Running reverse monitoring (${MONITOR_MODE} mode)..."
"${MONITOR}" reverse \
             -c "${EXAMPLE_DIR}/config.json" \
             -r "${EXAMPLE_DIR}/ckks.relinkey" \
             -b "${EXAMPLE_DIR}/tfhe.bkey" \
             -f "${EXAMPLE_DIR}/vrss.reversed.spec" \
             -m "${MONITOR_MODE}" \
             -i "${EXAMPLE_DIR}/move15.vrss.ckks" \
             -o "${REVERSE_RESULT_TFHE}" \
             --bootstrapping-freq 200 \
             --reversed
echo "✓ Reverse monitoring completed"

# Step 6: Decrypt TFHE results
echo -e "\n${GREEN}Step 6: Decrypting homomorphic monitoring results...${NC}"
"${AHOMFA_UTIL}" tfhe dec \
                 -K "${EXAMPLE_DIR}/tfhe.key" \
                 -i "${BLOCK_RESULT_TFHE}" \
                 -o "${BLOCK_RESULT}"
"${AHOMFA_UTIL}" tfhe dec \
                 -K "${EXAMPLE_DIR}/tfhe.key" \
                 -i "${REVERSE_RESULT_TFHE}" \
                 -o "${REVERSE_RESULT}"
echo "✓ Block and reverse results decrypted"

# Step 7: Verify that all modes produce the same outcome
echo -e "\n${GREEN}Step 7: Validating monitoring consistency...${NC}"
PLAIN_LINES=$(grep -cve '^[[:space:]]*$' "${PLAIN_RESULT}" || true)
BLOCK_LINES=$(grep -cve '^[[:space:]]*$' "${BLOCK_RESULT}" || true)
REVERSE_LINES=$(grep -cve '^[[:space:]]*$' "${REVERSE_RESULT}" || true)
MIN_LINES=$(printf "%s\n%s\n%s\n" "${PLAIN_LINES}" "${BLOCK_LINES}" "${REVERSE_LINES}" | sort -n | head -n1)

if [ "${MIN_LINES}" -eq 0 ]; then
    echo -e "${RED}✗ Unable to compare monitoring outputs: one of the result files is empty${NC}"
    exit 1
fi

if [ "${PLAIN_LINES}" -ne "${BLOCK_LINES}" ] || [ "${PLAIN_LINES}" -ne "${REVERSE_LINES}" ]; then
    echo -e "${YELLOW}Note: result lengths differ (plain=${PLAIN_LINES}, block=${BLOCK_LINES}, reverse=${REVERSE_LINES}). Comparing the first ${MIN_LINES} entries.${NC}"
fi

if cmp -s <(head -n "${MIN_LINES}" "${PLAIN_RESULT}") <(head -n "${MIN_LINES}" "${BLOCK_RESULT}") \
   && cmp -s <(head -n "${MIN_LINES}" "${PLAIN_RESULT}") <(head -n "${MIN_LINES}" "${REVERSE_RESULT}"); then
    echo "✓ Plain, block, and reverse monitoring results match on the compared entries"
else
    echo -e "${RED}✗ Monitoring outputs differ between modes${NC}"
    echo "Plain vs Block diff:"
    diff -u <(head -n "${MIN_LINES}" "${PLAIN_RESULT}") <(head -n "${MIN_LINES}" "${BLOCK_RESULT}") || true
    echo "Plain vs Reverse diff:"
    diff -u <(head -n "${MIN_LINES}" "${PLAIN_RESULT}") <(head -n "${MIN_LINES}" "${REVERSE_RESULT}") || true
    exit 1
fi

# Display results
echo -e "\n${GREEN}Monitoring Results:${NC}"
cat "${PLAIN_RESULT}"

echo -e "\n${GREEN}=========================================${NC}"
echo -e "${GREEN}Example completed successfully!${NC}"
echo -e "${GREEN}=========================================${NC}"

# Cleanup option
read -p "Do you want to clean up generated files? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    make -C "${EXAMPLE_DIR}" clean
    rm -f \
        "${BLOCK_RESULT_TFHE}" \
        "${BLOCK_RESULT}" \
        "${REVERSE_RESULT_TFHE}" \
        "${REVERSE_RESULT}" \
        "${PLAIN_RESULT}" \
        "${EXAMPLE_DIR}/result.tfhe" \
        "${EXAMPLE_DIR}/result.txt"
    echo "✓ Cleanup completed"
fi
