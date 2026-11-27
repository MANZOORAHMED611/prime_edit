#!/usr/bin/env python3
"""Report C++ functions in src/ that a diff adds or modifies and that exceed 50 lines.

Usage: python3 tools/check_function_length.py --base <rev>

The diff is <rev> against the working tree, plus untracked files under src/.
A function counts as added or modified when any added or changed line falls
inside it. Its length runs from the first line of its signature to the line of
its closing brace, inclusive; comments and blank lines inside it count.

Exit codes: 0 no violations, 1 violations reported, 2 usage or git error.

This is a heuristic scanner, not a C++ parser (no length tool is installed:
lizard and clang-tidy are absent). It strips comments, string and character
literals and preprocessor lines, then classifies each top-level or class-level
brace block as a scope (namespace, class, struct, union), an enum or
initializer (skipped), or a function body. Lambdas and local classes count as
part of the function that contains them.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

MAX_LINES = 50
CPP_SUFFIXES = {".cpp", ".cc", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
SOURCE_DIR = "src"

ACCESS_RE = re.compile(
    r"^\s*(?:(?:public|protected|private)(?:\s+(?:slots|Q_SLOTS))?|signals|Q_SIGNALS)\s*:(?!:)"
)
# Object-like macros that end without a semicolon (Q_OBJECT) must not be
# counted as the start of the next member's signature.
LEADING_MACRO_RE = re.compile(r"^\s*Q_[A-Z_]+\b(?!\s*\()")
TEMPLATE_RE = re.compile(r"^\s*template\s*<")
SCOPE_RE = re.compile(r'^\s*(?:inline\s+)?namespace\b|^\s*extern\s*"\s*"')
CLASS_RE = re.compile(r"^\s*(?:class|struct|union)\b")
ENUM_RE = re.compile(r"^\s*enum\b")
OPERATOR_RE = re.compile(r"\boperator\s*(?:\(\s*\)|[^\w\s(][^(]*|\w[\w\s:<>*&]*?)(?=\()")
NAME_RE = re.compile(r"(~?[A-Za-z_]\w*(?:\s*::\s*~?[A-Za-z_]\w*)*|operator\S*)\s*(?:<[^()]*>)?\s*$")


@dataclass
class Function:
    name: str
    start: int  # 1-based line of the signature's first character
    end: int    # 1-based line of the closing brace

    @property
    def length(self) -> int:
        return self.end - self.start + 1


RAW_STRING_RE = re.compile(r'R"([^(\s]*)\(')


def _end_of(text: str, needle: str, start: int, include: bool) -> int:
    j = text.find(needle, start)
    if j < 0:
        return len(text)
    return j + len(needle) if include else j


def _span_at(text: str, i: int) -> tuple[int, int, int] | None:
    """Return (blank_from, blank_to, resume_at) for a comment or literal at i, else None.

    Literal quotes are kept and only their contents blanked, so `extern "C"` stays
    recognisable as `extern " "`.
    """
    c = text[i]
    if text.startswith("//", i):
        end = _end_of(text, "\n", i, include=False)
        return i, end, end
    if text.startswith("/*", i):
        end = _end_of(text, "*/", i + 2, include=True)
        return i, end, end
    if c == "R" and not _ident_char(text, i - 1):
        m = RAW_STRING_RE.match(text, i)
        if m:
            end = _end_of(text, ")" + m.group(1) + '"', m.end(), include=True)
            return i + 1, end - 1, end
        return None
    if c == '"' or (c == "'" and not (i > 0 and text[i - 1].isdigit())):
        j = i + 1
        while j < len(text) and text[j] != c and text[j] != "\n":
            j += 2 if text[j] == "\\" else 1
        return i + 1, j, min(j + 1, len(text))
    return None


def _end_of_directive(text: str, i: int) -> int:
    """Index of the newline ending a preprocessor directive, following backslash continuations."""
    j = i
    while j < len(text) and text[j] != "\n":
        j = j + 2 if text[j] == "\\" else j + 1
    return min(j, len(text))


def strip_code(text: str) -> str:
    """Blank out comments, literals and preprocessor lines, keeping newlines and offsets."""
    out = list(text)

    def blank(a: int, b: int) -> None:
        for k in range(a, min(b, len(text))):
            if out[k] != "\n":
                out[k] = " "

    i, at_line_start = 0, True
    while i < len(text):
        c = text[i]
        if c in " \t\n":
            at_line_start = at_line_start or c == "\n"
            i += 1
            continue
        if at_line_start and c == "#":
            end = _end_of_directive(text, i)
            blank(i, end)
            i = end
            continue
        at_line_start = False
        span = _span_at(text, i)
        if span is None:
            i += 1
            continue
        blank(span[0], span[1])
        i = span[2]
    return "".join(out)


def _ident_char(text: str, i: int) -> bool:
    return i >= 0 and (text[i].isalnum() or text[i] == "_")


def _paren_depth_scan(sig: str):
    """Yield (index, char, depth) for sig, depth counted over () and []."""
    depth = 0
    for k, ch in enumerate(sig):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        yield k, ch, depth


def _last_top_paren(sig: str) -> int:
    last = -1
    for k, ch, depth in _paren_depth_scan(sig):
        if ch == ")" and depth == 0:
            last = k
    return last


def _first_top_paren(sig: str) -> int:
    for k, ch, depth in _paren_depth_scan(sig):
        if ch == "(" and depth == 1:
            return k
    return -1


def _has_top_level_assign(sig: str) -> bool:
    cleaned = OPERATOR_RE.sub("operator_", sig)
    for k, ch, depth in _paren_depth_scan(cleaned):
        if depth == 0 and ch == "=" and cleaned[k - 1:k] not in ("=", "!", "<", ">") \
                and cleaned[k + 1:k + 2] != "=":
            return True
    return False


def _in_init_list(sig: str) -> bool:
    """True when sig ends inside a constructor initializer list: `X() : a(1), b`."""
    close = _last_top_paren(sig)
    if close < 0:
        return False
    tail = sig[close + 1:]
    return bool(re.match(r"\s*(?:noexcept\s*)?:(?!:)", tail))


def classify(sig: str) -> str:
    """Return 'scope', 'function', or 'other' for the text before a `{`."""
    if SCOPE_RE.search(sig):
        return "scope"
    body = re.sub(r"^\s*template\s*<.*?>\s*", "", sig, flags=re.S) if TEMPLATE_RE.match(sig) else sig
    if ENUM_RE.match(body):
        return "other"
    if CLASS_RE.match(body) and _first_top_paren(body) < 0:
        return "scope"
    if _has_top_level_assign(body) or _last_top_paren(body) < 0:
        return "other"
    tail = body[_last_top_paren(body) + 1:]
    if _in_init_list(body):
        return "function"
    qualifiers = r"(?:\s*(?:const|volatile|override|final|noexcept(?:\s*\([^)]*\))?|&&?|->[^{;]*|"
    qualifiers += r"Q_DECL_\w+|try))*\s*$"
    if not re.fullmatch(qualifiers, tail):
        return "other"
    head = body[:_first_top_paren(body)]
    if re.search(r"\b(?:if|for|while|switch|catch|return|sizeof|decltype)\s*$", head):
        return "other"
    return "function"


def function_name(sig: str) -> str:
    head = sig[:_first_top_paren(sig)] if _first_top_paren(sig) >= 0 else sig
    m = NAME_RE.search(head.rstrip())
    return re.sub(r"\s+", "", m.group(1)) if m else "<unknown>"


def _trim_signature(code: str, start: int, end: int) -> int:
    """Advance start past whitespace, access specifiers and bare Qt macros."""
    while True:
        seg = code[start:end]
        m = ACCESS_RE.match(seg) or LEADING_MACRO_RE.match(seg)
        if not m:
            break
        start += m.end()
    while start < end and code[start].isspace():
        start += 1
    return start


def find_functions(source: str) -> list[Function]:
    code = strip_code(source)
    line_of = _line_index(code)
    functions: list[Function] = []
    stack: list[tuple[str, int, int]] = []  # (kind, signature start, open index)
    sig_start = 0
    init_brace_depth = 0
    for i, ch in enumerate(code):
        in_body = any(kind in ("function", "other") for kind, _, _ in stack)
        if ch == "{":
            if in_body:
                stack.append(("nested", 0, i))
                continue
            start = _trim_signature(code, sig_start, i)
            sig = code[start:i]
            if _in_init_list(sig) and re.search(r"[\w>]\s*$", sig):
                stack.append(("initbrace", start, i))  # member brace-init: m_x{0}
                init_brace_depth += 1
                continue
            stack.append((classify(sig), start, i))
            sig_start = i + 1
        elif ch == "}":
            if not stack:
                sig_start = i + 1
                continue
            kind, start, _ = stack.pop()
            if kind == "initbrace":
                init_brace_depth -= 1
                continue  # the signature continues after a member brace-init
            if kind == "function" and not any(k in ("function", "other") for k, _, _ in stack):
                functions.append(Function(function_name(code[start:].split("{", 1)[0]),
                                          line_of(start), line_of(i)))
            if not any(k in ("function", "other", "nested") for k, _, _ in stack):
                sig_start = i + 1
        elif ch == ";" and not in_body and init_brace_depth == 0:
            sig_start = i + 1
    return functions


def _line_index(code: str):
    starts = [0] + [k + 1 for k, ch in enumerate(code) if ch == "\n"]

    def line_of(offset: int) -> int:
        lo, hi = 0, len(starts) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if starts[mid] <= offset:
                lo = mid
            else:
                hi = mid - 1
        return lo + 1

    return line_of


def git(root: Path, *args: str) -> str:
    result = subprocess.run(["git", *args], cwd=root, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)} failed: {result.stderr.strip()}")
    return result.stdout


def changed_lines(root: Path, base: str) -> dict[str, set[int] | None]:
    """Map each changed C++ file under src/ to its changed new-side lines (None = whole file)."""
    changes: dict[str, set[int] | None] = {}
    current = None
    diff = git(root, "diff", "--unified=0", "--no-color", "--no-ext-diff", base, "--", SOURCE_DIR)
    for line in diff.splitlines():
        if line.startswith("+++ "):
            path = line[4:]
            current = path[2:] if path.startswith("b/") else None
            if current and Path(current).suffix in CPP_SUFFIXES:
                changes.setdefault(current, set())
            else:
                current = None
        elif line.startswith("@@") and current:
            m = re.match(r"@@ -\S+ \+(\d+)(?:,(\d+))? @@", line)
            first, count = int(m.group(1)), int(m.group(2) or "1")
            # A pure deletion (count 0) modifies the code around line `first`.
            span = range(first, first + count) if count else range(first, first + 2)
            changes[current].update(span)
    untracked = git(root, "ls-files", "--others", "--exclude-standard", "--", SOURCE_DIR)
    for path in untracked.splitlines():
        if Path(path).suffix in CPP_SUFFIXES:
            changes[path] = None
    return changes


def base_lengths(root: Path, base: str, path: str) -> dict[str, int]:
    """Length of each function in `path` at `base`, by name (empty if the file is new)."""
    try:
        source = git(root, "show", f"{base}:{path}")
    except RuntimeError:
        return {}
    lengths: dict[str, int] = {}
    for fn in find_functions(source):
        lengths[fn.name] = max(fn.length, lengths.get(fn.name, 0))
    return lengths


def violations(root: Path, base: str) -> list[tuple[str, Function]]:
    # Grandfather rule (Phase 1 ruling): about 78 functions were already over
    # the limit at the base. Editing one is allowed if it does not grow;
    # splitting them is a separate refactor. New functions get no allowance.
    found = []
    for path, lines in sorted(changed_lines(root, base).items()):
        file = root / path
        if not file.is_file():
            continue
        before = base_lengths(root, base, path)
        for fn in find_functions(file.read_text(encoding="utf-8", errors="replace")):
            touched = lines is None or any(fn.start <= ln <= fn.end for ln in lines)
            if not touched or fn.length <= MAX_LINES:
                continue
            if fn.name in before and fn.length <= before[fn.name] and before[fn.name] > MAX_LINES:
                continue
            found.append((path, fn))
    return found


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=f"Report C++ functions in {SOURCE_DIR}/ added or modified since --base "
                    f"that are longer than {MAX_LINES} lines.")
    parser.add_argument("--base", required=True, help="git revision to diff the working tree against")
    args = parser.parse_args(argv)
    try:
        root = Path(git(Path.cwd(), "rev-parse", "--show-toplevel").strip())
        try:
            git(root, "rev-parse", "--verify", "--quiet", args.base + "^{commit}")
        except RuntimeError:
            raise RuntimeError(f"--base {args.base!r} is not a commit in {root}") from None
        found = violations(root, args.base)
    except RuntimeError as err:
        print(f"check_function_length: {err}", file=sys.stderr)
        return 2
    for path, fn in found:
        print(f"{path}:{fn.start}-{fn.end}: {fn.name} is {fn.length} lines (max {MAX_LINES}; a function already over the limit at --base may be edited only if it does not grow)")
    if found:
        print(f"{len(found)} function(s) over {MAX_LINES} lines", file=sys.stderr)
        return 1
    print(f"OK: no added or modified function in {SOURCE_DIR}/ exceeds {MAX_LINES} lines")
    return 0


if __name__ == "__main__":
    sys.exit(main())
