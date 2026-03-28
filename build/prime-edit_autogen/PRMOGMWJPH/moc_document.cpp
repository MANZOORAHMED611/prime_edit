/****************************************************************************
** Meta object code from reading C++ file 'document.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/document.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'document.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Document_t {
    const uint offsetsAndSize[34];
    char stringdata0[195];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_Document_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_Document_t qt_meta_stringdata_Document = {
    {
QT_MOC_LITERAL(0, 8), // "Document"
QT_MOC_LITERAL(9, 15), // "modifiedChanged"
QT_MOC_LITERAL(25, 0), // ""
QT_MOC_LITERAL(26, 8), // "modified"
QT_MOC_LITERAL(35, 15), // "filePathChanged"
QT_MOC_LITERAL(51, 4), // "path"
QT_MOC_LITERAL(56, 15), // "encodingChanged"
QT_MOC_LITERAL(72, 8), // "encoding"
QT_MOC_LITERAL(81, 17), // "lineEndingChanged"
QT_MOC_LITERAL(99, 10), // "LineEnding"
QT_MOC_LITERAL(110, 6), // "ending"
QT_MOC_LITERAL(117, 15), // "languageChanged"
QT_MOC_LITERAL(133, 8), // "language"
QT_MOC_LITERAL(142, 15), // "readOnlyChanged"
QT_MOC_LITERAL(158, 8), // "readOnly"
QT_MOC_LITERAL(167, 15), // "contentsChanged"
QT_MOC_LITERAL(183, 11) // "setModified"

    },
    "Document\0modifiedChanged\0\0modified\0"
    "filePathChanged\0path\0encodingChanged\0"
    "encoding\0lineEndingChanged\0LineEnding\0"
    "ending\0languageChanged\0language\0"
    "readOnlyChanged\0readOnly\0contentsChanged\0"
    "setModified"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Document[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   62,    2, 0x06,    1 /* Public */,
       4,    1,   65,    2, 0x06,    3 /* Public */,
       6,    1,   68,    2, 0x06,    5 /* Public */,
       8,    1,   71,    2, 0x06,    7 /* Public */,
      11,    1,   74,    2, 0x06,    9 /* Public */,
      13,    1,   77,    2, 0x06,   11 /* Public */,
      15,    0,   80,    2, 0x06,   13 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      16,    1,   81,    2, 0x0a,   14 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::Bool,   14,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    3,

       0        // eod
};

void Document::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Document *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->modifiedChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->filePathChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->encodingChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->lineEndingChanged((*reinterpret_cast< std::add_pointer_t<LineEnding>>(_a[1]))); break;
        case 4: _t->languageChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->readOnlyChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->contentsChanged(); break;
        case 7: _t->setModified((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Document::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Document::modifiedChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Document::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Document::filePathChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Document::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Document::encodingChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Document::*)(LineEnding );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Document::lineEndingChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Document::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Document::languageChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Document::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Document::readOnlyChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Document::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Document::contentsChanged)) {
                *result = 6;
                return;
            }
        }
    }
}

const QMetaObject Document::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Document.offsetsAndSize,
    qt_meta_data_Document,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_Document_t
, QtPrivate::TypeAndForceComplete<Document, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<LineEnding, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>


>,
    nullptr
} };


const QMetaObject *Document::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Document::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Document.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Document::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void Document::modifiedChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Document::filePathChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Document::encodingChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Document::lineEndingChanged(LineEnding _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Document::languageChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Document::readOnlyChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void Document::contentsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
