#!/usr/bin/env bash
set -Eeuo pipefail

usage() {
    cat <<'USAGE'
Usage:
  ./omp_benchmark.sh source.c

Environment variables:
  CC                Compiler (default: gcc)
  RUNS              Repetitions per configuration (default: 5)
  START_THREADS     First thread count (default: 1)
  MAX_THREADS       Max thread count to test (default: auto-detect)
  BUILD_DIR         Directory for binaries (default: build_bench)
  KEEP_BINARIES     1 = keep compiled binaries, 0 = remove after benchmark (default: 1)

Custom flag sets:
  By default, the script tests:
    O0    -> -O0 -fopenmp
    O1    -> -O1 -fopenmp
    O2    -> -O2 -fopenmp
    O3    -> -O3 -fopenmp
    Ofast -> -Ofast -fopenmp

  You can override them via FLAG_SETS as a newline-separated list:
    export FLAG_SETS=$'O2|-O2 -fopenmp\nO3_native|-O3 -march=native -fopenmp'

CSV columns:
  timestamp,host,compiler,flag_tag,flags,run,threads,max_threads,elapsed_sec,status,binary
USAGE
}

if [[ ${1:-} == "-h" || ${1:-} == "--help" ]]; then
    usage
    exit 0
fi

if [[ $# -lt 1 ]]; then
    usage
    exit 1
fi

SRC=$1
CC=${CC:-gcc}
RUNS=${RUNS:-5}
START_THREADS=${START_THREADS:-1}
BUILD_DIR=${BUILD_DIR:-build}
KEEP_BINARIES=${KEEP_BINARIES:-1}
OUT_DIR="${2:-benchmark_csv}"

mkdir -p "$OUT_DIR"

if [[ ! -f "$SRC" ]]; then
    echo "Error: source file '$SRC' not found" >&2
    exit 1
fi

if ! command -v "$CC" >/dev/null 2>&1; then
    echo "Error: compiler '$CC' not found" >&2
    exit 1
fi

if ! command -v /usr/bin/time >/dev/null 2>&1; then
    echo "Error: /usr/bin/time not found" >&2
    exit 1
fi

if ! command -v nproc >/dev/null 2>&1; then
    echo "Error: nproc not found" >&2
    exit 1
fi

MAX_THREADS=${LSB_DJOB_NUMPROC:-$(nproc)}

if ! [[ "$RUNS" =~ ^[0-9]+$ ]] || (( RUNS < 1 )); then
    echo "Error: RUNS must be a positive integer" >&2
    exit 1
fi

if ! [[ "$START_THREADS" =~ ^[0-9]+$ ]] || (( START_THREADS < 1 )); then
    echo "Error: START_THREADS must be a positive integer" >&2
    exit 1
fi

mkdir -p "$BUILD_DIR"

SRC_NAME=$(basename "$SRC")
SRC_STEM=${SRC_NAME%.*}
STAMP=$(date +%Y%m%d_%H%M%S)
HOST=$(hostname)

if [[ -n "${FLAG_SETS:-}" ]]; then
    mapfile -t FLAG_LINES <<< "$FLAG_SETS"
else
    mapfile -t FLAG_LINES <<'FLAGS'
O0|-O0 -fopenmp
O1|-O1 -fopenmp
O2|-O2 -fopenmp
O3|-O3 -fopenmp
Ofast|-Ofast -fopenmp
FLAGS
fi


sanitize_tag() {
    local s=$1
    s=${s// /_}
    s=${s//\//_}
    s=${s//:/_}
    s=${s//,/__}
    s=${s//=/__}
    echo "$s"
}

for line in "${FLAG_LINES[@]}"; do
    [[ -z "$line" ]] && continue

    if [[ "$line" != *'|'* ]]; then
        echo "Skipping invalid FLAG_SETS entry (expected tag|flags): $line" >&2
        continue
    fi

    tag=${line%%|*}
    flags=${line#*|}
    safe_tag=$(sanitize_tag "$tag")
    BIN_NAME="${SRC_STEM}_${safe_tag}"
    BIN_PATH="$BUILD_DIR/${BIN_NAME}"
    OUT_CSV="${OUT_DIR}/${BIN_NAME}.csv"
    echo 'host,compiler,flags,run,threads,max_threads,elapsed_sec,status,binary' > "$OUT_CSV"

    echo "[build] $CC $flags -o $BIN_PATH $SRC"
    "$CC" $flags -o "$BIN_PATH" "$SRC" -lm

    export LC_ALL=C
    
    for ((threads=1; threads<=MAX_THREADS; threads++)); do
        for ((run=1; run<=RUNS; run++)); do
            tmp_time=$(mktemp)
            status="ok"
            elapsed="NA"
    
            if OMP_NUM_THREADS="$threads" /usr/bin/time -f "%e" -o "$tmp_time" \
                "$BIN_PATH" > /dev/null 2>&1; then
    
                elapsed=$(tr -d '[:space:]' < "$tmp_time")
    
                if [[ ! "$elapsed" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
                    status="bad_time_format"
                    elapsed="NA"
                elif ! awk "BEGIN {exit !($elapsed >= 0)}"; then
                    status="negative_time"
                    elapsed="NA"
                fi
            else
                status="run_failed"
            fi
    
            rm -f "$tmp_time"
    
            echo "\"$HOST\",\"$CC\",\"$flags\",$run,$threads,$MAX_THREADS,$elapsed,\"$status\",\"$BIN_NAME\"" >> "$OUT_CSV"
        done
    done

done

if [[ "$KEEP_BINARIES" == "0" ]]; then
    rm -f "$BUILD_DIR/${SRC_STEM}_"*
fi

echo "Done. Results saved"
