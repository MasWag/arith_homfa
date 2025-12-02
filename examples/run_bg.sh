#!/usr/bin/env bash
set -euo pipefail

echo "========================================="
echo "Blood Glucose Homomorphic Monitoring Example"
echo "========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

usage() {
    cat <<'EOF'
Usage: ./run_bg.sh [options]

Options:
  --formula <id>     Formula identifier (e.g., 1, 7). Defaults to 1.
  --dataset <name>   Dataset override (adult#001_night | adult#001_2nd_7days).
  --mode <mode>      Monitoring mode for block/reverse (fast | normal | slow). Default: fast
  --block-size <n>   Block size for block mode. Default: 1
  --bootstrap <n>    Bootstrapping frequency for reverse mode. Default: 200
  -h, --help         Show this help message and exit
EOF
}

FORMULA_ID=1
DATASET_NAME=""
CUSTOM_DATASET=false
MONITOR_MODE="${MONITOR_MODE:-fast}"
BLOCK_SIZE="${BLOCK_SIZE:-1}"
BOOTSTRAP_FREQ="${BOOTSTRAP_FREQ:-200}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --formula)
            [[ $# -lt 2 ]] && { echo "Missing value for --formula"; exit 1; }
            FORMULA_ID="$2"
            shift 2
            ;;
        --dataset)
            [[ $# -lt 2 ]] && { echo "Missing value for --dataset"; exit 1; }
            DATASET_NAME="$2"
            CUSTOM_DATASET=true
            shift 2
            ;;
        --mode)
            [[ $# -lt 2 ]] && { echo "Missing value for --mode"; exit 1; }
            MONITOR_MODE="$2"
            shift 2
            ;;
        --block-size)
            [[ $# -lt 2 ]] && { echo "Missing value for --block-size"; exit 1; }
            BLOCK_SIZE="$2"
            shift 2
            ;;
        --bootstrap)
            [[ $# -lt 2 ]] && { echo "Missing value for --bootstrap"; exit 1; }
            BOOTSTRAP_FREQ="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

if ! [[ "${FORMULA_ID}" =~ ^[0-9]+$ ]]; then
    echo "Formula id must be numeric"
    exit 1
fi

if [[ "${CUSTOM_DATASET}" = false ]]; then
    if (( FORMULA_ID <= 6 )); then
        DATASET_NAME="adult#001_night"
    else
        DATASET_NAME="adult#001_2nd_7days"
    fi
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EXAMPLE_DIR="${SCRIPT_DIR}/blood_glucose"
BUILD_DIR="${SCRIPT_DIR}/build"
AHOMFA_UTIL="${BUILD_DIR}/arith_homfa/ahomfa_util"
MONITOR_NAME="blood_glucose_one"
case "${FORMULA_ID}" in
    1) MONITOR_NAME="blood_glucose_one" ;;
    2) MONITOR_NAME="blood_glucose_two" ;;
    4) MONITOR_NAME="blood_glucose_four" ;;
    5) MONITOR_NAME="blood_glucose_five" ;;
    6) MONITOR_NAME="blood_glucose_six" ;;
    7) MONITOR_NAME="blood_glucose_seven" ;;
    8) MONITOR_NAME="blood_glucose_eight" ;;
    10) MONITOR_NAME="blood_glucose_ten" ;;
    11) MONITOR_NAME="blood_glucose_eleven" ;;
    *)
        MONITOR_NAME="blood_glucose_one"
        echo -e "${YELLOW}Formula ${FORMULA_ID} does not map to a dedicated binary; defaulting to blood_glucose_one${NC}"
        ;;
esac
MONITOR="${BUILD_DIR}/blood_glucose/${MONITOR_NAME}"
CONFIG="${EXAMPLE_DIR}/config.json"

if [[ ! -f "${CONFIG}" ]]; then
    echo -e "${RED}Configuration file not found at ${CONFIG}${NC}"
    exit 1
fi

