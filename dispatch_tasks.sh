#!/usr/bin/env bash
set -euo pipefail

COMPILERS=(gcc icc pgcc xlc_r)
OPTS=(-O0 -O2 -O3 -Ofast)
N_LIST=(66 130 258 514)
THREADS_LIST=(1 2 4 8 16)
RUNS=3

TASKS_DIR="tasks"

get_omp_flag() {
    case "$1" in
    gcc) echo "-fopenmp" ;;
    icc) echo "-qopenmp" ;;
    pgcc) echo "-mp" ;;
    xlc_r) echo "-qsmp=omp" ;;
    *)
        echo "Unknown compiler: $1"
        exit 1
        ;;
    esac
}

normalize_opt() {
    local opt="$1"
    echo "${opt#-}"
}

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 src/file1.c [src/file2.c ...]"
    exit 1
fi

for SRC in "$@"; do
    SRC_BASE="$(basename "${SRC%.c}")"

    for CC in "${COMPILERS[@]}"; do
        OMP_FLAG="$(get_omp_flag "$CC")"

        for OPT in "${OPTS[@]}"; do
            OPT_NORM="$(normalize_opt "$OPT")"

            for N in "${N_LIST[@]}"; do
                for T in "${THREADS_LIST[@]}"; do
                    for R in $(seq 1 "$RUNS"); do
                        TASK_NAME="${SRC_BASE}__cc-${CC}__opt-${OPT_NORM}__N-${N}__t-${T}__run-${R}"
                        TASK_PATH="${TASKS_DIR}/${TASK_NAME}"

                        echo "build: $TASK_NAME"
                        $CC $OMP_FLAG $OPT "$SRC" -DN=$N -DTHREADS=$T -lm -o "$TASK_PATH" || continue
                    done
                done
            done
        done
    done
done
