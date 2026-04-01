#!/usr/bin/env bash
set -uo pipefail

SRC=$1
RUNS=5

OUT_DIR="results"
mkdir -p $OUT_DIR

CC=xlc_r

FLAG_SETS=$'
O0|-O0
O2|-O2
O3|-O3
O4|-O4
O5|-O5
'

MAX_THREADS=${LSB_DJOB_NUMPROC:-$(nproc)}

echo "MAX_THREADS=$MAX_THREADS"

while IFS='|' read -r TAG FLAGS; do
    BIN="${SRC%.c}_$TAG"
    CSV="$OUT_DIR/$BIN.csv"

    echo "compile $BIN"
    if ! $CC $FLAGS $SRC -qsmp=omp -o $BIN -lm; then
        echo "compile fail $TAG"
        continue
    fi

    echo "threads,run,time" > $CSV

    for ((t=1; t<=MAX_THREADS; t*=2)); do
        for ((r=1; r<=RUNS; r++)); do
            start=$(date +%s.%N)

            OMP_NUM_THREADS=$t ./$BIN > /dev/null 2>&1

            end=$(date +%s.%N)

            TIME=$(echo "$end - $start" | bc)

            echo "$t,$r,$TIME" >> $CSV
        done
    done

done <<< "$FLAG_SETS"