# Build artifacts if necessary
if [[ ! -x "${MONITOR}" ]] || [[ ! -x "${AHOMFA_UTIL}" ]]; then
    echo -e "${YELLOW}Build artifacts missing. Configuring and building examples...${NC}"
    cmake -S "${BUILD_DIR}/.." -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}"
fi

if [[ ! -x "${MONITOR}" ]]; then
    ALT_MONITOR="${EXAMPLE_DIR}/${MONITOR_NAME}"
    if [[ -x "${ALT_MONITOR}" ]]; then
        MONITOR="${ALT_MONITOR}"
    else
        echo -e "${RED}Monitor binary ${MONITOR_NAME} not found. Build it first.${NC}"
        exit 1
    fi
fi

CKKS_KEY="${EXAMPLE_DIR}/ckks.key"
CKKS_RELINKEY="${EXAMPLE_DIR}/ckks.relinkey"
TFHE_KEY="${EXAMPLE_DIR}/tfhe.key"
TFHE_BKEY="${EXAMPLE_DIR}/tfhe.bkey"

FORMULA_FILE="${EXAMPLE_DIR}/bg${FORMULA_ID}.ltl"
SPEC_FILE="${EXAMPLE_DIR}/bg${FORMULA_ID}.spec"
REVERSED_SPEC_FILE="${EXAMPLE_DIR}/bg${FORMULA_ID}.reversed.spec"

if [[ ! -f "${FORMULA_FILE}" ]]; then
    echo -e "${RED}Formula file not found at ${FORMULA_FILE}${NC}"
    exit 1
fi

sanitize_name() {
    local raw="$1"
    raw="${raw//[^A-Za-z0-9._-]/_}"
    printf "%s" "${raw}"
}

RESULT_DIR="${EXAMPLE_DIR}/results"
mkdir -p "${RESULT_DIR}"
RESULT_TAG="bg${FORMULA_ID}_$(sanitize_name "${DATASET_NAME}")"
PLAIN_RESULT="${RESULT_DIR}/${RESULT_TAG}_plain.txt"
BLOCK_RESULT_TFHE="${RESULT_DIR}/${RESULT_TAG}_block.tfhe"
BLOCK_RESULT="${RESULT_DIR}/${RESULT_TAG}_block.txt"
REVERSE_RESULT_TFHE="${RESULT_DIR}/${RESULT_TAG}_reverse.tfhe"
REVERSE_RESULT="${RESULT_DIR}/${RESULT_TAG}_reverse.txt"

dataset_csv="${EXAMPLE_DIR}/homfa-experiment/ap9/${DATASET_NAME}.csv"
PLAINTEXT_DATA="${EXAMPLE_DIR}/${DATASET_NAME}.bg.txt"
ENCRYPTED_DATA="${EXAMPLE_DIR}/${DATASET_NAME}.bg.ckks"

if [[ ! -f "${dataset_csv}" ]]; then
    echo -e "${RED}Dataset ${dataset_csv} not found${NC}"
    exit 1
fi

echo -e "\n${GREEN}Step 1: Generating encryption keys...${NC}"
if [[ ! -f "${CKKS_KEY}" ]]; then
    "${AHOMFA_UTIL}" ckks genkey -c "${CONFIG}" -o "${CKKS_KEY}"
    echo "Generated CKKS secret key"
else
    echo "CKKS secret key already exists"
fi

if [[ ! -f "${CKKS_RELINKEY}" ]] || [[ "${CKKS_KEY}" -nt "${CKKS_RELINKEY}" ]]; then
    "${AHOMFA_UTIL}" ckks genrelinkey -c "${CONFIG}" -K "${CKKS_KEY}" -o "${CKKS_RELINKEY}"
    echo "Generated CKKS relinkey"
else
    echo "CKKS relinkey already up to date"
fi

if [[ ! -f "${TFHE_KEY}" ]]; then
    "${AHOMFA_UTIL}" tfhe genkey -o "${TFHE_KEY}"
    echo "Generated TFHE secret key"
else
    echo "TFHE secret key already exists"
fi

