// PrimeEdit Crash & Stress Test Suite
// Tests every permutation of actions that could crash the editor
// Runs headless via QT_QPA_PLATFORM=offscreen

#include <QApplication>
#include <QTimer>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QTextCursor>
#include <QScrollBar>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>
#include <functional>

#include "ui/mainwindow.h"
#include "ui/editor.h"
#include "ui/tabwidget.h"
#include "ui/statusbar.h"
#include "ui/theme.h"
#include "ui/searchdialog.h"
#include "ui/completionpopup.h"
#include "core/document.h"
#include "core/documentmanager.h"
#include "core/piecetable.h"
#include "core/searchengine.h"
#include "core/schemavalidator.h"
#include "core/session.h"
#include "core/largefile.h"
#include "core/macrorecorder.h"
#include "core/lspclient.h"
#include "core/lspmanager.h"
#include "core/llmevaluator.h"
#include "core/encoding.h"
#include "core/charsetdetector.h"
#include "syntax/highlighter.h"
#include "syntax/languagemanager.h"
#include "utils/settings.h"

static int g_pass = 0;
static int g_fail = 0;
static int g_crash = 0;
static int g_total = 0;
static QStringList g_failures;

void runTest(const char *name, std::function<void()> fn)
{
    g_total++;
    std::cout << "  " << name << " ... " << std::flush;
    try {
        fn();
        g_pass++;
        std::cout << "\033[32mOK\033[0m" << std::endl;
    } catch (const std::exception &e) {
        g_crash++;
        std::string msg = std::string(name) + ": EXCEPTION: " + e.what();
        g_failures.append(QString::fromStdString(msg));
        std::cout << "\033[31mCRASH: " << e.what() << "\033[0m" << std::endl;
    } catch (...) {
        g_crash++;
        std::string msg = std::string(name) + ": UNKNOWN CRASH";
        g_failures.append(QString::fromStdString(msg));
        std::cout << "\033[31mCRASH\033[0m" << std::endl;
    }
}

void section(const char *name) {
    std::cout << "\n=== " << name << " ===" << std::endl;
}

