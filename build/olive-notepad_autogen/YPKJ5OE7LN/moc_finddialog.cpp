/****************************************************************************
** Meta object code from reading C++ file 'finddialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/finddialog.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'finddialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FindDialog_t {
    const uint offsetsAndSize[20];
    char stringdata0[105];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_FindDialog_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_FindDialog_t qt_meta_stringdata_FindDialog = {
    {
QT_MOC_LITERAL(0, 10), // "FindDialog"
QT_MOC_LITERAL(11, 8), // "findNext"
QT_MOC_LITERAL(20, 0), // ""
QT_MOC_LITERAL(21, 12), // "findPrevious"
QT_MOC_LITERAL(34, 10), // "replaceOne"
QT_MOC_LITERAL(45, 10), // "replaceAll"
QT_MOC_LITERAL(56, 10), // "onFindNext"
QT_MOC_LITERAL(67, 14), // "onFindPrevious"
QT_MOC_LITERAL(82, 9), // "onReplace"
QT_MOC_LITERAL(92, 12) // "onReplaceAll"

    },
    "FindDialog\0findNext\0\0findPrevious\0"
    "replaceOne\0replaceAll\0onFindNext\0"
    "onFindPrevious\0onReplace\0onReplaceAll"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FindDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    1 /* Public */,
       3,    0,   63,    2, 0x06,    2 /* Public */,
       4,    0,   64,    2, 0x06,    3 /* Public */,
       5,    0,   65,    2, 0x06,    4 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    0,   66,    2, 0x08,    5 /* Private */,
       7,    0,   67,    2, 0x08,    6 /* Private */,
       8,    0,   68,    2, 0x08,    7 /* Private */,
       9,    0,   69,    2, 0x08,    8 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void FindDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FindDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->findNext(); break;
        case 1: _t->findPrevious(); break;
        case 2: _t->replaceOne(); break;
        case 3: _t->replaceAll(); break;
        case 4: _t->onFindNext(); break;
        case 5: _t->onFindPrevious(); break;
        case 6: _t->onReplace(); break;
        case 7: _t->onReplaceAll(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FindDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FindDialog::findNext)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FindDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FindDialog::findPrevious)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (FindDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FindDialog::replaceOne)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (FindDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FindDialog::replaceAll)) {
                *result = 3;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject FindDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_FindDialog.offsetsAndSize,
    qt_meta_data_FindDialog,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_FindDialog_t
, QtPrivate::TypeAndForceComplete<FindDialog, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *FindDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FindDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FindDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int FindDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
void FindDialog::findNext()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void FindDialog::findPrevious()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void FindDialog::replaceOne()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void FindDialog::replaceAll()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
