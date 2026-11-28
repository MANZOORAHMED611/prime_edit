// Spike gate 3: open a large file the way Notepad++ does (ILoader into a
// SC_DOCUMENTOPTION_TEXT_LARGE document) and time it.
#pragma once

#include <QString>

#include "ScintillaEdit.h"

namespace demo {

// Loads `path` through SCI_CREATELOADER. Returns false (and prints why) on error.
bool loadLargeFile(ScintillaEdit *ed, const QString &path);

// Loads, then prints "BENCH key=value" lines: time to loaded, to first paint
// (visible), to a confirmed edit (editable), and to the end of the document
// (SCI_DOCUMENTEND + paint), plus VmHWM. Times are measured from BENCH_T0_NS
// (CLOCK_REALTIME ns, set by the launcher just before exec) when present.
int runBench(ScintillaEdit *ed, const QString &path);

} // namespace demo
