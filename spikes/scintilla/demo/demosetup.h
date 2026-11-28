// Spike: editor configuration shared by every demo mode.
#pragma once

#include "ScintillaEdit.h"

namespace demo {

// Margin layout used throughout the demo.
enum Margin { LineNumbers = 0, History = 1, GitGutter = 2, Fold = 3 };
// Marker number the ChangeTracker uses for its own "git gutter" proof.
constexpr int kGutterMarker = 10;
// Indicator slot used for smart highlight (Notepad++ uses 29-31; any free slot works).
constexpr int kSmartHighlightIndicator = 8;

// Monospace font, margins (line numbers, change history, gutter, fold),
// multiple selection with typing, rectangular virtual space, inline IME.
void configureEditor(ScintillaEdit *ed);
// Lexilla's C++ lexer with keywords, folding and a small colour scheme.
void applyCppLexer(ScintillaEdit *ed);
// Lexilla's hypertext lexer (HTML with embedded JS) with keywords and colours.
void applyHtmlLexer(ScintillaEdit *ed);
// Plain text, no lexer: what a large-file open uses.
void applyNoLexer(ScintillaEdit *ed);

// Short C++ sample with four nesting levels, used by the interactive demo.
const char *sampleCpp();

// Whole buffer as UTF-8 (a copy: test helper only, never on the save path).
QByteArray bufferText(ScintillaEdit *ed);

} // namespace demo
