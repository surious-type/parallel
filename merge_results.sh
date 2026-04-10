${RUN_ID}#!/usr/bin/env bash
set -euo pipefail

PARTS_DIR="${1:-results/parts}"
OUT_FILE="${2:-results/benchmark_results.csv}"
TMP_FILE="$(mktemp)"

shopt -s nullglob
FILES=("$PARTS_DIR"/*.csv)

if [ ${#FILES[@]} -eq 0 ]; then
    echo "No partial CSV files found in $PARTS_DIR"
    exit 1
fi

# склеиваем все строки
for f in "${FILES[@]}"; do
    cat "$f" >>"$TMP_FILE"
done

# формируем итог
{
    echo "source,binary,compiler,opt,N,threads,time,host"
    sort -t, -k2,2 -k6,6n "$TMP_FILE"
} >"$OUT_FILE"

rm -f "$TMP_FILE"

echo "Merged into $OUT_FILE",
