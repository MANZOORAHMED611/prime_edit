#!/usr/bin/env bash
# Spike helper: run measure_open.sh for both editors on the three gate-3 files.
# Usage: run_matrix.sh RESULTS_DIR DEMO_BIN PRIMEEDIT_BIN FILE_100MB FILE_500MB FILE_JSON
set -uo pipefail
R=$1; DEMO=$2; PE=$3; F100=$4; F500=$5; FJSON=$6
HERE=$(cd "$(dirname "$0")" && pwd)
export DISPLAY=${DISPLAY:-:72}

run() {  # run <label> <file> <quiet-s> <command...>
    local label=$1 file=$2 quiet=$3
    shift 3
    echo "== $label"
    QUIET_S=$quiet "$HERE/measure_open.sh" "$R/ext_$label" "$file" -- "$@" 2>&1 | tail -7
}

run demo_100mb "$F100" 4 "$DEMO" --open
run pe_100mb "$F100" 4 "$PE"
run demo_500mb "$F500" 4 "$DEMO" --open
run pe_500mb "$F500" 4 "$PE"
run pe_json "$FJSON" 8 "$PE"
# Scintilla freezes during the 97 MB single-line layout: a short quiet window
# would mistake the frozen screen for "settled".
run demo_json "$FJSON" 70 "$DEMO" --open
