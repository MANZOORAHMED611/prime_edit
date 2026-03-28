/****************************************************************************
** Meta object code from reading C++ file 'hexviewer.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/hexviewer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'hexviewer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_HexViewerWidget_t {
    const uint offsetsAndSize[14];
    char stringdata0[70];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_HexViewerWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_HexViewerWidget_t qt_meta_stringdata_HexViewerWidget = {
    {
QT_MOC_LITERAL(0, 15), // "HexViewerWidget"
QT_MOC_LITERAL(16, 21), // "cursorPositionChanged"
QT_MOC_LITERAL(38, 0), // ""
QT_MOC_LITERAL(39, 3), // "pos"
QT_MOC_LITERAL(43, 16), // "selectionChanged"
QT_MOC_LITERAL(60, 5), // "start"
QT_MOC_LITERAL(66, 3) // "end"

    },
    "HexViewerWidget\0cursorPositionChanged\0"
    "\0pos\0selectionChanged\0start\0end"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_HexViewerWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   26,    2, 0x06,    1 /* Public */,
       4,    2,   29,    2, 0x06,    3 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::LongLong,    3,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong,    5,    6,

       0        // eod
};

void HexViewerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<HexViewerWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->cursorPositionChanged((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 1: _t->selectionChanged((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (HexViewerWidget::*)(qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HexViewerWidget::cursorPositionChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (HexViewerWidget::*)(qint64 , qint64 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&HexViewerWidget::selectionChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject HexViewerWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_HexViewerWidget.offsetsAndSize,
    qt_meta_data_HexViewerWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_HexViewerWidget_t
, QtPrivate::TypeAndForceComplete<HexViewerWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<qint64, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<qint64, std::false_type>, QtPrivate::TypeAndForceComplete<qint64, std::false_type>



>,
    nullptr
} };


const QMetaObject *HexViewerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *HexViewerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_HexViewerWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int HexViewerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void HexViewerWidget::cursorPositionChanged(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void HexViewerWidget::selectionChanged(qint64 _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_HexViewer_t {
    const uint offsetsAndSize[14];
    char stringdata0[58];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_HexViewer_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_HexViewer_t qt_meta_stringdata_HexViewer = {
    {
QT_MOC_LITERAL(0, 9), // "HexViewer"
QT_MOC_LITERAL(10, 10), // "goToOffset"
QT_MOC_LITERAL(21, 0), // ""
QT_MOC_LITERAL(22, 6), // "offset"
QT_MOC_LITERAL(29, 4), // "find"
QT_MOC_LITERAL(34, 7), // "pattern"
QT_MOC_LITERAL(42, 15) // "exportSelection"

    },
    "HexViewer\0goToOffset\0\0offset\0find\0"
    "pattern\0exportSelection"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_HexViewer[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   32,    2, 0x0a,    1 /* Public */,
       4,    1,   35,    2, 0x0a,    3 /* Public */,
       6,    0,   38,    2, 0x0a,    5 /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::LongLong,    3,
    QMetaType::Void, QMetaType::QByteArray,    5,
    QMetaType::Void,

       0        // eod
};

void HexViewer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<HexViewer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->goToOffset((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 1: _t->find((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1]))); break;
        case 2: _t->exportSelection(); break;
        default: ;
        }
    }
}

const QMetaObject HexViewer::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_HexViewer.offsetsAndSize,
    qt_meta_data_HexViewer,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_HexViewer_t
, QtPrivate::TypeAndForceComplete<HexViewer, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<qint64, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *HexViewer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *HexViewer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_HexViewer.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int HexViewer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
