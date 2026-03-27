/****************************************************************************
** Meta object code from reading C++ file 'tabwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/tabwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tabwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TabBar_t {
    const uint offsetsAndSize[16];
    char stringdata0[112];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_TabBar_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_TabBar_t qt_meta_stringdata_TabBar = {
    {
QT_MOC_LITERAL(0, 6), // "TabBar"
QT_MOC_LITERAL(7, 15), // "newTabRequested"
QT_MOC_LITERAL(23, 0), // ""
QT_MOC_LITERAL(24, 20), // "closeOthersRequested"
QT_MOC_LITERAL(45, 5), // "index"
QT_MOC_LITERAL(51, 21), // "closeToRightRequested"
QT_MOC_LITERAL(73, 20), // "closeToLeftRequested"
QT_MOC_LITERAL(94, 17) // "closeAllRequested"

    },
    "TabBar\0newTabRequested\0\0closeOthersRequested\0"
    "index\0closeToRightRequested\0"
    "closeToLeftRequested\0closeAllRequested"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TabBar[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    1 /* Public */,
       3,    1,   45,    2, 0x06,    2 /* Public */,
       5,    1,   48,    2, 0x06,    4 /* Public */,
       6,    1,   51,    2, 0x06,    6 /* Public */,
       7,    0,   54,    2, 0x06,    8 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void,

       0        // eod
};

void TabBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TabBar *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->newTabRequested(); break;
        case 1: _t->closeOthersRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->closeToRightRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->closeToLeftRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->closeAllRequested(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TabBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TabBar::newTabRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TabBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TabBar::closeOthersRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TabBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TabBar::closeToRightRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (TabBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TabBar::closeToLeftRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (TabBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TabBar::closeAllRequested)) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject TabBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QTabBar::staticMetaObject>(),
    qt_meta_stringdata_TabBar.offsetsAndSize,
    qt_meta_data_TabBar,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_TabBar_t
, QtPrivate::TypeAndForceComplete<TabBar, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>



>,
    nullptr
} };


const QMetaObject *TabBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TabBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TabBar.stringdata0))
        return static_cast<void*>(this);
    return QTabBar::qt_metacast(_clname);
}

int TabBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTabBar::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void TabBar::newTabRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TabBar::closeOthersRequested(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void TabBar::closeToRightRequested(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void TabBar::closeToLeftRequested(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void TabBar::closeAllRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
struct qt_meta_stringdata_TabWidget_t {
    const uint offsetsAndSize[16];
    char stringdata0[123];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_TabWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_TabWidget_t qt_meta_stringdata_TabWidget = {
    {
QT_MOC_LITERAL(0, 9), // "TabWidget"
QT_MOC_LITERAL(10, 15), // "newTabRequested"
QT_MOC_LITERAL(26, 0), // ""
QT_MOC_LITERAL(27, 22), // "onCloseOthersRequested"
QT_MOC_LITERAL(50, 5), // "index"
QT_MOC_LITERAL(56, 23), // "onCloseToRightRequested"
QT_MOC_LITERAL(80, 22), // "onCloseToLeftRequested"
QT_MOC_LITERAL(103, 19) // "onCloseAllRequested"

    },
    "TabWidget\0newTabRequested\0\0"
    "onCloseOthersRequested\0index\0"
    "onCloseToRightRequested\0onCloseToLeftRequested\0"
    "onCloseAllRequested"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TabWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    1,   45,    2, 0x08,    2 /* Private */,
       5,    1,   48,    2, 0x08,    4 /* Private */,
       6,    1,   51,    2, 0x08,    6 /* Private */,
       7,    0,   54,    2, 0x08,    8 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void,

       0        // eod
};

void TabWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TabWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->newTabRequested(); break;
        case 1: _t->onCloseOthersRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->onCloseToRightRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->onCloseToLeftRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->onCloseAllRequested(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TabWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TabWidget::newTabRequested)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject TabWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QTabWidget::staticMetaObject>(),
    qt_meta_stringdata_TabWidget.offsetsAndSize,
    qt_meta_data_TabWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_TabWidget_t
, QtPrivate::TypeAndForceComplete<TabWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *TabWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TabWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TabWidget.stringdata0))
        return static_cast<void*>(this);
    return QTabWidget::qt_metacast(_clname);
}

int TabWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTabWidget::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void TabWidget::newTabRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
