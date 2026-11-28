#ifndef OPENOPTIONS_H
#define OPENOPTIONS_H

#include <QMetaType>
#include <QString>

// How files named on the command line are opened (Notepad++ -n -c -p -l -ro).
// Carried from main.cpp to MainWindow::applyOpenOptions, and through the
// single-instance forwarding request.
struct OpenOptions {
    int line = 0;        // 1-based; 0 = none
    int column = 0;      // 1-based, with line; 0 = none
    int position = -1;   // 0-based character offset, used when line is 0; -1 = none
    QString language;    // a Notepad++ or PrimeEdit language name; empty = detect
    bool readOnly = false;

    bool hasNavigation() const { return line > 0 || position >= 0; }
};

Q_DECLARE_METATYPE(OpenOptions)

#endif // OPENOPTIONS_H
