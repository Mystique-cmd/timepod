#!/usr/bin/env bash
set -euo pipefail
./timepod &
PID=$!
sleep 1
# just quit
printf 'q' > /proc/$PID/fd/0 2>/dev/null || true
sleep 0.2
kill $PID 2>/dev/null || true
ls -la timepod_days.bin || true

