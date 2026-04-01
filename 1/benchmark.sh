#!/usr/bin/env bash
set -uo pipefail

SRC=$1
RUNS=5

OUT_DIR="results"
mkdir -p $OUT_DIR

CC=gcc

FLAG_SETS=$'O0|-O0 -fopenmp
O2|-O2 -fopenmp
O3|-O3 -fopenmp
Ofast|-Ofast -fopenmp'

MAX_THREADS=${LSB_DJOB_NUMPROC:-$(nproc)}

echo "MAX_THREADS=$MAX_THREADS"

while IFS='|' read -r TAG FLAGS; do
    BIN="${SRC%.c}_$TAG"
    CSV="$OUT_DIR/$BIN.csv"

    echo "compile $BIN"
    if ! $CC $FLAGS $SRC -o $BIN -lm; then
        echo "compile fail $TAG"
        continue
    fi

    echo "threads,run,time" > $CSV

    for ((t=1; t<=MAX_THREADS; t*=2)); do
        for ((r=1; r<=RUNS; r++)); do

            TIME=$(
                OMP_NUM_THREADS=$t \
                /usr/bin/time -f "%e" ./$BIN \
                1>/dev/null 2>&1
            )

            echo "$t,$r,$TIME" >> $CSV
        done
    done

done <<< "$FLAG_SETS"