if [[ ! -f "${TFHE_BKEY}" ]] || [[ "${TFHE_KEY}" -nt "${TFHE_BKEY}" ]] || [[ "${CKKS_KEY}" -nt "${TFHE_BKEY}" ]]; then
    "${AHOMFA_UTIL}" tfhe genbkey -c "${CONFIG}" -K "${TFHE_KEY}" -S "${CKKS_KEY}" -o "${TFHE_BKEY}"
    echo "Generated TFHE bootstrapping key"
else
    echo "TFHE bootstrapping key already up to date"
fi

echo -e "\n${GREEN}Step 2: Generating specification files...${NC}"
PREDICATES=$(grep -o 'p[0-9]\+' "${FORMULA_FILE}" || true)
if [[ -z "${PREDICATES}" ]]; then
    NUM_VARS=1
else
    NUM_VARS=$(printf "%s\n" "${PREDICATES}" | sort -u | wc -l | tr -d ' ')
fi

if [[ ! -f "${SPEC_FILE}" ]] || [[ "${FORMULA_FILE}" -nt "${SPEC_FILE}" ]]; then
    "${AHOMFA_UTIL}" ltl2spec -n "${NUM_VARS}" -e "$(cat "${FORMULA_FILE}")" -o "${SPEC_FILE}"
    echo "Generated ${SPEC_FILE}"
else
    echo "Specification already up to date"
fi

if [[ ! -f "${REVERSED_SPEC_FILE}" ]] || [[ "${SPEC_FILE}" -nt "${REVERSED_SPEC_FILE}" ]]; then
    "${AHOMFA_UTIL}" spec2spec --reverse -i "${SPEC_FILE}" -o "${REVERSED_SPEC_FILE}"
    echo "Generated ${REVERSED_SPEC_FILE}"
else
    echo "Reversed specification already up to date"
fi

echo -e "\n${GREEN}Step 3: Preparing plaintext data (${DATASET_NAME})...${NC}"
if [[ ! -f "${PLAINTEXT_DATA}" ]] || [[ "${dataset_csv}" -nt "${PLAINTEXT_DATA}" ]]; then
    cut -d ',' -f 2 "${dataset_csv}" | sed '1 d' > "${PLAINTEXT_DATA}"
    echo "Created ${PLAINTEXT_DATA}"
else
    echo "Plaintext data already prepared"
fi

echo -e "\n${GREEN}Step 4: Encrypting measurements (${DATASET_NAME})...${NC}"
if [[ ! -f "${ENCRYPTED_DATA}" ]] || [[ "${PLAINTEXT_DATA}" -nt "${ENCRYPTED_DATA}" ]] || [[ "${CKKS_KEY}" -nt "${ENCRYPTED_DATA}" ]]; then
    "${AHOMFA_UTIL}" ckks enc -c "${CONFIG}" -K "${CKKS_KEY}" -i "${PLAINTEXT_DATA}" -o "${ENCRYPTED_DATA}"
    echo "Encrypted data stored in ${ENCRYPTED_DATA}"
else
    echo "Encrypted data already up to date"
fi

echo -e "\n${GREEN}Step 5: Running monitoring in plain, block, and reverse modes...${NC}"
echo "Running plain monitoring..."
"${MONITOR}" plain \
    -c "${CONFIG}" \
    -f "${SPEC_FILE}" \
    -i "${PLAINTEXT_DATA}" \
    -o "${PLAIN_RESULT}"
echo "[OK] Plain monitoring completed"

echo "Running block monitoring (mode=${MONITOR_MODE})..."
"${MONITOR}" block \
    -c "${CONFIG}" \
    -r "${CKKS_RELINKEY}" \
    -b "${TFHE_BKEY}" \
    -f "${SPEC_FILE}" \
    --block-size "${BLOCK_SIZE}" \
    -m "${MONITOR_MODE}" \
    -i "${ENCRYPTED_DATA}" \
    -o "${BLOCK_RESULT_TFHE}"
echo "[OK] Block monitoring completed"

