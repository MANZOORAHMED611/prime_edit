/****************************************************************************
** Meta object code from reading C++ file 'macrorecorder.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/macrorecorder.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'macrorecorder.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MacroRecorder_t {
    const uint offsetsAndSize[18];
    char stringdata0[109];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_MacroRecorder_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_MacroRecorder_t qt_meta_stringdata_MacroRecorder = {
    {
QT_MOC_LITERAL(0, 13), // "MacroRecorder"
QT_MOC_LITERAL(14, 16), // "recordingStarted"
QT_MOC_LITERAL(31, 0), // ""
QT_MOC_LITERAL(32, 16), // "recordingStopped"
QT_MOC_LITERAL(49, 15), // "playbackStarted"
QT_MOC_LITERAL(65, 15), // "playbackStopped"
QT_MOC_LITERAL(81, 10), // "macroSaved"
QT_MOC_LITERAL(92, 4), // "name"
QT_MOC_LITERAL(97, 11) // "macroLoaded"

    },
    "MacroRecorder\0recordingStarted\0\0"
    "recordingStopped\0playbackStarted\0"
    "playbackStopped\0macroSaved\0name\0"
    "macroLoaded"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MacroRecorder[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x06,    1 /* Public */,
       3,    0,   51,    2, 0x06,    2 /* Public */,
       4,    0,   52,    2, 0x06,    3 /* Public */,
       5,    0,   53,    2, 0x06,    4 /* Public */,
       6,    1,   54,    2, 0x06,    5 /* Public */,
       8,    1,   57,    2, 0x06,    7 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,

       0        // eod
};

void MacroRecorder::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MacroRecorder *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->recordingStarted(); break;
        case 1: _t->recordingStopped(); break;
        case 2: _t->playbackStarted(); break;
        case 3: _t->playbackStopped(); break;
        case 4: _t->macroSaved((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->macroLoaded((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MacroRecorder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MacroRecorder::recordingStarted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MacroRecorder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MacroRecorder::recordingStopped)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MacroRecorder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MacroRecorder::playbackStarted)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MacroRecorder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MacroRecorder::playbackStopped)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MacroRecorder::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MacroRecorder::macroSaved)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MacroRecorder::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MacroRecorder::macroLoaded)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject MacroRecorder::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_MacroRecorder.offsetsAndSize,
    qt_meta_data_MacroRecorder,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_MacroRecorder_t
, QtPrivate::TypeAndForceComplete<MacroRecorder, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>



>,
    nullptr
} };


const QMetaObject *MacroRecorder::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MacroRecorder::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MacroRecorder.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MacroRecorder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void MacroRecorder::recordingStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MacroRecorder::recordingStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MacroRecorder::playbackStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MacroRecorder::playbackStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MacroRecorder::macroSaved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void MacroRecorder::macroLoaded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
