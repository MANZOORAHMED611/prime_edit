/****************************************************************************
** Meta object code from reading C++ file 'searchdialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/searchdialog.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'searchdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SearchDialog_t {
    const uint offsetsAndSize[26];
    char stringdata0[153];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SearchDialog_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SearchDialog_t qt_meta_stringdata_SearchDialog = {
    {
QT_MOC_LITERAL(0, 12), // "SearchDialog"
QT_MOC_LITERAL(13, 8), // "findNext"
QT_MOC_LITERAL(22, 0), // ""
QT_MOC_LITERAL(23, 12), // "findPrevious"
QT_MOC_LITERAL(36, 10), // "replaceOne"
QT_MOC_LITERAL(47, 10), // "replaceAll"
QT_MOC_LITERAL(58, 14), // "countRequested"
QT_MOC_LITERAL(73, 16), // "findAllInCurrent"
QT_MOC_LITERAL(90, 16), // "findAllInAllOpen"
QT_MOC_LITERAL(107, 11), // "findInFiles"
QT_MOC_LITERAL(119, 14), // "replaceInFiles"
QT_MOC_LITERAL(134, 7), // "markAll"
QT_MOC_LITERAL(142, 10) // "clearMarks"

    },
    "SearchDialog\0findNext\0\0findPrevious\0"
    "replaceOne\0replaceAll\0countRequested\0"
    "findAllInCurrent\0findAllInAllOpen\0"
    "findInFiles\0replaceInFiles\0markAll\0"
    "clearMarks"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SearchDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   80,    2, 0x06,    1 /* Public */,
       3,    0,   81,    2, 0x06,    2 /* Public */,
       4,    0,   82,    2, 0x06,    3 /* Public */,
       5,    0,   83,    2, 0x06,    4 /* Public */,
       6,    0,   84,    2, 0x06,    5 /* Public */,
       7,    0,   85,    2, 0x06,    6 /* Public */,
       8,    0,   86,    2, 0x06,    7 /* Public */,
       9,    0,   87,    2, 0x06,    8 /* Public */,
      10,    0,   88,    2, 0x06,    9 /* Public */,
      11,    0,   89,    2, 0x06,   10 /* Public */,
      12,    0,   90,    2, 0x06,   11 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void SearchDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SearchDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->findNext(); break;
        case 1: _t->findPrevious(); break;
        case 2: _t->replaceOne(); break;
        case 3: _t->replaceAll(); break;
        case 4: _t->countRequested(); break;
        case 5: _t->findAllInCurrent(); break;
        case 6: _t->findAllInAllOpen(); break;
        case 7: _t->findInFiles(); break;
        case 8: _t->replaceInFiles(); break;
        case 9: _t->markAll(); break;
        case 10: _t->clearMarks(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::findNext)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::findPrevious)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::replaceOne)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::replaceAll)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::countRequested)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::findAllInCurrent)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::findAllInAllOpen)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::findInFiles)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::replaceInFiles)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::markAll)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (SearchDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchDialog::clearMarks)) {
                *result = 10;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject SearchDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_SearchDialog.offsetsAndSize,
    qt_meta_data_SearchDialog,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SearchDialog_t
, QtPrivate::TypeAndForceComplete<SearchDialog, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>



>,
    nullptr
} };


const QMetaObject *SearchDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SearchDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SearchDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int SearchDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void SearchDialog::findNext()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SearchDialog::findPrevious()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SearchDialog::replaceOne()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SearchDialog::replaceAll()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SearchDialog::countRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SearchDialog::findAllInCurrent()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SearchDialog::findAllInAllOpen()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SearchDialog::findInFiles()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void SearchDialog::replaceInFiles()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void SearchDialog::markAll()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void SearchDialog::clearMarks()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