echo "Running reverse monitoring (mode=${MONITOR_MODE})..."
"${MONITOR}" reverse \
    -c "${CONFIG}" \
    -r "${CKKS_RELINKEY}" \
    -b "${TFHE_BKEY}" \
    -f "${REVERSED_SPEC_FILE}" \
    -m "${MONITOR_MODE}" \
    -i "${ENCRYPTED_DATA}" \
    -o "${REVERSE_RESULT_TFHE}" \
    --bootstrapping-freq "${BOOTSTRAP_FREQ}" \
    --reversed
echo "[OK] Reverse monitoring completed"

echo -e "\n${GREEN}Step 6: Decrypting homomorphic monitoring results...${NC}"
"${AHOMFA_UTIL}" tfhe dec \
    -K "${TFHE_KEY}" \
    -i "${BLOCK_RESULT_TFHE}" \
    -o "${BLOCK_RESULT}"
"${AHOMFA_UTIL}" tfhe dec \
    -K "${TFHE_KEY}" \
    -i "${REVERSE_RESULT_TFHE}" \
    -o "${REVERSE_RESULT}"
echo "[OK] Block and reverse outputs decrypted"

echo -e "\n${GREEN}Step 7: Validating monitoring consistency...${NC}"
PLAIN_LINES=$(grep -cve '^[[:space:]]*$' "${PLAIN_RESULT}" || true)
BLOCK_LINES=$(grep -cve '^[[:space:]]*$' "${BLOCK_RESULT}" || true)
REVERSE_LINES=$(grep -cve '^[[:space:]]*$' "${REVERSE_RESULT}" || true)
MIN_LINES=$(printf "%s\n%s\n%s\n" "${PLAIN_LINES}" "${BLOCK_LINES}" "${REVERSE_LINES}" | sort -n | head -n1)

if [[ "${MIN_LINES}" -eq 0 ]]; then
    echo -e "${RED}One of the result files is empty; cannot compare outputs${NC}"
    exit 1
fi

if [[ "${PLAIN_LINES}" -ne "${BLOCK_LINES}" ]] || [[ "${PLAIN_LINES}" -ne "${REVERSE_LINES}" ]]; then
    echo -e "${YELLOW}Result lengths differ (plain=${PLAIN_LINES}, block=${BLOCK_LINES}, reverse=${REVERSE_LINES}). Comparing the first ${MIN_LINES} entries.${NC}"
fi

if cmp -s <(head -n "${MIN_LINES}" "${PLAIN_RESULT}") <(head -n "${MIN_LINES}" "${BLOCK_RESULT}") \
   && cmp -s <(head -n "${MIN_LINES}" "${PLAIN_RESULT}") <(head -n "${MIN_LINES}" "${REVERSE_RESULT}"); then
    echo "[OK] Plain, block, and reverse monitoring results match on compared entries"
else
    echo -e "${RED}Monitoring outputs differ between modes${NC}"
    echo "Plain vs Block diff:"
    diff -u <(head -n "${MIN_LINES}" "${PLAIN_RESULT}") <(head -n "${MIN_LINES}" "${BLOCK_RESULT}") || true
    echo "Plain vs Reverse diff:"
    diff -u <(head -n "${MIN_LINES}" "${PLAIN_RESULT}") <(head -n "${MIN_LINES}" "${REVERSE_RESULT}") || true
    exit 1
fi

echo -e "\n${GREEN}Monitoring Results:${NC}"
cat "${PLAIN_RESULT}"

echo -e "\n${GREEN}=========================================${NC}"
echo -e "${GREEN}Blood glucose example completed successfully!${NC}"
echo -e "${GREEN}=========================================${NC}"

if [[ -t 0 ]]; then
    read -r -p "Do you want to clean up generated files? (y/n) " -n 1 REPLY
    echo
    if [[ "${REPLY}" =~ ^[Yy]$ ]]; then
        rm -f \
            "${BLOCK_RESULT_TFHE}" \
            "${BLOCK_RESULT}" \
            "${REVERSE_RESULT_TFHE}" \
            "${REVERSE_RESULT}" \
            "${PLAIN_RESULT}"
        echo "[OK] Cleanup completed"
    fi
fi
