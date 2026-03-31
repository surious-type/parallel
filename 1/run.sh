#!/bin/bash

# Проверка аргументов
if [ $# -lt 1 ]; then
    echo "Usage: $0 <program> [runs]"
    exit 1
fi

PROGRAM=$1
RUNS=5

# Проверка, что файл существует и исполняемый
if [ ! -x "$PROGRAM" ]; then
    echo "Error: '$PROGRAM' not found or not executable"
    exit 1
fi

# Имя CSV = имя программы + .csv
BASENAME=$(basename "$PROGRAM")
OUTFILE="${BASENAME}.csv"

echo "run,time" > "$OUTFILE"

for ((i=1; i<=RUNS; i++))
do
    output=$($PROGRAM)

    time=$(/usr/bin/time -f "%e" "$PROGRAM" 2>&1 1>/dev/tty)

    # если вдруг не нашли время
    if [ -z "$time" ]; then
        echo "Error: cannot parse time from output"
        echo "$output"
        exit 1
    fi

    echo "$i,$time" >> "$OUTFILE"
done

echo "Done. Results saved to $OUTFILE"
