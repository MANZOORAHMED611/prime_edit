#include "demosetup.h"

#include "ILexer.h"
#include "Lexilla.h"
#include "SciLexer.h"
#include "Scintilla.h"

namespace demo {
namespace {

void defineFoldMarkers(ScintillaEdit *ed)
{
    const int markers[][2] = {
        {SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS}, {SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS},
        {SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE}, {SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER},
        {SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED},
        {SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED},
        {SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER}};
    for (const auto &m : markers) {
        ed->send(SCI_MARKERDEFINE, m[0], m[1]);
        ed->send(SCI_MARKERSETFORE, m[0], 0xFFFFFF);
        ed->send(SCI_MARKERSETBACK, m[0], 0x808080);
    }
    ed->send(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED);
    ed->send(SCI_SETAUTOMATICFOLD, SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CLICK
                                       | SC_AUTOMATICFOLD_CHANGE);
}

void configureMargins(ScintillaEdit *ed)
{
    ed->send(SCI_SETMARGINS, 4);
    ed->send(SCI_SETMARGINTYPEN, LineNumbers, SC_MARGIN_NUMBER);
    const sptr_t digitWidth = ed->send(SCI_TEXTWIDTH, STYLE_LINENUMBER,
                                       reinterpret_cast<sptr_t>("_99999999"));
    ed->send(SCI_SETMARGINWIDTHN, LineNumbers, digitWidth);

    // Change history markers (modified / saved / reverted) in their own margin.
    ed->send(SCI_SETMARGINTYPEN, History, SC_MARGIN_SYMBOL);
    ed->send(SCI_SETMARGINMASKN, History, SC_MASK_HISTORY);
    ed->send(SCI_SETMARGINWIDTHN, History, 6);

    // The ChangeTracker's own gutter marker, driven only by SCN_MODIFIED.
    ed->send(SCI_SETMARGINTYPEN, GitGutter, SC_MARGIN_SYMBOL);
    ed->send(SCI_SETMARGINMASKN, GitGutter, 1 << kGutterMarker);
    ed->send(SCI_SETMARGINWIDTHN, GitGutter, 5);
    ed->send(SCI_MARKERDEFINE, kGutterMarker, SC_MARK_FULLRECT);
    ed->send(SCI_MARKERSETBACK, kGutterMarker, 0x2B7A78);

    ed->send(SCI_SETMARGINTYPEN, Fold, SC_MARGIN_SYMBOL);
    ed->send(SCI_SETMARGINMASKN, Fold, SC_MASK_FOLDERS);
    ed->send(SCI_SETMARGINWIDTHN, Fold, 16);
    ed->send(SCI_SETMARGINSENSITIVEN, Fold, 1);
    defineFoldMarkers(ed);
}

void setStyle(ScintillaEdit *ed, int style, int fore, bool bold = false)
{
    ed->send(SCI_STYLESETFORE, style, fore);
    ed->send(SCI_STYLESETBOLD, style, bold ? 1 : 0);
}

void setLexer(ScintillaEdit *ed, const char *name)
{
    // Lexilla is linked statically: CreateLexer is called directly, no dlopen.
    Scintilla::ILexer5 *lexer = CreateLexer(name);
    ed->send(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
}

} // namespace

void configureEditor(ScintillaEdit *ed)
{
    ed->send(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("DejaVu Sans Mono"));
    ed->send(SCI_STYLESETSIZE, STYLE_DEFAULT, 11);
    ed->send(SCI_STYLECLEARALL);
    ed->send(SCI_SETCODEPAGE, SC_CP_UTF8);
    configureMargins(ed);

    ed->send(SCI_SETMULTIPLESELECTION, 1);
    ed->send(SCI_SETADDITIONALSELECTIONTYPING, 1);
    ed->send(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);
    ed->send(SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION);
    ed->send(SCI_SETIMEINTERACTION, SC_IME_INLINE);

    ed->send(SCI_INDICSETSTYLE, kSmartHighlightIndicator, INDIC_ROUNDBOX);
    ed->send(SCI_INDICSETFORE, kSmartHighlightIndicator, 0x00C000);
    ed->send(SCI_INDICSETALPHA, kSmartHighlightIndicator, 100);
    ed->send(SCI_INDICSETUNDER, kSmartHighlightIndicator, 1);
}

void applyCppLexer(ScintillaEdit *ed)
{
    setLexer(ed, "cpp");
    ed->send(SCI_SETKEYWORDS, 0, reinterpret_cast<sptr_t>(
        "int return if else for while class struct namespace void const auto bool"));
    ed->send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold"), reinterpret_cast<sptr_t>("1"));
    ed->send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold.compact"),
             reinterpret_cast<sptr_t>("0"));
    setStyle(ed, SCE_C_COMMENTLINE, 0x008000);
    setStyle(ed, SCE_C_NUMBER, 0x0080FF);
    setStyle(ed, SCE_C_WORD, 0x800000, true);
    setStyle(ed, SCE_C_STRING, 0x808080);
    setStyle(ed, SCE_C_PREPROCESSOR, 0x8000FF);
    ed->send(SCI_COLOURISE, 0, -1);
}

void applyHtmlLexer(ScintillaEdit *ed)
{
    setLexer(ed, "hypertext");
    ed->send(SCI_SETKEYWORDS, 0, reinterpret_cast<sptr_t>(
        "html head body style script p div class title"));
    ed->send(SCI_SETKEYWORDS, 1, reinterpret_cast<sptr_t>("function var let return if"));
    ed->send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold"), reinterpret_cast<sptr_t>("1"));
    ed->send(SCI_SETPROPERTY, reinterpret_cast<uptr_t>("fold.html"),
             reinterpret_cast<sptr_t>("1"));
    setStyle(ed, SCE_H_TAG, 0x800000, true);
    setStyle(ed, SCE_H_ATTRIBUTE, 0x0000FF);
    setStyle(ed, SCE_H_DOUBLESTRING, 0x808000);
    setStyle(ed, SCE_HJ_KEYWORD, 0xFF0000, true);
    setStyle(ed, SCE_HJ_NUMBER, 0x0080FF);
    setStyle(ed, SCE_HJ_DOUBLESTRING, 0x808080);
    setStyle(ed, SCE_HJ_WORD, 0x404040);
    ed->send(SCI_COLOURISE, 0, -1);
}

void applyNoLexer(ScintillaEdit *ed)
{
    ed->send(SCI_SETILEXER, 0, 0);
}

const char *sampleCpp()
{
    return "#include <cstdio>\n"
           "// Lexilla C++ lexer, fold margin, line numbers, multi-selection.\n"
           "namespace demo {\n"
           "class Counter {\n"
           "public:\n"
           "    int next() {\n"
           "        if (value < 10) {\n"
           "            return ++value;\n"
           "        }\n"
           "        return 0;\n"
           "    }\n"
           "private:\n"
           "    int value = 0;\n"
           "};\n"
           "} // namespace demo\n"
           "int main() { std::printf(\"%d\\n\", 42); return 0; }\n";
}

QByteArray bufferText(ScintillaEdit *ed)
{
    const sptr_t length = ed->send(SCI_GETLENGTH);
    QByteArray bytes(length + 1, Qt::Uninitialized);
    ed->send(SCI_GETTEXT, length + 1, reinterpret_cast<sptr_t>(bytes.data()));
    bytes.resize(length);
    return bytes;
}

} // namespace demo
