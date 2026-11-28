#!/usr/bin/env bash
# Throwaway spike helper: download the pinned Scintilla and Lexilla tarballs,
# verify their sha256, extract them, and generate ScintillaEdit.{h,cpp}.
#
# Usage: fetch_sources.sh <download-dir> <source-dir>
#   <download-dir>  new, empty directory that receives the tarballs
#   <source-dir>    directory that receives scintilla/ and lexilla/
#
# The tarballs are untrusted data: they are checked against the pinned hashes
# before anything is extracted.
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <download-dir> <source-dir>" >&2
    exit 2
fi
DL_DIR=$1
SRC_DIR=$2

SCI_URL=https://www.scintilla.org/scintilla569.tgz
SCI_SHA=1622546ce1d3804b589292f1adae98ad1d87fa8c43bb401edc098adae12dcacb
LEX_URL=https://www.scintilla.org/lexilla555.tgz
LEX_SHA=fa4093ad5c48acf49a6f3eca1b75143a5c824ef1a6f5b0ee59e8c3d6a970f229

mkdir -p "$DL_DIR" "$SRC_DIR"
fetch() {
    local url=$1 sha=$2 out
    out="$DL_DIR/$(basename "$url")"
    [[ -f $out ]] || curl -sS -fL -m 120 -o "$out" "$url"
    echo "$sha  $out" | sha256sum -c -
    tar -xzf "$out" -C "$SRC_DIR"
}
fetch "$SCI_URL" "$SCI_SHA"
fetch "$LEX_URL" "$LEX_SHA"

# WidgetGen.py resolves its inputs relative to its own directory, so it has to
# run from there. -I keeps the current directory and user site-packages off
# sys.path; the script adds ../../scripts itself for Face.py/FileGenerator.py.
( cd "$SRC_DIR/scintilla/qt/ScintillaEdit" && python3 -I WidgetGen.py )
echo "scintilla $(cat "$SRC_DIR/scintilla/version.txt"), lexilla $(cat "$SRC_DIR/lexilla/version.txt")"
