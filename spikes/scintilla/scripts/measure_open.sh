#!/usr/bin/env bash
# Spike helper: open FILE in an editor on an X display and measure, from the
# outside, the same three things for every editor:
#   open   - time until the screen settles after launch (time to visible)
#   end    - time until the screen settles after Ctrl+End (scroll to end)
#   typed  - whether typing one character changes the screen (editable)
# plus peak RSS from /usr/bin/time -v. The app runs with HOME and every XDG
# directory redirected to OUTDIR, and only the PID started here is killed.
#
# Usage: measure_open.sh OUTDIR FILE -- COMMAND [ARGS...]
#   (FILE is appended as the last argument of COMMAND)
# Env:   DISPLAY (default :72), QUIET_S (default 4), TIMEOUT_S (default 600)
set -euo pipefail

OUT=$1; FILE=$2; shift 3
HERE=$(cd "$(dirname "$0")" && pwd)
export DISPLAY=${DISPLAY:-:72}
QUIET_S=${QUIET_S:-4}
TIMEOUT_S=${TIMEOUT_S:-600}

mkdir -p "$OUT/home/config" "$OUT/home/data" "$OUT/home/cache"
RT=$(mktemp -d /tmp/pe-rt.XXXXXX)   # short path for sockets
chmod 700 "$RT"

watch() {  # watch <t0-ns> <name> <quiet-s> [trigger command...]
    local t0=$1 name=$2 quiet=$3
    shift 3
    python3 -I "$HERE/framewatch.py" --display "$DISPLAY" --t0-ns "$t0" \
        --quiet-s "$quiet" --timeout-s "$TIMEOUT_S" --out "$OUT/$name.png" \
        ${1:+-- "$@"} > "$OUT/$name.json"
    echo "$name: $(cut -c1-200 "$OUT/$name.json")"
}

T0=$(date +%s%N)
env HOME="$OUT/home" XDG_CONFIG_HOME="$OUT/home/config" XDG_DATA_HOME="$OUT/home/data" \
    XDG_CACHE_HOME="$OUT/home/cache" XDG_RUNTIME_DIR="$RT" BENCH_T0_NS="$T0" \
    /usr/bin/time -v -o "$OUT/time.txt" "$@" "$FILE" > "$OUT/app.log" 2>&1 &
TIME_PID=$!
APP_PID=""
for _ in $(seq 1 100); do
    APP_PID=$(pgrep -P "$TIME_PID" || true)
    [[ -n $APP_PID ]] && break
    sleep 0.02
done
echo "app pid=$APP_PID"
watch "$T0" open "$QUIET_S"

WIN=$(xdotool search --onlyvisible --pid "$APP_PID" | tail -1)
xdotool windowfocus --sync "$WIN" || true
watch 0 end "$QUIET_S" xdotool key --clearmodifiers ctrl+End
watch 0 typed 2 xdotool type --delay 0 Z

kill -TERM "$APP_PID" 2>/dev/null || true
wait "$TIME_PID" 2>/dev/null || true
rm -rf "$RT"
grep -E "Maximum resident set size|Elapsed \(wall clock\)|Exit status|terminated" "$OUT/time.txt"
