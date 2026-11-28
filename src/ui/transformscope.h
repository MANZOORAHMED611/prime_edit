#ifndef TRANSFORMSCOPE_H
#define TRANSFORMSCOPE_H

#include <QString>
#include <functional>

// Which text a Notepad++-style transform reads (Editor::applyTextTransform).
// Selection: exactly the selection, nothing when there is none.
// LinesOrDocument: the selected lines, or the whole document.
// LinesOrCurrentLine: the selected lines, or the line holding the cursor.
enum class TransformScope { Selection, LinesOrDocument, LinesOrCurrentLine };

// A pure text transform: text in, text out (core/texttransforms.h).
using TextTransform = std::function<QString(const QString &)>;

#endif // TRANSFORMSCOPE_H
