/****************************************************************************
** Meta object code from reading C++ file 'incrementalsearchbar.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/incrementalsearchbar.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'incrementalsearchbar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_IncrementalSearchBar_t {
    const uint offsetsAndSize[14];
    char stringdata0[70];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_IncrementalSearchBar_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_IncrementalSearchBar_t qt_meta_stringdata_IncrementalSearchBar = {
    {
QT_MOC_LITERAL(0, 20), // "IncrementalSearchBar"
QT_MOC_LITERAL(21, 13), // "searchChanged"
QT_MOC_LITERAL(35, 0), // ""
QT_MOC_LITERAL(36, 4), // "text"
QT_MOC_LITERAL(41, 8), // "findNext"
QT_MOC_LITERAL(50, 12), // "findPrevious"
QT_MOC_LITERAL(63, 6) // "closed"

    },
    "IncrementalSearchBar\0searchChanged\0\0"
    "text\0findNext\0findPrevious\0closed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_IncrementalSearchBar[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   38,    2, 0x06,    1 /* Public */,
       4,    0,   41,    2, 0x06,    3 /* Public */,
       5,    0,   42,    2, 0x06,    4 /* Public */,
       6,    0,   43,    2, 0x06,    5 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void IncrementalSearchBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<IncrementalSearchBar *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->searchChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->findNext(); break;
        case 2: _t->findPrevious(); break;
        case 3: _t->closed(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (IncrementalSearchBar::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IncrementalSearchBar::searchChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (IncrementalSearchBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IncrementalSearchBar::findNext)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (IncrementalSearchBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IncrementalSearchBar::findPrevious)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (IncrementalSearchBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IncrementalSearchBar::closed)) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject IncrementalSearchBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_IncrementalSearchBar.offsetsAndSize,
    qt_meta_data_IncrementalSearchBar,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_IncrementalSearchBar_t
, QtPrivate::TypeAndForceComplete<IncrementalSearchBar, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>



>,
    nullptr
} };


const QMetaObject *IncrementalSearchBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *IncrementalSearchBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_IncrementalSearchBar.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int IncrementalSearchBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void IncrementalSearchBar::searchChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void IncrementalSearchBar::findNext()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void IncrementalSearchBar::findPrevious()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void IncrementalSearchBar::closed()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
