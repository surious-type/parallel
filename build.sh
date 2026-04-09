#!/usr/bin/env bash
set -euo pipefail

# --- настройки ---
COMPILERS=(gcc icc pgcc xlc_r)
OPTS=(-O2 -O3 -Ofast -fast)
N_LIST=(66 130 258 514)

BIN_DIR="bin"
mkdir -p "$BIN_DIR"

# --- функция выбора OpenMP флага ---
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

# --- нормализация флага ---
normalize_opt() {
    local opt="$1"
    opt="${opt#-}" # убрать "-"
    echo "$opt"
}

# --- вход: список исходников ---
if [ "$#" -lt 1 ]; then
    echo "Usage: $0 src/file1.c [src/file2.c ...]"
    exit 1
fi

for SRC in "$@"; do
    SRC_BASE="$(basename "${SRC%.c}")"

    for CC in "${COMPILERS[@]}"; do
        OMP_FLAG=$(get_omp_flag "$CC")

        for OPT in "${OPTS[@]}"; do
            OPT_NORM=$(normalize_opt "$OPT")

            for N in "${N_LIST[@]}"; do

                BIN_NAME="${SRC_BASE}__cc-${CC}__opt-${OPT_NORM}__N-${N}"
                BIN_PATH="${BIN_DIR}/${BIN_NAME}"

                echo "build: $BIN_NAME"

                $CC $OMP_FLAG $OPT "$SRC" -DN=$N -lm -o "$BIN_PATH" || continue

            done
        done
    done
done
