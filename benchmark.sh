#!/usr/bin/env bash
set -euo pipefail

# --- аргументы ---
BIN_DIR=${1:-bin}
PATTERN=${2:-"*"}

# --- настройки ---
THREADS_LIST=(1 2 4 8 16)
RUNS=3

CSV_DIR="results"
CSV_FILE="${CSV_DIR}/benchmark_results.csv"

mkdir -p "$CSV_DIR"

# --- создать CSV если нет ---
if [ ! -f "$CSV_FILE" ]; then
    echo "source,binary,compiler,opt,N,threads,run,time" >"$CSV_FILE"
fi

# --- проверка существования записи ---
exists_record() {
    local binary=$1
    local threads=$2
    local run=$3

    grep -q "^.*,${binary},.*,.*,.*,${threads},${run}," "$CSV_FILE"
}

# --- парсинг имени ---
parse_binary() {
    local name="$1"

    # var03.foromp__cc-gcc__opt-O3__N-130
    SOURCE="${name%%__cc-*}.c"

    COMPILER=$(echo "$name" | sed -n 's/.*__cc-\([^_]*\).*/\1/p')
    OPT=$(echo "$name" | sed -n 's/.*__opt-\([^_]*\).*/\1/p')
    N=$(echo "$name" | sed -n 's/.*__N-\([0-9]*\).*/\1/p')
}

echo "Scanning ${BIN_DIR}/${PATTERN}"

shopt -s nullglob

for BIN_PATH in "$BIN_DIR"/$PATTERN; do
    [ -x "$BIN_PATH" ] || continue

    BIN_NAME=$(basename "$BIN_PATH")

    parse_binary "$BIN_NAME"

    echo "run: $BIN_NAME"

    for t in "${THREADS_LIST[@]}"; do
        for r in $(seq 1 $RUNS); do

            if exists_record "$BIN_NAME" "$t" "$r"; then
                echo "skip: $BIN_NAME t=$t r=$r"
                continue
            fi

            TIME=$(OMP_NUM_THREADS=$t "$BIN_PATH" | awk -F'=' '/time=/ {print $2}')

            echo "${SOURCE},${BIN_NAME},${COMPILER},${OPT},${N},${t},${r},${TIME}" >>"$CSV_FILE"

        done
    done
done
