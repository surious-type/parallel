#!/usr/bin/env bash
set -euo pipefail

TASKS_DIR="${1:-tasks}"
PATTERN="${2:-*}"

RESULTS_DIR="results"
PARTS_DIR="${RESULTS_DIR}/parts"
mkdir -p "$PARTS_DIR"

HOST="$(hostname)"
PID="$$"
PART_CSV="${PARTS_DIR}/${HOST}__pid-${PID}.csv"
touch "$PART_CSV"

parse_task_name() {
    local name="$1"

    SOURCE="${name%%__cc-*}.c"
    COMPILER="$(echo "$name" | sed -n 's/.*__cc-\([^_]*\).*/\1/p')"
    OPT="$(echo "$name" | sed -n 's/.*__opt-\([^_]*\).*/\1/p')"
    N="$(echo "$name" | sed -n 's/.*__N-\([0-9]*\).*/\1/p')"
    THREADS="$(echo "$name" | sed -n 's/.*__t-\([0-9]*\).*/\1/p')"
}

shopt -s nullglob

for p in "$TASKS_DIR"/$PATTERN; do
    [ -f "$p" ] || continue
    [ -x "$p" ] || continue

    case "$p" in
    *.lock | *.done) continue ;;
    esac

    TASK_PATH="$p"

    LOCK_PATH="${TASK_PATH}.lock"

    if ! mv "$TASK_PATH" "$LOCK_PATH" 2>/dev/null; then
        continue
    fi

    TASK_NAME="$(basename "$TASK_PATH")"
    parse_task_name "$TASK_NAME"

    echo "run: $TASK_NAME"

    TIME="$("$LOCK_PATH" | awk -F'=' '/time=/ {print $2}')"

    echo "${SOURCE},${TASK_NAME},${COMPILER},${OPT},${N},${THREADS},${TIME},${HOST}" >>"$PART_CSV"

    mv "$LOCK_PATH" "${TASK_PATH}.done"
done
