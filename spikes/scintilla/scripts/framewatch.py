#!/usr/bin/env python3
"""Spike helper: watch an X display until it stops changing.

Grabs the root window with xwd in a loop and counts changed pixels between
consecutive frames. A frame "changes" when more than --threshold pixels differ
(this ignores the blinking caret). Reports, in ms since --t0-ns, the first and
the last significant change once the screen has been quiet for --quiet-s.
The last change is the "settled" time used as time-to-visible / time-to-scroll.

Usage: framewatch.py --display :72 --t0-ns N [--quiet-s 4] [--threshold 200]
                     [--timeout-s 600] [--out frame.png] [-- CMD ...]
With CMD, t0 is reset to the moment CMD starts (after the baseline grab).
"""
import argparse
import json
import struct
import subprocess
import time

import numpy as np


def grab(display):
    raw = subprocess.run(["xwd", "-root", "-silent", "-display", display],
                         capture_output=True, check=True).stdout
    width, height = struct.unpack(">II", raw[16:24])
    bytes_per_line = struct.unpack(">I", raw[48:52])[0]
    pixels = np.frombuffer(raw[-height * bytes_per_line:], dtype=np.uint32)
    return raw, pixels


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--display", default=":72")
    ap.add_argument("--t0-ns", type=int, required=True)
    ap.add_argument("--quiet-s", type=float, default=4.0)
    ap.add_argument("--threshold", type=int, default=200)
    ap.add_argument("--timeout-s", type=float, default=600.0)
    ap.add_argument("--out")
    ap.add_argument("trigger", nargs=argparse.REMAINDER,
                    help="-- CMD ...: command run right after the baseline grab")
    args = ap.parse_args()

    ms = lambda: (time.time_ns() - args.t0_ns) / 1e6
    raw, prev = grab(args.display)
    if args.trigger and args.trigger[0] == "--":
        args.trigger = args.trigger[1:]
    if args.trigger:
        # Baseline first, then the action: a 4 ms scroll must not land before it.
        args.t0_ns = time.time_ns()
        subprocess.run(args.trigger, check=True)
    watch_start = ms()
    first = last = None
    changes = []
    polls = 1
    while True:
        raw, cur = grab(args.display)
        polls += 1
        changed = int(np.count_nonzero(cur != prev)) if cur.shape == prev.shape else cur.size
        now = ms()
        if changed > args.threshold:
            last = now
            first = first if first is not None else now
            changes.append([round(now), changed])
        prev = cur
        quiet_since = last if last is not None else watch_start
        if now - quiet_since >= args.quiet_s * 1000 and (last is not None or now >= args.quiet_s * 1000):
            break  # quiet after a change, or no change at all within quiet_s
        if now >= args.timeout_s * 1000:
            break
    if args.out:
        subprocess.run(["convert", "xwd:-", args.out], input=raw, check=True)
    print(json.dumps({
        "first_change_ms": None if first is None else round(first),
        "settled_ms": None if last is None else round(last),
        "timed_out": ms() >= args.timeout_s * 1000,
        "polls": polls,
        "changes": changes[:40],
    }))


if __name__ == "__main__":
    main()
