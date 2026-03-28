/****************************************************************************
** Meta object code from reading C++ file 'completionpopup.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/completionpopup.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'completionpopup.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CompletionPopup_t {
    const uint offsetsAndSize[14];
    char stringdata0[85];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_CompletionPopup_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_CompletionPopup_t qt_meta_stringdata_CompletionPopup = {
    {
QT_MOC_LITERAL(0, 15), // "CompletionPopup"
QT_MOC_LITERAL(16, 18), // "completionSelected"
QT_MOC_LITERAL(35, 0), // ""
QT_MOC_LITERAL(36, 10), // "completion"
QT_MOC_LITERAL(47, 15), // "onItemActivated"
QT_MOC_LITERAL(63, 16), // "QListWidgetItem*"
QT_MOC_LITERAL(80, 4) // "item"

    },
    "CompletionPopup\0completionSelected\0\0"
    "completion\0onItemActivated\0QListWidgetItem*\0"
    "item"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CompletionPopup[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   26,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   29,    2, 0x08,    3 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 5,    6,

       0        // eod
};

void CompletionPopup::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CompletionPopup *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->completionSelected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->onItemActivated((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CompletionPopup::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CompletionPopup::completionSelected)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject CompletionPopup::staticMetaObject = { {
    QMetaObject::SuperData::link<QListWidget::staticMetaObject>(),
    qt_meta_stringdata_CompletionPopup.offsetsAndSize,
    qt_meta_data_CompletionPopup,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_CompletionPopup_t
, QtPrivate::TypeAndForceComplete<CompletionPopup, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>


>,
    nullptr
} };


const QMetaObject *CompletionPopup::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CompletionPopup::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CompletionPopup.stringdata0))
        return static_cast<void*>(this);
    return QListWidget::qt_metacast(_clname);
}

int CompletionPopup::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QListWidget::qt_metacall(_c, _id, _a);
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
void CompletionPopup::completionSelected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
