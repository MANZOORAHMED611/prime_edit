# Backlog for Phases 2 and 3

Sources: the 2026-11-26 full review, the Phase 1 integration check (Abu Bakr) and the Phase 1 output gate (Umar).

## Phase 2: security, crashes and correctness
- **Git config code execution.** Opening a file inside a repo with a hostile `.git/config` (`diff.external`, `core.fsmonitor`) runs commands.
- Missing `--` in git calls.
- **Language servers start automatically, with no trust prompt.**
- Plugins that are disabled are still loaded.
- Remote files are downloaded to a predictable, world-readable path in /tmp.
- The LLM API key is stored in plaintext.
- The remote-file mapping is lost after a session restore.
- **Editing commands that corrupt text:**
  - Alt+Up duplicates the line;
  - Tab with a selection replaces the selection;
  - Shift+Tab unindents only the first line;
  - Replace All with an empty search string.
- Find Next and Find Previous ignore the search options.
- **Crashes:**
  - Evaluate Selection after its tab is closed;
  - LSP use-after-free when a tab closes during server init;
  - a Content-Length overflow in the LSP client hangs the GUI.
- **LSP client:**
  - 30 s GUI stalls on a blocked pipe;
  - the message framing desyncs;
  - URIs are not encoded;
  - stderr is never read and grows without limit.
- Menu commands (sort, duplicate, case) bypass the read-only key guard. Nothing reaches disk, because save is refused, but the tab is marked modified.
- The plugin API's `currentText()` returns `toPlainText()`, so a plugin's read-modify-write loses U+00A0. This is a public interface, so the change is high-risk. (integ m5)
- A forward that arrives during startup restore can be acknowledged and then dropped. (integ m4, fixed in T1.9)

## Phase 3: performance, features and cleanup
- **Length rule.** About 78 legacy functions are over 50 lines; split them. The Phase 1 grandfathering ruling lapses here. (gate a)
- **The Auto-save / recovery timer always runs** (`mainwindow.cpp:85`) and writes `recovery/*.recovery` files that hold user text and are never read. Remove the timer and the hidden preference. (gate f)
- When the socket path is too long, reword the "not responding" message. (gate g)
- **Large files.** Wire the memory-mapped `LargeFileReader` viewport (files over 1 GB). Reloading a partial view empties its display. (integ m3)
- **Memory and speed:**
  - the Document Map's hidden pixmap (420 MB for a 49 MB file);
  - highlighting runs synchronously over the whole document;
  - `toPlainText()` is called on every keystroke;
  - the text is held twice, in the PieceTable and the QTextDocument.
- A true shared-document Clone, using a per-document holder.
- Preserve mixed line endings per line, instead of normalising to the majority.
- Detection for ISO-8859-6 and windows-1256.
- Make D12's "no write permission" a read-only cause. Today Save refuses it. (integ m2)
- Remove the dead stub scaffold in `tests/regression/support.h`, and the dead `RecoveryDialog`.
- The spec's contract table is missing `canonicalName`, `detectCharset`, `showAction`/`showChoice` and `originalFilePath`. (integ m7)
- Unreachable UI: wire up or remove the file explorer, hex viewer, theme editor and git diff viewer.
- The `.deb` and AppImage are built by hand. Commit the packaging script.
