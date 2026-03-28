/****************************************************************************
** Meta object code from reading C++ file 'editor.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/editor.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'editor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Editor_t {
    const uint offsetsAndSize[20];
    char stringdata0[134];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_Editor_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_Editor_t qt_meta_stringdata_Editor = {
    {
QT_MOC_LITERAL(0, 6), // "Editor"
QT_MOC_LITERAL(7, 21), // "jumpToMatchingBracket"
QT_MOC_LITERAL(29, 0), // ""
QT_MOC_LITERAL(30, 25), // "updateLineNumberAreaWidth"
QT_MOC_LITERAL(56, 13), // "newBlockCount"
QT_MOC_LITERAL(70, 20), // "updateLineNumberArea"
QT_MOC_LITERAL(91, 4), // "rect"
QT_MOC_LITERAL(96, 2), // "dy"
QT_MOC_LITERAL(99, 20), // "highlightCurrentLine"
QT_MOC_LITERAL(120, 13) // "onTextChanged"

    },
    "Editor\0jumpToMatchingBracket\0\0"
    "updateLineNumberAreaWidth\0newBlockCount\0"
    "updateLineNumberArea\0rect\0dy\0"
    "highlightCurrentLine\0onTextChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Editor[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x0a,    1 /* Public */,
       3,    1,   45,    2, 0x08,    2 /* Private */,
       5,    2,   48,    2, 0x08,    4 /* Private */,
       8,    0,   53,    2, 0x08,    7 /* Private */,
       9,    0,   54,    2, 0x08,    8 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::QRect, QMetaType::Int,    6,    7,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void Editor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Editor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->jumpToMatchingBracket(); break;
        case 1: _t->updateLineNumberAreaWidth((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->updateLineNumberArea((*reinterpret_cast< std::add_pointer_t<QRect>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 3: _t->highlightCurrentLine(); break;
        case 4: _t->onTextChanged(); break;
        default: ;
        }
    }
}

const QMetaObject Editor::staticMetaObject = { {
    QMetaObject::SuperData::link<QPlainTextEdit::staticMetaObject>(),
    qt_meta_stringdata_Editor.offsetsAndSize,
    qt_meta_data_Editor,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_Editor_t
, QtPrivate::TypeAndForceComplete<Editor, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QRect &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *Editor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Editor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Editor.stringdata0))
        return static_cast<void*>(this);
    return QPlainTextEdit::qt_metacast(_clname);
}

int Editor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPlainTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
