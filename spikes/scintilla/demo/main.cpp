// Scintilla + Lexilla on Qt 6 feasibility demo (throwaway spike).
//
//   scintilla_demo                        interactive C++ demo window
//   scintilla_demo --html                 interactive HTML (embedded JS/CSS) window
//   scintilla_demo --open FILE            open FILE (large-file loader) and stay open
//   scintilla_demo --bench FILE           open FILE, print BENCH timings, exit
//   scintilla_demo --selftest SHOTS TMP   run gate 4/5 checks, save screenshots, exit
//   --ime-log FILE                        (any interactive mode) log QInputMethodEvents
//                                         to stderr and write the buffer to FILE on change
#include <QApplication>
#include <QFile>
#include <QInputMethodEvent>
#include <QTimer>

#include <cstdio>

#include "Scintilla.h"
#include "bench.h"
#include "demosetup.h"
#include "selftest.h"

namespace {

QString hexCodepoints(const QString &s)
{
    QStringList parts;
    for (uint cp : s.toUcs4())
        parts << QString("U+%1").arg(cp, 4, 16, QChar('0')).toUpper();
    return parts.join(' ');
}

// Logs what the input method delivers, so composition can be verified headless.
class ImeLogger : public QObject
{
public:
    using QObject::QObject;
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::InputMethod) {
            auto *ime = static_cast<QInputMethodEvent *>(event);
            std::fprintf(stderr, "IME preedit=\"%s\" [%s] commit=\"%s\" [%s]\n",
                         qPrintable(ime->preeditString()),
                         qPrintable(hexCodepoints(ime->preeditString())),
                         qPrintable(ime->commitString()),
                         qPrintable(hexCodepoints(ime->commitString())));
            std::fflush(stderr);
        }
        return QObject::eventFilter(watched, event);
    }
};

void attachImeLog(ScintillaEdit *ed, const QString &logPath)
{
    ed->installEventFilter(new ImeLogger(ed));
    QObject::connect(ed, &ScintillaEditBase::notifyChange, ed, [ed, logPath] {
        QFile out(logPath);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
            out.write(demo::bufferText(ed));
    });
}

QString argAfter(const QStringList &args, const QString &flag, int offset = 1)
{
    const int i = args.indexOf(flag);
    return (i >= 0 && i + offset < args.size()) ? args.at(i + offset) : QString();
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    const QStringList args = app.arguments();

    ScintillaEdit editor;
    editor.setWindowTitle("scintilla_demo");
    editor.resize(1000, 600);
    demo::configureEditor(&editor);
    editor.show();

    if (args.contains("--selftest")) {
        QCoreApplication::processEvents();
        return demo::runSelfTest(&editor, argAfter(args, "--selftest"),
                                 argAfter(args, "--selftest", 2)) == 0 ? 0 : 1;
    }
    if (args.contains("--bench")) {
        QCoreApplication::processEvents();
        return demo::runBench(&editor, argAfter(args, "--bench"));
    }
    if (args.contains("--open")) {
        demo::applyNoLexer(&editor);
        if (!demo::loadLargeFile(&editor, argAfter(args, "--open")))
            return 1;
    } else if (args.contains("--html")) {
        editor.send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(
            "<html><head><style>p { color: red; }</style>\n"
            "<script>function f() { var x = 42; return \"s\"; }</script></head>\n"
            "<body><p class=\"c\">Text</p></body></html>\n"));
        demo::applyHtmlLexer(&editor);
    } else {
        editor.send(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(demo::sampleCpp()));
        demo::applyCppLexer(&editor);
    }
    editor.send(SCI_EMPTYUNDOBUFFER);
    if (args.contains("--ime-log"))
        attachImeLog(&editor, argAfter(args, "--ime-log"));
    return app.exec();
}
