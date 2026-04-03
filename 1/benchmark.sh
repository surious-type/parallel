#!/usr/bin/env bash
set -uo pipefail

SRC=$1
RUNS=5

OUT_DIR="results"
mkdir -p $OUT_DIR

if [ "$2" = "xlc_r" ]; then
    CC="xlc_r -qsmp=omp"
else
    CC="gcc -fopenmp"
fi

FLAG_SETS=$'O0|-O0
O2|-O2
O3|-O3
O4|-O4
O5|-O5'

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
