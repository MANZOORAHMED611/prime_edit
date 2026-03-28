/****************************************************************************
** Meta object code from reading C++ file 'terminalwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/terminalwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'terminalwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TerminalWidget_t {
    const uint offsetsAndSize[38];
    char stringdata0[281];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_TerminalWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_TerminalWidget_t qt_meta_stringdata_TerminalWidget = {
    {
QT_MOC_LITERAL(0, 14), // "TerminalWidget"
QT_MOC_LITERAL(15, 15), // "commandExecuted"
QT_MOC_LITERAL(31, 0), // ""
QT_MOC_LITERAL(32, 7), // "command"
QT_MOC_LITERAL(40, 15), // "processFinished"
QT_MOC_LITERAL(56, 8), // "exitCode"
QT_MOC_LITERAL(65, 25), // "onReadyReadStandardOutput"
QT_MOC_LITERAL(91, 24), // "onReadyReadStandardError"
QT_MOC_LITERAL(116, 17), // "onProcessFinished"
QT_MOC_LITERAL(134, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(155, 10), // "exitStatus"
QT_MOC_LITERAL(166, 14), // "onProcessError"
QT_MOC_LITERAL(181, 22), // "QProcess::ProcessError"
QT_MOC_LITERAL(204, 5), // "error"
QT_MOC_LITERAL(210, 20), // "onInputReturnPressed"
QT_MOC_LITERAL(231, 14), // "onShellChanged"
QT_MOC_LITERAL(246, 5), // "index"
QT_MOC_LITERAL(252, 14), // "onClearClicked"
QT_MOC_LITERAL(267, 13) // "onStopClicked"

    },
    "TerminalWidget\0commandExecuted\0\0command\0"
    "processFinished\0exitCode\0"
    "onReadyReadStandardOutput\0"
    "onReadyReadStandardError\0onProcessFinished\0"
    "QProcess::ExitStatus\0exitStatus\0"
    "onProcessError\0QProcess::ProcessError\0"
    "error\0onInputReturnPressed\0onShellChanged\0"
    "index\0onClearClicked\0onStopClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TerminalWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   74,    2, 0x06,    1 /* Public */,
       4,    1,   77,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    0,   80,    2, 0x08,    5 /* Private */,
       7,    0,   81,    2, 0x08,    6 /* Private */,
       8,    2,   82,    2, 0x08,    7 /* Private */,
      11,    1,   87,    2, 0x08,   10 /* Private */,
      14,    0,   90,    2, 0x08,   12 /* Private */,
      15,    1,   91,    2, 0x08,   13 /* Private */,
      17,    0,   94,    2, 0x08,   15 /* Private */,
      18,    0,   95,    2, 0x08,   16 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Int,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 9,    5,   10,
    QMetaType::Void, 0x80000000 | 12,   13,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   16,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TerminalWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TerminalWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->commandExecuted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->processFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->onReadyReadStandardOutput(); break;
        case 3: _t->onReadyReadStandardError(); break;
        case 4: _t->onProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 5: _t->onProcessError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 6: _t->onInputReturnPressed(); break;
        case 7: _t->onShellChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->onClearClicked(); break;
        case 9: _t->onStopClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TerminalWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TerminalWidget::commandExecuted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TerminalWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TerminalWidget::processFinished)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject TerminalWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_TerminalWidget.offsetsAndSize,
    qt_meta_data_TerminalWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_TerminalWidget_t
, QtPrivate::TypeAndForceComplete<TerminalWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<QProcess::ExitStatus, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *TerminalWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TerminalWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TerminalWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int TerminalWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void TerminalWidget::commandExecuted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TerminalWidget::processFinished(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
