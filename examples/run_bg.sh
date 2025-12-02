#!/bin/sh -ue

log() {
    printf "[$(date +"%Y-%m-%d %H:%M:%S")] [\e[32mArithHomFA/run_bg\e[m] %s\n" "$@"
}

ROOT="$(cd "$(dirname $0)" && pwd)"

if [ ! -d "${ROOT}" ]; then
    printf "Error: Root directory does not exist\n"
    exit 1
fi

readonly BG_ROOT="${ROOT}/blood-glucose"
readonly LOG_ROOT="${HOME}/logs/ArithHomFA/monitoring/blood-glucose"

# The options to the monitor
readonly COMMON_OPTIONS="-c ${BG_ROOT}/config.json -r ${BG_ROOT}/ckks.relinkey -b ${BG_ROOT}/tfhe.bkey"
readonly REVERSE_OPTIONS="--bootstrapping-freq 200 --reversed"
readonly BLOCK_OPTIONS="--block-size 1"

if [ $# -lt 3 ] || { [ "$1" != reverse ] && [ "$1" != block ] ;} || { [ "$2" != fast ] && [ "$2" != normal ] && [ "$2" != slow ] ;} then
    printf "Usage: %s [reverse|block] [normal|fast|slow] [benchmark_number]\n" "$0"
    exit 0
fi
readonly algorithm="$1"
shift

readonly mode="$1"
shift

# Specify the monitor
readonly number="$1"
shift

case "$number" in
    1)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_one"
        ;;
    2)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_two"
        ;;
    4)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_four"
        ;;
    5)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_five"
        ;;
    6)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_six"
        ;;
    7)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_seven"
        ;;
    8)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_eight"
        ;;
    10)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_ten"
        ;;
    11)
        readonly monitor="${ROOT}/cmake-build-release/blood-glucose/blood_glucose_eleven"
        ;;
    *)
        printf "Error: unknown benchmark number $number. Allowed numbers are 1, 2, 4, 5, 6, 7, 8, 10, 11\n"
        exit 1
esac
readonly ahomfa_util="${ROOT}/cmake-build-release/arith_homfa/ahomfa_util"

# Make the log filename
timestamp="$(date +%Y-%m-%d-%H-%M-%S)"
readonly LOG_NAME="${HOME}/logs/ArithHomFA/monitoring/blood-glucose/bg$number-$algorithm-$mode-$timestamp.log"
readonly RESULT_NAME="${HOME}/logs/ArithHomFA/monitoring/blood-glucose/bg$number-$algorithm-$mode-$timestamp.tfhe"
readonly PLAIN_NAME="${HOME}/logs/ArithHomFA/monitoring/blood-glucose/bg$number-$algorithm-$mode-$timestamp.txt"
mkdir -p "$(dirname "$LOG_NAME")"
log "LOG_NAME: $LOG_NAME"
log "RESULT_NAME: $RESULT_NAME"

if [ "$number" -le 6 ]; then
    make -C "${BG_ROOT}" adult#001_night.bg.ckks
    INPUT="${BG_ROOT}/adult#001_night.bg.ckks"
else
    make -C "${BG_ROOT}" adult#001_2nd_7days.bg.ckks
    INPUT="${BG_ROOT}/adult#001_2nd_7days.bg.ckks"
fi
log "Input is constructed: $INPUT"

make -C "${BG_ROOT}" ckks.key tfhe.key ckks.relinkey tfhe.bkey -j 2
log "Keys are generated"

if [ -f "bg${number}.spec" ] && [ -s "bg${number}.spec" ]; then
    log "bg${number}.spec already exists"
else
    make -C "${BG_ROOT}" "bg${number}.spec" -j 2
    log "bg${number}.spec is generated"
fi

case "$algorithm" in
    reverse)
        if [ -f "bg${number}.reversed.spec" ] && [ -s "bg${number}.reversed.spec" ]; then
            log "bg${number}.reversed.spec already exists"
        else
            make -C "${BG_ROOT}" "bg${number}.reversed.spec" -j 2
            log "bg${number}.reversed.spec is generated"
        fi
        "$monitor" reverse $COMMON_OPTIONS $REVERSE_OPTIONS -f "${BG_ROOT}/bg${number}.reversed.spec" -m "$mode" -i "${INPUT}" -o "$RESULT_NAME" | tee "$LOG_NAME"
        ;;
    block)
        "$monitor" block $COMMON_OPTIONS $BLOCK_OPTIONS -f "${BG_ROOT}/bg${number}.spec" -i "${INPUT}" -m "$mode" -o "$RESULT_NAME" | tee "$LOG_NAME"
        ;;
    *)
esac
"$ahomfa_util" tfhe dec -K "${BG_ROOT}/tfhe.key" -i "$RESULT_NAME" -o "$PLAIN_NAME"

notif_my_slack "finished: $0 $algorithm $mode $number $*"
