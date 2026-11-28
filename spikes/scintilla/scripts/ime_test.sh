#!/usr/bin/env bash
# Spike gate 2: IME composition into the demo, under a private D-Bus session
# and a private ibus-daemon (the user's own ibus is never touched).
#
# Usage: ime_test.sh OUTDIR DEMO_BIN PLATFORM [ENGINE]
#   PLATFORM  xcb | wayland
#   ENGINE    ibus engine name, default libpinyin
# Env for wayland: WL_ROOT = directory where the jammy qt6-wayland, weston and
#   libweston-9-0 .debs were unpacked (dpkg-deb -x). Weston runs nested on
#   $DISPLAY (x11 backend, pixman renderer, kiosk shell); xdotool keys go to
#   its window and Weston forwards them to the Wayland client.
# Types "nihao", screenshots the preedit, presses space to commit, screenshots
# again, then prints the IME events the widget received and the final buffer.
set -uo pipefail
OUT=$1; DEMO=$2; PLATFORM=$3; ENGINE=${4:-libpinyin}
HERE=$(cd "$(dirname "$0")" && pwd)

if [[ ${IME_INNER:-} != 1 ]]; then
    mkdir -p "$OUT/home/config" "$OUT/home/data" "$OUT/home/cache"
    RT=$(mktemp -d /tmp/pe-ime.XXXXXX); chmod 700 "$RT"
    env HOME="$OUT/home" XDG_CONFIG_HOME="$OUT/home/config" XDG_DATA_HOME="$OUT/home/data" \
        XDG_CACHE_HOME="$OUT/home/cache" XDG_RUNTIME_DIR="$RT" IME_INNER=1 \
        TMPDIR="$RT" GIO_USE_VFS=local GTK_USE_PORTAL=0 \
        dbus-run-session -- "$0" "$@"
    rc=$?
    # ibus names its abstract socket after $TMPDIR: a long TMPDIR is cut at the
    # 108-byte sun_path limit and clients cannot connect. Hence the short $RT.
    sleep 1
    rm -rf "$RT" 2>/dev/null || find "$RT" -depth -type d -empty -delete
    exit $rc
fi

shot() { xwd -root -silent -display "${XSHOT_DISPLAY:-$DISPLAY}" | convert xwd:- "$OUT/$1.png"; }

WESTON_PID=""
if [[ $PLATFORM == wayland ]]; then
    L=$WL_ROOT/usr/lib/x86_64-linux-gnu
    export LD_LIBRARY_PATH=$L:$L/weston${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
    export QT_PLUGIN_PATH=$L/qt6/plugins
    WESTON_MODULE_MAP="x11-backend.so=$L/libweston-9/x11-backend.so;kiosk-shell.so=$L/weston/kiosk-shell.so" \
        "$WL_ROOT/usr/bin/weston" --backend=x11-backend.so --shell=kiosk-shell.so \
        --use-pixman --socket=wl-spike --width=1200 --height=800 --no-config \
        > "$OUT/weston.log" 2>&1 &
    WESTON_PID=$!
    for _ in $(seq 1 100); do [[ -S $XDG_RUNTIME_DIR/wl-spike ]] && break; sleep 0.1; done
    export WAYLAND_DISPLAY=wl-spike
    echo "weston pid=$WESTON_PID socket=$(ls "$XDG_RUNTIME_DIR")"
fi

ibus-daemon --daemonize --panel disable --replace --address="unix:tmpdir=$XDG_RUNTIME_DIR"
for _ in $(seq 1 50); do ibus engine >/dev/null 2>&1 && break; sleep 0.2; done
ibus engine "$ENGINE"; echo "ibus engine now: $(ibus engine)"

QT_QPA_PLATFORM=$PLATFORM QT_IM_MODULE=ibus "$DEMO" --ime-log "$OUT/buffer.txt" \
    > "$OUT/demo.log" 2>&1 &
DEMO_PID=$!
sleep 3
if [[ $PLATFORM == xcb ]]; then
    WIN=$(xdotool search --onlyvisible --pid "$DEMO_PID" | tail -1)
    xdotool windowfocus --sync "$WIN"
else
    # Wayland: the compositor window on the X display receives the keys.
    # Weston sets no _NET_WM_PID, xdotool's name search misses its window and
    # there is no window manager: take the id from xwininfo, focus explicitly.
    WIN=$(xwininfo -root -children | grep "Weston Compositor" | awk '{print $1}')
    echo "weston X window=$WIN"
    xdotool windowfocus --sync "$WIN"
    xdotool mousemove --window "$WIN" 300 200 click 1
fi
sleep 0.5
xdotool key --delay 120 Home Return Up n i h a o
sleep 1.5
shot ime_preedit
xdotool key space
sleep 1.5
shot ime_committed

kill "$DEMO_PID"; wait "$DEMO_PID" 2>/dev/null
ibus exit >/dev/null 2>&1
[[ -n $WESTON_PID ]] && kill "$WESTON_PID" && wait "$WESTON_PID" 2>/dev/null
echo "--- IME events received by the widget:"
grep "^IME" "$OUT/demo.log" || echo "(none)"
echo "--- first line of buffer after commit:"
head -1 "$OUT/buffer.txt"; echo
echo "--- first line as code points:"
head -1 "$OUT/buffer.txt" | python3 -I -c "import sys; print(' '.join('U+%04X' % ord(c) for c in sys.stdin.read().strip()))"
grep -v "^IME\|^libEGL" "$OUT/demo.log" | head -5