// Helper: get current editor from mainwindow
Editor* getEditor(MainWindow &mw) {
    return mw.currentEditor();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("PrimeEdit");
    app.setOrganizationName("PrimeEdit");

    // Load themes
    ThemeManager::instance().loadThemes();
    Settings::instance().load();

    std::cout << "============================================================" << std::endl;
    std::cout << "  PrimeEdit Crash & Stress Test Suite" << std::endl;
    std::cout << "============================================================" << std::endl;

    // ================================================================
    section("1. MAINWINDOW LIFECYCLE");
    // ================================================================

    runTest("Create and destroy MainWindow", []() {
        MainWindow *mw = new MainWindow();
        mw->show();
        delete mw;
    });

    runTest("Create multiple MainWindows", []() {
        MainWindow *mw1 = new MainWindow();
        MainWindow *mw2 = new MainWindow();
        mw1->show();
        mw2->show();
        delete mw2;
        delete mw1;
    });

    runTest("Close with no tabs", []() {
        MainWindow mw;
        mw.show();
        mw.closeAllFiles();
        QCloseEvent event;
        QApplication::sendEvent(&mw, &event);
    });

    // ================================================================
    section("2. TAB OPERATIONS — RAPID FIRE");
    // ================================================================

    runTest("Open and close 50 tabs rapidly", []() {
        MainWindow mw;
        mw.show();
        for (int i = 0; i < 50; ++i) {
            mw.newFile();
        }
        // Mark all unmodified to avoid save prompts in headless mode
        for (int i = 0; i < mw.tabWidget()->count(); ++i) {
            Editor *e = qobject_cast<Editor*>(mw.tabWidget()->widget(i));
            if (e && e->document()) e->document()->setModified(false);
        }
        for (int i = mw.tabWidget()->count() - 1; i >= 0; --i) {
            mw.closeFile(i);
        }
    });

    runTest("Open, type, close 20 tabs", []() {
        MainWindow mw;
        mw.show();
        for (int i = 0; i < 20; ++i) {
            mw.newFile();
            Editor *e = getEditor(mw);
            if (e) {
                e->setPlainText(QString("Content for tab %1\nLine 2\nLine 3").arg(i));
                // Mark unmodified to avoid save prompt in headless mode
                e->document()->setModified(false);
            }
        }
        mw.closeAllFiles();
    });

    runTest("Switch tabs rapidly", []() {
        MainWindow mw;
        mw.show();
        for (int i = 0; i < 10; ++i) mw.newFile();
        for (int cycle = 0; cycle < 100; ++cycle) {
            mw.setCurrentTabIndex(cycle % mw.tabWidget()->count());
        }
        mw.closeAllFiles();
    });

    runTest("Close current tab when it's the only one", []() {
        MainWindow mw;
        mw.show();
        // Mark unmodified
        Editor *e = getEditor(mw);
        if (e && e->document()) e->document()->setModified(false);
        mw.closeFile(0);
        mw.newFile();
    });

    // ================================================================
    section("3. EDITOR OPERATIONS ON EMPTY DOCUMENT");
    // ================================================================

    runTest("All operations on empty editor", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;

        // These should all be no-ops, not crashes
        e->duplicateLine();
        e->deleteLine();
        e->moveLineUp();
        e->moveLineDown();
        e->toggleComment();
        e->sortLinesAscending();
        e->sortLinesDescending();
        e->removeDuplicateLines();
        e->trimTrailingWhitespace();
        e->joinLines();
        e->toUpperCase();
        e->toLowerCase();
        e->toTitleCase();
        e->foldAll();
        e->unfoldAll();
        e->clearBookmarks();
        e->nextBookmark();
        e->previousBookmark();
    });

    runTest("Search on empty document", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;

        e->findNext("test", QTextDocument::FindFlags());
        e->findPrevious("test", QTextDocument::FindFlags());
        e->replaceAll("a", "b", QTextDocument::FindFlags());
    });

    runTest("Bracket matching on empty document", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;
        e->jumpToMatchingBracket();
    });

    // ================================================================
    section("4. UNDO/REDO STRESS");
    // ================================================================

    runTest("1000 insert + undo + redo cycles", []() {
        PieceTable pt;
        for (int i = 0; i < 1000; ++i) {
            pt.insert(pt.length(), QString("Line %1\n").arg(i));
        }
        for (int i = 0; i < 1000; ++i) {
            pt.undo();
        }
        for (int i = 0; i < 1000; ++i) {
            pt.redo();
        }
    });

    runTest("Undo on empty PieceTable", []() {
        PieceTable pt;
        pt.undo(); // should be no-op
        pt.redo(); // should be no-op
    });

    runTest("Undo past beginning", []() {
        PieceTable pt;
        pt.insert(0, "Hello");
        pt.undo();
        pt.undo(); // already at beginning
        pt.undo();
    });

    runTest("Grouped undo — 500 operations in one group", []() {
        PieceTable pt;
        pt.beginUndoGroup();
        for (int i = 0; i < 500; ++i) {
            pt.insert(pt.length(), "x");
        }
        pt.endUndoGroup();
        // Single undo should revert all 500
        pt.undo();
        if (pt.length() != 0) {
            throw std::runtime_error("grouped undo failed");
        }
    });

    runTest("Nested undo groups", []() {
        PieceTable pt;
        pt.beginUndoGroup();
        pt.insert(0, "A");
        pt.beginUndoGroup();
        pt.insert(1, "B");
        pt.endUndoGroup();
        pt.endUndoGroup();
    });

    runTest("endUndoGroup without begin", []() {
        PieceTable pt;
        pt.endUndoGroup(); // should not crash
    });

    // ================================================================
    section("5. THEME SWITCHING — ALL PERMUTATIONS");
    // ================================================================

    runTest("Switch through all 9 themes", []() {
        MainWindow mw;
        mw.show();
        mw.newFile();
        Editor *e = getEditor(mw);
        if (e) e->setPlainText("int main() { return 0; }");

        QStringList themes = {"Notepad++", "Default Dark", "Default Light",
                              "Olive", "Monokai", "Dracula", "Nord",
                              "Solarized Dark", "Solarized Light"};
        for (const QString &t : themes) {
            ThemeManager::instance().applyTheme(t);
            QApplication::processEvents();
        }
    });

    runTest("Rapid theme switching 50 times", []() {
        MainWindow mw;
        mw.show();
        mw.newFile();
        QStringList themes = {"Notepad++", "Dracula", "Olive", "Nord"};
        for (int i = 0; i < 50; ++i) {
            ThemeManager::instance().applyTheme(themes[i % themes.size()]);
            QApplication::processEvents();
        }
    });

    runTest("Theme switch with multiple editors open", []() {
        MainWindow mw;
        mw.show();
        for (int i = 0; i < 5; ++i) {
            mw.newFile();
            Editor *e = getEditor(mw);
            if (e) e->setPlainText(QString("Content %1\nLine 2").arg(i));
        }
        ThemeManager::instance().applyTheme("Dracula");
        QApplication::processEvents();
        ThemeManager::instance().applyTheme("Notepad++");
        QApplication::processEvents();
    });

    // ================================================================
    section("6. SEARCH ENGINE EDGE CASES");
    // ================================================================

    runTest("Search with empty pattern", []() {
        SearchEngine engine;
        SearchEngine::Options opts;
        engine.findAll("Hello World", "", opts);
        engine.findNext("Hello World", 0, "", opts);
        engine.matchCount("Hello World", "", opts);
    });

    runTest("Search with empty text", []() {
        SearchEngine engine;
        SearchEngine::Options opts;
        engine.findAll("", "pattern", opts);
        engine.findNext("", 0, "pattern", opts);
    });

    runTest("Invalid regex", []() {
        SearchEngine engine;
        SearchEngine::Options opts;
        opts.mode = SearchEngine::Regex;
        engine.findAll("Hello World", "[invalid(regex", opts);
    });

    runTest("Search with very long pattern", []() {
        SearchEngine engine;
        SearchEngine::Options opts;
        QString longPattern(10000, 'a');
        engine.findAll("test", longPattern, opts);
    });

    runTest("Replace in empty text", []() {
        SearchEngine engine;
        SearchEngine::Options opts;
        engine.replaceInText("", "a", "b", opts);
    });

    runTest("Extended mode — all escape sequences", []() {
        SearchEngine::expandEscapes("\\n\\t\\r\\0\\\\\\x41\\x00");
    });

    // ================================================================
    section("7. LARGE FILE READER EDGE CASES");
    // ================================================================

    runTest("Open non-existent file", []() {
        LargeFileReader reader;
        bool result = reader.open("/tmp/does_not_exist_12345.txt");
        if (result) throw std::runtime_error("should fail");
    });

    runTest("Open empty file", []() {
        QTemporaryFile tmp;
        tmp.open();
        tmp.close();
        LargeFileReader reader;
        reader.open(tmp.fileName());
        // Should handle gracefully
    });

    runTest("Read line beyond bounds", []() {
        QTemporaryFile tmp;
        tmp.open();
        tmp.write("Line 1\nLine 2\nLine 3\n");
        tmp.close();
        LargeFileReader reader;
        reader.open(tmp.fileName());
        reader.line(-1);     // negative
        reader.line(999999); // way past end
        reader.lines(0, 999999); // request more than available
        reader.lines(-1, 5);     // negative start
    });

    runTest("Close and access", []() {
        LargeFileReader reader;
        reader.close(); // close without open
        reader.line(0); // access after close
        reader.lineCount();
        reader.fileSize();
    });

    // ================================================================
    section("8. DOCUMENT OPERATIONS");
    // ================================================================

    runTest("Save untitled document to valid path", []() {
        Document doc;
        doc.setText("Test content");
        QTemporaryFile tmp;
        tmp.open();
        QString path = tmp.fileName();
        tmp.close();
        doc.saveAs(path);
    });

    runTest("Load then immediately save", []() {
        QTemporaryFile tmp;
        tmp.open();
        tmp.write("Original content");
        tmp.close();
        Document doc;
        doc.load(tmp.fileName());
        doc.save();
    });

    runTest("Set encoding to every supported type", []() {
        Document doc;
        doc.setText("Hello");
        doc.setEncoding("UTF-8");
        doc.setEncoding("UTF-8-BOM");
        doc.setEncoding("UTF-16LE");
        doc.setEncoding("UTF-16BE");
        doc.setEncoding("ANSI");
    });

    runTest("Set all line endings", []() {
        Document doc;
        doc.setText("Line 1\nLine 2");
        doc.setLineEnding(Document::Unix);
        doc.setLineEnding(Document::Windows);
        doc.setLineEnding(Document::ClassicMac);
    });

    runTest("Insert at every position", []() {
        Document doc;
        doc.setText("Hello");
        for (int i = 0; i <= 5; ++i) {
            doc.insert(i, "X");
        }
    });

    runTest("Remove at every position", []() {
        Document doc;
        doc.setText("XXXXXXXXXX");
        for (int i = 9; i >= 0; --i) {
            doc.remove(i, 1);
        }
    });

    runTest("Remove beyond text length", []() {
        Document doc;
        doc.setText("Hi");
        doc.remove(0, 100); // remove more than exists
    });

    // ================================================================
    section("9. CHARSET DETECTION EDGE CASES");
    // ================================================================

    runTest("Detect empty data", []() {
        CharsetDetector::detect(QByteArray());
    });

    runTest("Detect single byte", []() {
        CharsetDetector::detect(QByteArray(1, 'A'));
    });

    runTest("Detect all nulls", []() {
        CharsetDetector::detect(QByteArray(100, '\0'));
    });

    runTest("Detect random high bytes", []() {
        QByteArray data;
        for (int i = 0; i < 1000; ++i) {
            data.append(static_cast<char>(128 + (i % 128)));
        }
        CharsetDetector::detect(data);
    });

    runTest("Detect truncated UTF-8", []() {
        QByteArray data;
        data.append('\xC3'); // start of 2-byte sequence without continuation
        CharsetDetector::detect(data);
    });

    // ================================================================
    section("10. SCHEMA VALIDATION EDGE CASES");
    // ================================================================

    runTest("Validate empty text", []() {
        SchemaValidator v;
        v.loadSchemaFromJson(SchemaValidator::hadithSchema());
        v.validate("");
    });

    runTest("Validate with no schema loaded", []() {
        SchemaValidator v;
        v.validate("Some text");
    });

    runTest("Load invalid schema JSON", []() {
        SchemaValidator v;
        QJsonObject bad;
        bad["type"] = "test";
        // no rules array
        v.loadSchemaFromJson(bad);
    });

    runTest("Rule with invalid regex pattern", []() {
        SchemaValidator v;
        QJsonObject schema;
        schema["type"] = "test";
        QJsonArray rules;
        QJsonObject rule;
        rule["name"] = "bad_regex";
        rule["pattern"] = "[invalid(regex";
        rule["mustExist"] = true;
        rule["message"] = "test";
        rule["severity"] = "error";
        rule["scope"] = "document";
        rules.append(rule);
        schema["rules"] = rules;
        v.loadSchemaFromJson(schema);
        v.validate("Some text");
    });

    // ================================================================
    section("11. ENCODING EDGE CASES");
    // ================================================================

    runTest("Encode/decode empty string", []() {
        Encoding::encode("", "UTF-8");
        Encoding::decode(QByteArray(), "UTF-8");
    });

    runTest("Encode with unknown encoding name", []() {
        Encoding::encode("Hello", "FAKE-ENCODING-999");
    });

    runTest("Decode with unknown encoding name", []() {
        Encoding::decode("Hello", "FAKE-ENCODING-999");
    });

    // ================================================================
    section("12. MACRO RECORDER EDGE CASES");
    // ================================================================

    runTest("Stop without start", []() {
        MacroRecorder::instance().stopRecording();
    });

    runTest("Playback with no recording", []() {
        MacroRecorder::instance().clear();
        // playback with no target is not safe without a widget
        // but clear + description should work
        MacroRecorder::instance().currentMacroDescription();
    });

    runTest("Save empty macro", []() {
        MacroRecorder::instance().clear();
        MacroRecorder::instance().saveMacro("empty_test");
    });

    runTest("Delete non-existent macro", []() {
        MacroRecorder::instance().deleteMacro("non_existent_macro_xyz");
    });

    runTest("Load non-existent macro", []() {
        MacroRecorder::instance().loadMacro("non_existent_macro_xyz");
    });

    // ================================================================
    section("13. SETTINGS EDGE CASES");
    // ================================================================

    runTest("Set font size to extremes", []() {
        Settings &s = Settings::instance();
        s.setFontSize(1);
        s.setFontSize(200);
        s.setFontSize(0);
        s.setFontSize(-5);
        s.setFontSize(10); // restore
    });

    runTest("Set tab width to extremes", []() {
        Settings &s = Settings::instance();
        s.setTabWidth(0);
        s.setTabWidth(100);
        s.setTabWidth(4); // restore
    });

    runTest("Set empty font family", []() {
        Settings &s = Settings::instance();
        s.setFontFamily("");
        s.setFontFamily("DejaVu Sans Mono"); // restore
    });

    runTest("Set empty theme name", []() {
        Settings &s = Settings::instance();
        s.setTheme("");
        s.setTheme("Notepad++"); // restore
    });

    // ================================================================
    section("14. EDITOR — EXTREME CONTENT");
    // ================================================================

    runTest("Paste 100,000 characters", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;
        QString bigText(100000, 'X');
        e->setPlainText(bigText);
    });

    runTest("10,000 lines with bookmarks on each", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;
        QString text;
        for (int i = 0; i < 10000; ++i) {
            text += QString("Line %1\n").arg(i);
        }
        e->setPlainText(text);
        for (int i = 1; i <= 100; ++i) {
            e->toggleBookmark(i * 100);
        }
        e->nextBookmark();
        e->previousBookmark();
        e->clearBookmarks();
    });

    runTest("Very long single line (1MB)", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;
        QString longLine(1000000, 'A');
        e->setPlainText(longLine);
    });

    runTest("Unicode stress — mixed scripts", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;
        e->setPlainText(
            "English text\n"
            "\xD8\xA8\xD8\xB3\xD9\x85 \xD8\xA7\xD9\x84\xD9\x84\xD9\x87\n"  // Arabic
            "\xE4\xB8\xAD\xE6\x96\x87\xE6\xB5\x8B\xE8\xAF\x95\n"            // Chinese
            "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E\n"                          // Japanese
            "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4\n"                          // Korean
            "\xF0\x9F\x98\x80\xF0\x9F\x8E\x89\xF0\x9F\x9A\x80\n"              // Emoji
        );
        QApplication::processEvents();
    });

    // ================================================================
    section("15. OPEN FILE + THEME SWITCH + CLOSE (THE CRASH COMBO)");
    // ================================================================

    runTest("Open file, switch to Olive, close", []() {
        QTemporaryFile tmp;
        tmp.open();
        tmp.write("int main() {\n    return 0;\n}\n");
        tmp.close();

        MainWindow mw;
        mw.show();
        mw.openFile(tmp.fileName());
        QApplication::processEvents();

        ThemeManager::instance().applyTheme("Olive");
        QApplication::processEvents();

        QCloseEvent event;
        QApplication::sendEvent(&mw, &event);
    });

    runTest("Open file, switch all themes, close", []() {
        QTemporaryFile tmp;
        tmp.open();
        tmp.write("def hello():\n    print('hello')\n");
        tmp.close();

        MainWindow mw;
        mw.show();
        mw.openFile(tmp.fileName());

        QStringList themes = {"Olive", "Dracula", "Monokai", "Nord",
                              "Solarized Dark", "Solarized Light",
                              "Default Dark", "Default Light", "Notepad++"};
        for (const QString &t : themes) {
            ThemeManager::instance().applyTheme(t);
            QApplication::processEvents();
        }

        QCloseEvent event;
        QApplication::sendEvent(&mw, &event);
    });

    runTest("Untitled with content, switch theme, close, verify save", []() {
        {
            MainWindow mw;
            mw.show();
            Editor *e = getEditor(mw);
            if (e) e->setPlainText("This must survive the theme switch");
            QApplication::processEvents();

            ThemeManager::instance().applyTheme("Dracula");
            QApplication::processEvents();

            ThemeManager::instance().applyTheme("Notepad++");
            QApplication::processEvents();

            Session::instance().saveUnsavedDocuments(&mw);
        }
        // Verify the unsaved cache has content
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions/unsaved");
        QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
        bool foundContent = false;
        for (const QString &f : files) {
            QFile file(dir.absoluteFilePath(f));
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                if (data.contains("This must survive")) {
                    foundContent = true;
                }
            }
        }
        // Clean up
        for (const QString &f : files) {
            QFile::remove(dir.absoluteFilePath(f));
        }
        if (!foundContent) {
            throw std::runtime_error("unsaved content not persisted after theme switch");
        }
    });

    // ================================================================
    section("16. SESSION SAVE/RESTORE STRESS");
    // ================================================================

    runTest("Save and restore session with 10 tabs", []() {
        QString sessionDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions/unsaved";
        // Clean up first
        QDir(sessionDir).removeRecursively();
        QDir().mkpath(sessionDir);

        {
            MainWindow mw;
            mw.show();
            for (int i = 0; i < 10; ++i) {
                mw.newFile();
                Editor *e = getEditor(mw);
                if (e) e->setPlainText(QString("Tab %1 content").arg(i));
            }
            Session::instance().saveUnsavedDocuments(&mw);
        }
        {
            MainWindow mw2;
            mw2.show();
            Session::instance().restoreUnsavedDocuments(&mw2);
            // Should have restored tabs
        }
        // Clean up
        QDir(sessionDir).removeRecursively();
        QDir().mkpath(sessionDir);
    });

    // ================================================================
    section("17. LSP CLIENT EDGE CASES");
    // ================================================================

    runTest("Create LSPClient with non-existent server", []() {
        LSPClient client("non_existent_server_binary_xyz", {});
        client.initialize("/tmp");
        // Should fail gracefully
        QApplication::processEvents();
    });

    runTest("Send requests before initialization", []() {
        LSPClient client("non_existent", {});
        client.hover("file:///test.cpp", 0, 0);
        client.completion("file:///test.cpp", 0, 0);
        client.gotoDefinition("file:///test.cpp", 0, 0);
        client.references("file:///test.cpp", 0, 0);
        client.didOpen("file:///test.cpp", "cpp", "int main() {}");
        client.didChange("file:///test.cpp", "modified");
        client.didClose("file:///test.cpp");
        client.didSave("file:///test.cpp");
    });

    runTest("Shutdown without initialize", []() {
        LSPClient client("non_existent", {});
        client.shutdown();
    });

    // ================================================================
    section("18. CONCURRENT OPERATIONS");
    // ================================================================

    runTest("Edit while search is active", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;
        e->setPlainText("Hello World Hello World Hello World");
        e->findNext("Hello", QTextDocument::FindFlags());
        e->setPlainText("Changed content");
    });

    runTest("Theme switch during text modification", []() {
        MainWindow mw;
        mw.show();
        Editor *e = getEditor(mw);
        if (!e) return;
        e->setPlainText("int x = 42;");
        ThemeManager::instance().applyTheme("Monokai");
        e->setPlainText("let y = 43;");
        ThemeManager::instance().applyTheme("Notepad++");
        QApplication::processEvents();
    });

    // ================================================================
    // RESULTS
    // ================================================================

    std::cout << "\n============================================================" << std::endl;
    std::cout << "  RESULTS: " << g_pass << " passed, " << g_crash << " crashed, " << g_total << " total" << std::endl;

    if (g_crash == 0) {
        std::cout << "  \033[32mNO CRASHES DETECTED\033[0m" << std::endl;
    } else {
        std::cout << "  \033[31m" << g_crash << " CRASHES:\033[0m" << std::endl;
        for (const QString &f : g_failures) {
            std::cout << "    - " << f.toStdString() << std::endl;
        }
    }
    std::cout << "============================================================" << std::endl;

    return g_crash > 0 ? 1 : 0;
}
