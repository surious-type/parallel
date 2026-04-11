#!/usr/bin/env bash

DIR=${1:-tasks}

all=$(find "$DIR" -type f | sed -E 's/\.(lock|done|fail)$//' | sort -u | wc -l)
done=$(find "$DIR" -type f -name "*.done" | wc -l)
locked=$(find "$DIR" -type f -name "*.lock" | wc -l)
failed=$(find "$DIR" -type f -name "*.fail" | wc -l)

remaining=$((all - done - failed))

echo "=== STATS ==="
echo "ALL TASKS: $all"
echo "DONE:      $done"
echo "LOCKED:    $locked"
echo "FAILED:    $failed"
echo "REMAINING: $remaining"
