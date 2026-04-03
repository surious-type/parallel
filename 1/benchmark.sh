#!/usr/bin/env bash
set -euo pipefail

CC=${1:-gcc}
SRC=$2
FLAG_OPTIMIZE=${3:-"-O2"}
RUNS=5

N_BASE=$((2 ** 6)) # 64
MEM=$(awk '/MemAvailable/ {print $2}' /proc/meminfo)
MEM_HALF=$((MEM / 2))
K=$(awk -v m="$MEM_HALF" -v nbase="$N_BASE" 'BEGIN {printf "%d", ((m*1024/8)^(1/3))/nbase}')

OUT_DIR="results"
mkdir -p $OUT_DIR

if [ "$CC" = "xlc_r" ]; then
    FLAGS="-qsmp=omp ${FLAG_OPTIMIZE}"
else
    FLAGS="-fopenmp ${FLAG_OPTIMIZE}"
fi

MAX_THREADS=${LSB_DJOB_NUMPROC:-$(nproc)}

echo "MAX_THREADS=$MAX_THREADS"

NAME="$(basename ${SRC%.c}_${FLAG_OPTIMIZE})"
CSV="${OUT_DIR}/${NAME}.csv"
echo "threads,run,time,N" >$CSV

for (( ; K > 0; K--)); do
    N=$((2 ** K + 2))
    ((N < N_BASE)) && break
    echo $N_BASE
    echo $N
    NAME_N="$(basename ${SRC%.c}_${FLAG_OPTIMIZE}_${N})"
    BIN="bin/${NAME_N}"
    echo "compile ${NAME_N}"

    if ! $CC $FLAGS $SRC -DN=$N -o $BIN -lm; then
        echo "compile fail ${FLAG_OPTIMIZE}_${N}"
        continue
    fi

    for ((t = 1; t <= MAX_THREADS; t *= 2)); do
        for ((r = 1; r <= RUNS; r++)); do
            TIME=$(OMP_NUM_THREADS=$t "$BIN" | awk -F'=' '/time=/ {print $2}')
            echo "$t,$r,$TIME,$N" >>"$CSV"
        done
    done
done
