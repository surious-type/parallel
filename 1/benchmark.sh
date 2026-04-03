#!/usr/bin/env bash
set -euo pipefail

SRC=$1
COMPILER=${2:-gcc}
RUNS=5

OUT_DIR="results"
mkdir -p $OUT_DIR

if [ "$COMPILER" = "xlc_r" ]; then
    CC="xlc_r"
    FLAG_SETS=$'O2|-qsmp=omp
O3|-qsmp=omp -O3
O4|-qsmp=omp -O4
O5|-qsmp=omp -O5'
else
    CC="gcc"
    FLAG_SETS=$'O2|-fopenmp -O2
O3|-fopenmp -O3
O4|-fopenmp -O4
O5|-fopenmp -O5'
fi

MAX_THREADS=${LSB_DJOB_NUMPROC:-$(nproc)}

echo "MAX_THREADS=$MAX_THREADS"

while IFS='|' read -r TAG FLAGS; do
    NAME="$(basename ${SRC%.c}_$TAG)"
    BIN="bin/$NAME"
    CSV="$OUT_DIR/$NAME.csv"

    echo "compile $NAME"
    if ! $CC $FLAGS $SRC -o $BIN -lm; then
        echo "compile fail $TAG"
        continue
    fi

    echo "threads,run,time" >$CSV

    for ((t = 1; t <= MAX_THREADS; t *= 2)); do
        for ((r = 1; r <= RUNS; r++)); do
            TIME=$(OMP_NUM_THREADS=$t "$BIN" | awk -F'=' '/time=/ {print $2}')
            echo "$t,$r,$TIME" >>"$CSV"
        done
    done

done <<<"$FLAG_SETS"
