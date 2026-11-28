#include "selftest.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QKeyEvent>
#include <QPixmap>

#include <cstdio>

#include "SciLexer.h"
#include "Scintilla.h"
#include "changetracker.h"
#include "demosetup.h"
#include "toysave.h"

namespace demo {
namespace {

struct Ctx {
    ScintillaEdit *ed;
    QString shots;
    QString scratch;
    int failures = 0;
};

void report(Ctx &c, const char *name, bool ok, const QString &evidence)
{
    std::printf("CHECK %-20s %s %s\n", name, ok ? "PASS" : "FAIL", qPrintable(evidence));
    std::fflush(stdout);
    if (!ok)
        ++c.failures;
}

void info(const QString &line)
{
    std::printf("    %s\n", qPrintable(line));
    std::fflush(stdout);
}

void shot(Ctx &c, const char *name)
{
    QCoreApplication::processEvents();
    c.ed->grab().save(c.shots + "/" + name + ".png");
}

void setText(ScintillaEdit *ed, const char *text)
{
    ed->send(SCI_SETCHANGEHISTORY, SC_CHANGE_HISTORY_DISABLED);
    ed->send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(text));
    ed->send(SCI_EMPTYUNDOBUFFER);
    ed->send(SCI_SETSAVEPOINT);
    ed->send(SCI_MARKERDELETEALL, -1);
}

// Text input through ScintillaEditBase::keyPressEvent, the real typing path.
// (Modifier chords cannot be simulated this way: keyPressEvent reads the
// global QApplication::keyboardModifiers(), not the event's modifiers.)
void typeText(ScintillaEdit *ed, const QString &text)
{
    QKeyEvent press(QEvent::KeyPress, 0, Qt::NoModifier, text);
    QCoreApplication::sendEvent(ed, &press);
    QKeyEvent release(QEvent::KeyRelease, 0, Qt::NoModifier, text);
    QCoreApplication::sendEvent(ed, &release);
}

QString quoted(const QByteArray &bytes)
{
    QString s = QString::fromUtf8(bytes);
    return "\"" + s.replace('\n', "\\n") + "\"";
}

void checkLspDidChange(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    setText(ed, "alpha\nbeta gamma\ndelta\n");
    ChangeTracker tracker(ed);
    ed->send(SCI_INSERTTEXT, 6, reinterpret_cast<sptr_t>("NEW "));      // insert
    ed->send(SCI_DELETERANGE, 2, 10);                                     // delete across lines
    ed->send(SCI_SETTARGETRANGE, 0, 3);
    ed->send(SCI_REPLACETARGET, -1, reinterpret_cast<sptr_t>("x\ny\nz")); // multi-line replace
    ed->send(SCI_DOCUMENTEND);
    typeText(ed, QString::fromUtf8("\xF0\x9F\x98\x80 \xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7"));
    const sptr_t afterEmoji = ed->send(SCI_POSITIONFROMLINE, ed->send(SCI_LINEFROMPOSITION,
                                       ed->send(SCI_GETCURRENTPOS))) + 4;
    ed->send(SCI_INSERTTEXT, afterEmoji, reinterpret_cast<sptr_t>("|")); // UTF-16 col 2, byte 4
    ed->send(SCI_SETSELECTION, 1, 1);
    ed->send(SCI_ADDSELECTION, 3, 3);
    typeText(ed, "##");                                                   // multi-cursor typing
    ed->send(SCI_UNDO);
    ed->send(SCI_UNDO);
    ed->send(SCI_REDO);
    ed->send(SCI_SETSAVEPOINT);
    ed->send(SCI_INSERTTEXT, 0, reinterpret_cast<sptr_t>("Q"));
    ed->send(SCI_UNDO);
    const bool match = tracker.shadowMatchesBuffer();
    report(c, "lsp_incremental", match && tracker.changes().size() >= 10,
           QString("%1 SCN_MODIFIED-derived changes replayed into a shadow copy; "
                   "shadow == buffer: %2").arg(tracker.changes().size()).arg(match));
    for (const LspChange &ch : tracker.changes().mid(0, 7))
        info("didChange " + ch.toJson());
    QList<int> lines = tracker.gutterLines().values();
    std::sort(lines.begin(), lines.end());
    QStringList ls;
    for (int l : lines)
        ls << QString::number(l);
    report(c, "git_gutter_lines", !lines.isEmpty(),
           "gutter marker lines from SCN_MODIFIED: {" + ls.join(",") + "}");
    report(c, "dirty_flag", tracker.dirtyLog().size() >= 4 && !tracker.dirty(),
           "save-point notifications: " + tracker.dirtyLog().join(" -> "));
}

QByteArray sha256OfFile(const QString &path)
{
    QFile f(path);
    QCryptographicHash h(QCryptographicHash::Sha256);
    if (f.open(QIODevice::ReadOnly))
        h.addData(&f);
    return h.result().toHex().left(16);
}

QByteArray sha256OfBuffer(ScintillaEdit *ed)
{
    const char *p = reinterpret_cast<const char *>(ed->send(SCI_GETCHARACTERPOINTER));
    const QByteArray view = QByteArray::fromRawData(p, ed->send(SCI_GETLENGTH));
    return QCryptographicHash::hash(view, QCryptographicHash::Sha256).toHex().left(16);
}

void fillLargeDocument(ScintillaEdit *ed, int megabytes)
{
    QByteArray block;
    for (int i = 0; block.size() < 1024 * 1024; ++i)
        block += "Line " + QByteArray::number(i) + ": the quick brown fox jumps over the dog.\n";
    for (int i = 0; i < megabytes; ++i)
        ed->send(SCI_APPENDTEXT, block.size(), reinterpret_cast<sptr_t>(block.constData()));
    ed->send(SCI_EMPTYUNDOBUFFER);
}

QString describe(const ToySaveReport &r)
{
    return QString("ok=%1 bytes=%2 VmRSS %3->%4 kB (delta %5 kB), VmHWM %6->%7 kB, "
                   "pointer %8 ms, write+fdatasync+commit %9 ms %10")
        .arg(r.ok).arg(r.bytes).arg(r.rssBeforeKb).arg(r.rssAfterKb)
        .arg(r.rssAfterKb - r.rssBeforeKb).arg(r.hwmBeforeKb).arg(r.hwmAfterKb)
        .arg(r.pointerMs, 0, 'f', 2).arg(r.writeMs, 0, 'f', 1).arg(r.error);
}

void checkToySave(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    const sptr_t original = ed->send(SCI_GETDOCPOINTER);
    ed->send(SCI_ADDREFDOCUMENT, 0, original);
    const sptr_t big = ed->send(SCI_CREATEDOCUMENT, 0, SC_DOCUMENTOPTION_TEXT_LARGE);
    ed->send(SCI_SETDOCPOINTER, 0, big);
    fillLargeDocument(ed, 256);
    ed->send(SCI_INSERTTEXT, ed->send(SCI_GETLENGTH) / 2, reinterpret_cast<sptr_t>("EDIT"));

    const QString wholePath = c.scratch + "/toy_whole.txt";
    const ToySaveReport whole = toySaveWhole(ed, wholePath);
    const bool wholeSame = sha256OfFile(wholePath) == sha256OfBuffer(ed);
    report(c, "toysave_whole", whole.ok && wholeSame,
           describe(whole) + QString("; file sha256 == buffer sha256: %1").arg(wholeSame));

    ed->send(SCI_INSERTTEXT, ed->send(SCI_GETLENGTH) / 3, reinterpret_cast<sptr_t>("GAP"));
    const QString chunkPath = c.scratch + "/toy_chunked.txt";
    const ToySaveReport chunked = toySaveChunked(ed, chunkPath, 4 * 1024 * 1024);
    const bool chunkSame = sha256OfFile(chunkPath) == sha256OfBuffer(ed);
    report(c, "toysave_chunked", chunked.ok && chunkSame,
           describe(chunked) + QString("; file sha256 == buffer sha256: %1").arg(chunkSame));

    // Contrast: what a full copy (today's QString text() path) costs.
    const qint64 rssBefore = procStatusKb("VmRSS:");
    QByteArray copy(ed->send(SCI_GETLENGTH) + 1, Qt::Uninitialized);
    ed->send(SCI_GETTEXT, copy.size(), reinterpret_cast<sptr_t>(copy.data()));
    info(QString("contrast: SCI_GETTEXT full copy moves VmRSS by %1 kB")
             .arg(procStatusKb("VmRSS:") - rssBefore));
    copy.clear();

    ed->send(SCI_SETDOCPOINTER, 0, original);
    ed->send(SCI_RELEASEDOCUMENT, 0, original);
    ed->send(SCI_RELEASEDOCUMENT, 0, big);
    QFile::remove(wholePath);
    QFile::remove(chunkPath);
}

void checkUndoGrouping(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    const char *original = "one\ntwo\nthree\nfour\nfive\n";
    auto commentAll = [ed]() {
        for (int line = 0; line < 5; ++line)
            ed->send(SCI_INSERTTEXT, ed->send(SCI_POSITIONFROMLINE, line),
                     reinterpret_cast<sptr_t>("// "));
    };
    auto undosToRestore = [ed, original]() {
        int n = 0;
        while (ed->send(SCI_CANUNDO) && ++n < 100) {
            ed->send(SCI_UNDO);
            if (bufferText(ed) == original)
                break;
        }
        return n;
    };
    setText(ed, original);
    ed->send(SCI_BEGINUNDOACTION);
    commentAll();
    ed->send(SCI_ENDUNDOACTION);
    const QByteArray transformed = bufferText(ed);
    const int grouped = undosToRestore();
    const bool restored = bufferText(ed) == original && !ed->send(SCI_CANUNDO);
    setText(ed, original);
    commentAll();
    const int ungrouped = undosToRestore();
    report(c, "undo_grouping", grouped == 1 && restored,
           QString("5-line comment transform: grouped needs %1 undo (canUndo after: %2), "
                   "ungrouped needs %3; transformed=%4")
               .arg(grouped).arg(ed->send(SCI_CANUNDO) ? "n/a" : "false").arg(ungrouped)
               .arg(quoted(transformed.left(20))));
}

void checkRectangularVirtual(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    // Two real configurations. Vertical caret moves keep virtual space only
    // with SCVS_USERACCESSIBLE (Editor::PositionUpOrDown uses UserVirtualSpace()),
    // so with rectangle-only virtual space (Notepad++'s default) the column is
    // widened into virtual space after moving down, not before.
    struct Case {
        const char *name;
        int options;
        bool downFirst;
        QByteArray expected;
        bool gate; // false: recorded as INFO (observed behaviour, not a gate)
    };
    const Case cases[] = {
        {"rect_virtual_default", SCVS_RECTANGULARSELECTION, true,
         "alpha|\na muc|longer line\nbeta |\n", false},
        {"rect_virtual_user", SCVS_RECTANGULARSELECTION | SCVS_USERACCESSIBLE, false,
         "alpha|\na muc|onger line\nbeta |\n", true},
    };
    for (const Case &k : cases) {
        ed->send(SCI_SETVIRTUALSPACEOPTIONS, k.options);
        setText(ed, "alpha\na much longer line\nbeta\n");
        ed->send(SCI_GOTOPOS, 5);
        // The commands bound to Alt+Shift+Down / Alt+Shift+Right in the keymap.
        auto down = [ed] { for (int i = 0; i < 2; ++i) ed->send(SCI_LINEDOWNRECTEXTEND); };
        auto right = [ed] { for (int i = 0; i < 3; ++i) ed->send(SCI_CHARRIGHTRECTEXTEND); };
        k.downFirst ? (down(), right()) : (right(), down());
        const QString sel = QString("rect anchor=%1+%2v caret=%3+%4v mode=%5")
            .arg(ed->send(SCI_GETRECTANGULARSELECTIONANCHOR))
            .arg(ed->send(SCI_GETRECTANGULARSELECTIONANCHORVIRTUALSPACE))
            .arg(ed->send(SCI_GETRECTANGULARSELECTIONCARET))
            .arg(ed->send(SCI_GETRECTANGULARSELECTIONCARETVIRTUALSPACE))
            .arg(ed->send(SCI_GETSELECTIONMODE));
        const bool rectangular = ed->send(SCI_GETSELECTIONMODE) == SC_SEL_RECTANGLE
                                 && ed->send(SCI_GETRECTANGULARSELECTIONCARETVIRTUALSPACE) > 0;
        shot(c, (QByteArray(k.name) + "_selection").constData());
        typeText(ed, "|");
        shot(c, (QByteArray(k.name) + "_typed").constData());
        const QString evidence = sel + "; after typing '|' once: " + quoted(bufferText(ed));
        if (k.gate)
            report(c, k.name, rectangular && bufferText(ed) == k.expected, evidence);
        else
            std::printf("CHECK %-20s INFO %s (expected %s)\n", k.name, qPrintable(evidence),
                        qPrintable(quoted(k.expected)));
    }
    ed->send(SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION);
}

void checkMultiSelectionTyping(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    setText(ed, "one\ntwo\nthree\n");
    ed->send(SCI_SETSELECTION, 3, 3);
    ed->send(SCI_ADDSELECTION, 7, 7);
    ed->send(SCI_ADDSELECTION, 13, 13);
    const sptr_t selections = ed->send(SCI_GETSELECTIONS);
    typeText(ed, "_x");
    shot(c, "multi_selection_typing");
    const QByteArray expected = "one_x\ntwo_x\nthree_x\n";
    report(c, "multi_select_typing", selections == 3 && bufferText(ed) == expected,
           QString("selections=%1; typed '_x' once: %2").arg(selections)
               .arg(quoted(bufferText(ed))));
}

int smartHighlight(ScintillaEdit *ed, const QByteArray &word)
{
    const sptr_t length = ed->send(SCI_GETLENGTH);
    ed->send(SCI_SETINDICATORCURRENT, kSmartHighlightIndicator);
    ed->send(SCI_INDICATORCLEARRANGE, 0, length);
    ed->send(SCI_SETSEARCHFLAGS, SCFIND_WHOLEWORD | SCFIND_MATCHCASE);
    int count = 0;
    for (sptr_t from = 0;;) {
        ed->send(SCI_SETTARGETRANGE, from, length);
        const sptr_t at = ed->send(SCI_SEARCHINTARGET, word.size(),
                                   reinterpret_cast<sptr_t>(word.constData()));
        if (at < 0)
            break;
        ed->send(SCI_INDICATORFILLRANGE, at, word.size());
        from = at + word.size();
        ++count;
    }
    return count;
}

int countIndicatorRuns(ScintillaEdit *ed, int indicator)
{
    int runs = 0;
    const sptr_t length = ed->send(SCI_GETLENGTH);
    for (sptr_t pos = 0; pos < length;) {
        if (ed->send(SCI_INDICATORVALUEAT, indicator, pos))
            ++runs;
        const sptr_t next = ed->send(SCI_INDICATOREND, indicator, pos);
        if (next <= pos)
            break;
        pos = next;
    }
    return runs;
}

void checkSmartHighlight(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    setText(ed, sampleCpp());
    applyCppLexer(ed);
    const int filled = smartHighlight(ed, "value");
    const sptr_t first = QByteArray(sampleCpp()).indexOf("value");
    ed->send(SCI_SETSEL, first, first + 5);
    shot(c, "smart_highlight");
    const int runs = countIndicatorRuns(ed, kSmartHighlightIndicator);
    report(c, "smart_highlight", filled == 3 && runs == 3,
           QString("whole-word 'value': %1 matches filled with INDIC_ROUNDBOX on indicator "
                   "%2; indicator runs read back: %3").arg(filled)
               .arg(kSmartHighlightIndicator).arg(runs));
}

QString historyBits(ScintillaEdit *ed, int line)
{
    const sptr_t m = ed->send(SCI_MARKERGET, line);
    QStringList names;
    if (m & (1 << SC_MARKNUM_HISTORY_REVERTED_TO_ORIGIN)) names << "reverted_to_origin";
    if (m & (1 << SC_MARKNUM_HISTORY_SAVED)) names << "saved";
    if (m & (1 << SC_MARKNUM_HISTORY_MODIFIED)) names << "modified";
    if (m & (1 << SC_MARKNUM_HISTORY_REVERTED_TO_MODIFIED)) names << "reverted_to_modified";
    return names.isEmpty() ? "none" : names.join("+");
}

void checkChangeHistory(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    applyNoLexer(ed);
    setText(ed, "line0\nline1\nline2\nline3\nline4\n");
    ed->send(SCI_SETCHANGEHISTORY, SC_CHANGE_HISTORY_ENABLED | SC_CHANGE_HISTORY_MARKERS);
    // L1 and L2 change, then save; undoing L2's change past the save point makes
    // it "reverted to origin" (ScintillaDoc.html, Change history); L3 changes
    // after the save and stays "modified".
    ed->send(SCI_INSERTTEXT, ed->send(SCI_POSITIONFROMLINE, 1), reinterpret_cast<sptr_t>("A"));
    ed->send(SCI_INSERTTEXT, ed->send(SCI_POSITIONFROMLINE, 2), reinterpret_cast<sptr_t>("C"));
    ed->send(SCI_SETSAVEPOINT);
    ed->send(SCI_UNDO);
    ed->send(SCI_INSERTTEXT, ed->send(SCI_POSITIONFROMLINE, 3), reinterpret_cast<sptr_t>("B"));
    shot(c, "change_history");
    QStringList states;
    for (int l = 0; l < 5; ++l)
        states << QString("L%1=%2").arg(l).arg(historyBits(ed, l));
    const bool ok = historyBits(ed, 1) == "saved" && historyBits(ed, 3) == "modified"
                    && historyBits(ed, 2) == "reverted_to_origin" && historyBits(ed, 0) == "none";
    report(c, "change_history", ok,
           QString("SCI_GETCHANGEHISTORY=%1; ").arg(ed->send(SCI_GETCHANGEHISTORY))
               + states.join(" "));
}

void checkHideLines(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    QByteArray text;
    for (int i = 0; i < 20; ++i)
        text += "line " + QByteArray::number(i) + "\n";
    setText(ed, text.constData());
    ed->send(SCI_HIDELINES, 5, 9);
    shot(c, "hide_lines");
    bool hiddenOk = ed->send(SCI_GETLINEVISIBLE, 4) && ed->send(SCI_GETLINEVISIBLE, 10);
    for (int l = 5; l <= 9; ++l)
        hiddenOk = hiddenOk && !ed->send(SCI_GETLINEVISIBLE, l);
    const sptr_t displayOf10 = ed->send(SCI_VISIBLEFROMDOCLINE, 10);
    const sptr_t allVisibleWhileHidden = ed->send(SCI_GETALLLINESVISIBLE);
    ed->send(SCI_SHOWLINES, 5, 9);
    const sptr_t allVisibleAfter = ed->send(SCI_GETALLLINESVISIBLE);
    report(c, "hide_lines", hiddenOk && displayOf10 == 5 && !allVisibleWhileHidden
                                && allVisibleAfter,
           QString("lines 5-9 hidden; doc line 10 shown at display line %1; "
                   "allLinesVisible hidden=%2 after SHOWLINES=%3")
               .arg(displayOf10).arg(allVisibleWhileHidden).arg(allVisibleAfter));
}

int visibleLineCount(ScintillaEdit *ed)
{
    int n = 0;
    const sptr_t lines = ed->send(SCI_GETLINECOUNT);
    for (sptr_t l = 0; l < lines; ++l)
        n += ed->send(SCI_GETLINEVISIBLE, l) ? 1 : 0;
    return n;
}

void checkFoldLevels(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    setText(ed, sampleCpp());
    applyCppLexer(ed);
    QStringList table;
    const sptr_t lines = ed->send(SCI_GETLINECOUNT);
    for (sptr_t l = 0; l < lines; ++l) {
        const sptr_t level = ed->send(SCI_GETFOLDLEVEL, l);
        if (level & SC_FOLDLEVELHEADERFLAG)
            table << QString("L%1:depth%2").arg(l).arg((level & SC_FOLDLEVELNUMBERMASK)
                                                      - SC_FOLDLEVELBASE);
    }
    const int before = visibleLineCount(ed);
    // Notepad++ "Collapse level 2" (Alt+2): contract every header at depth 1.
    for (sptr_t l = 0; l < lines; ++l) {
        const sptr_t level = ed->send(SCI_GETFOLDLEVEL, l);
        if ((level & SC_FOLDLEVELHEADERFLAG)
            && (level & SC_FOLDLEVELNUMBERMASK) - SC_FOLDLEVELBASE == 1)
            ed->send(SCI_FOLDLINE, l, SC_FOLDACTION_CONTRACT);
    }
    const int after = visibleLineCount(ed);
    shot(c, "fold_level2");
    const bool ok = table.size() == 4 && after < before && ed->send(SCI_GETFOLDEXPANDED, 2)
                    && !ed->send(SCI_GETFOLDEXPANDED, 3);
    report(c, "fold_levels", ok,
           "headers " + table.join(" ")
               + QString("; collapse level 2: visible lines %1 -> %2, namespace expanded=%3, "
                         "class expanded=%4").arg(before).arg(after)
                     .arg(ed->send(SCI_GETFOLDEXPANDED, 2)).arg(ed->send(SCI_GETFOLDEXPANDED, 3)));
    ed->send(SCI_FOLDALL, SC_FOLDACTION_EXPAND);
}

const char *sampleHtml()
{
    return "<!DOCTYPE html>\n<html>\n<head>\n<title>Spike</title>\n"
           "<style>\nbody { color: red; margin: 0; }\n</style>\n"
           "<script>\nfunction hello(n) { var x = 42; return \"hi\"; }\n</script>\n"
           "</head>\n<body><p class=\"c\">Text</p></body>\n</html>\n";
}

void checkHtmlEmbedded(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    setText(ed, sampleHtml());
    applyHtmlLexer(ed);
    const QByteArray html(sampleHtml());
    auto styleAt = [ed, &html](const char *needle, int offset = 0) {
        return static_cast<int>(ed->send(SCI_GETSTYLEAT, html.indexOf(needle) + offset));
    };
    const int tag = styleAt("<p", 1), attr = styleAt("class="), keyword = styleAt("function");
    const int number = styleAt("42"), string = styleAt("\"hi\""), word = styleAt("hello");
    shot(c, "html_embedded");
    report(c, "html_embedded_js", tag == SCE_H_TAG && attr == SCE_H_ATTRIBUTE
                                      && keyword == SCE_HJ_KEYWORD && number == SCE_HJ_NUMBER
                                      && string == SCE_HJ_DOUBLESTRING && word == SCE_HJ_WORD,
           QString("styles: <p>=%1 class=%2 function=%3 42=%4 \"hi\"=%5 hello=%6 "
                   "(expected %7 %8 %9 %10 %11 %12)")
               .arg(tag).arg(attr).arg(keyword).arg(number).arg(string).arg(word)
               .arg(SCE_H_TAG).arg(SCE_H_ATTRIBUTE).arg(SCE_HJ_KEYWORD).arg(SCE_HJ_NUMBER)
               .arg(SCE_HJ_DOUBLESTRING).arg(SCE_HJ_WORD));
    const int sel = styleAt("body {"), prop = styleAt("color"), value = styleAt("red");
    const int brace = styleAt("{ color");
    report(c, "html_embedded_css", !(sel == prop && prop == value && value == brace),
           QString("styles inside <style>: selector=%1 '{'=%2 property=%3 value=%4 "
                   "(identical = no CSS sub-lexer in Lexilla's hypertext lexer)")
               .arg(sel).arg(brace).arg(prop).arg(value));
}

void checkBidiInfo(Ctx &c)
{
    ScintillaEdit *ed = c.ed;
    applyNoLexer(ed);
    setText(ed, "Hello \xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7 \xD8\xA8\xD8\xA7\xD9\x84"
                "\xD8\xB9\xD8\xA7\xD9\x84\xD9\x85 world 123\n");
    ed->send(SCI_SETBIDIRECTIONAL, SC_BIDIRECTIONAL_L2R);
    const sptr_t bidi = ed->send(SCI_GETBIDIRECTIONAL);
    shot(c, "bidi_mixed_line");
    info(QString("INFO bidi: SCI_SETBIDIRECTIONAL(L2R) then SCI_GETBIDIRECTIONAL = %1 "
                 "(0 = SC_BIDIRECTIONAL_DISABLED: the Qt platform layer does not implement it)")
             .arg(bidi));
}

void demoShot(Ctx &c)
{
    setText(c.ed, sampleCpp());
    applyCppLexer(c.ed);
    c.ed->send(SCI_SETSELECTION, 120, 120);
    c.ed->send(SCI_ADDSELECTION, 160, 160);
    shot(c, "demo_cpp");
}

} // namespace

int runSelfTest(ScintillaEdit *ed, const QString &shotDir, const QString &scratchDir)
{
    Ctx c{ed, shotDir, scratchDir};
    demoShot(c);
    checkLspDidChange(c);
    checkToySave(c);
    checkUndoGrouping(c);
    checkRectangularVirtual(c);
    checkMultiSelectionTyping(c);
    checkSmartHighlight(c);
    checkChangeHistory(c);
    checkHideLines(c);
    checkFoldLevels(c);
    checkHtmlEmbedded(c);
    checkBidiInfo(c);
    std::printf("SELFTEST failures=%d\n", c.failures);
    return c.failures;
}

} // namespace demo
