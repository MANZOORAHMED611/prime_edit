// Spike gates 4 and 5: programmatic checks with printed evidence and a
// screenshot per Notepad++ parity primitive.
#pragma once

#include <QString>

#include "ScintillaEdit.h"

namespace demo {

// Runs every check against `ed`, prints "CHECK <name> PASS|FAIL <evidence>"
// lines to stdout and writes <shotDir>/<name>.png. Returns the failure count.
int runSelfTest(ScintillaEdit *ed, const QString &shotDir, const QString &scratchDir);

} // namespace demo
