#!/usr/bin/env bash
set -euo pipefail

CC=${1:-gcc}
SRC=$2
FLAG_OPTIMIZE=${3:-"-O2"}
RUNS=5

N_BASE=$((2 ** 6)) # 64

OUT_DIR="results"
mkdir -p $OUT_DIR
mkdir -p bin

# --- выбор OpenMP флага ---
case "$CC" in
gcc)
    OMP_FLAG="-fopenmp"
    ;;
icc)
    OMP_FLAG="-qopenmp"
    ;;
pgcc)
    OMP_FLAG="-mp"
    ;;
xlc_r)
    OMP_FLAG="-qsmp=omp"
    ;;
*)
    echo "Unknown compiler: $CC"
    exit 1
    ;;
esac

FLAGS="$OMP_FLAG $FLAG_OPTIMIZE"

MAX_THREADS=$(nproc)

echo "Compiler=$CC"
echo "FLAGS=$FLAGS"
echo "MAX_THREADS=$MAX_THREADS"

NAME="$(basename ${SRC%.c}_${CC}_${FLAG_OPTIMIZE})"
CSV="${OUT_DIR}/${NAME}.csv"
echo "threads,run,time,N" >$CSV

for ((K = 0; K < 4; K++)); do
    N=$((N_BASE * (2 ** K) + 2))

    NAME_N="$(basename ${SRC%.c}_${CC}_${FLAG_OPTIMIZE}_${N})"
    BIN="bin/${NAME_N}"

    echo "compile ${NAME_N}"

    if ! $CC $FLAGS $SRC -DN=$N -o $BIN -lm; then
        echo "compile fail ${CC}_${FLAG_OPTIMIZE}_${N}"
        continue
    fi

    for ((t = 1; t <= MAX_THREADS; t *= 2)); do
        for ((r = 1; r <= RUNS; r++)); do
            TIME=$(OMP_NUM_THREADS=$t "$BIN" | awk -F'=' '/time=/ {print $2}')
            echo "$t,$r,$TIME,$N" >>"$CSV"
        done
    done

    echo "Done N=$N"
done
