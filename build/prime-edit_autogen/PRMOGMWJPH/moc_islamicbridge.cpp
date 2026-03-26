/****************************************************************************
** Meta object code from reading C++ file 'islamicbridge.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/islamicbridge.h"
#include <QtNetwork/QSslError>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'islamicbridge.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_IslamicBridge_t {
    const uint offsetsAndSize[38];
    char stringdata0[223];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_IslamicBridge_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_IslamicBridge_t qt_meta_stringdata_IslamicBridge = {
    {
QT_MOC_LITERAL(0, 13), // "IslamicBridge"
QT_MOC_LITERAL(14, 15), // "hadithValidated"
QT_MOC_LITERAL(30, 0), // ""
QT_MOC_LITERAL(31, 16), // "HadithValidation"
QT_MOC_LITERAL(48, 6), // "result"
QT_MOC_LITERAL(55, 14), // "quranValidated"
QT_MOC_LITERAL(70, 15), // "QuranValidation"
QT_MOC_LITERAL(86, 13), // "scholarResult"
QT_MOC_LITERAL(100, 4), // "name"
QT_MOC_LITERAL(105, 4), // "info"
QT_MOC_LITERAL(110, 16), // "connectionTested"
QT_MOC_LITERAL(127, 7), // "success"
QT_MOC_LITERAL(135, 7), // "message"
QT_MOC_LITERAL(143, 13), // "errorOccurred"
QT_MOC_LITERAL(157, 5), // "error"
QT_MOC_LITERAL(163, 13), // "onHadithReply"
QT_MOC_LITERAL(177, 12), // "onQuranReply"
QT_MOC_LITERAL(190, 14), // "onScholarReply"
QT_MOC_LITERAL(205, 17) // "onConnectionReply"

    },
    "IslamicBridge\0hadithValidated\0\0"
    "HadithValidation\0result\0quranValidated\0"
    "QuranValidation\0scholarResult\0name\0"
    "info\0connectionTested\0success\0message\0"
    "errorOccurred\0error\0onHadithReply\0"
    "onQuranReply\0onScholarReply\0"
    "onConnectionReply"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_IslamicBridge[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   68,    2, 0x06,    1 /* Public */,
       5,    1,   71,    2, 0x06,    3 /* Public */,
       7,    2,   74,    2, 0x06,    5 /* Public */,
      10,    2,   79,    2, 0x06,    8 /* Public */,
      13,    1,   84,    2, 0x06,   11 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      15,    0,   87,    2, 0x08,   13 /* Private */,
      16,    0,   88,    2, 0x08,   14 /* Private */,
      17,    0,   89,    2, 0x08,   15 /* Private */,
      18,    0,   90,    2, 0x08,   16 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    8,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   11,   12,
    QMetaType::Void, QMetaType::QString,   14,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void IslamicBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<IslamicBridge *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->hadithValidated((*reinterpret_cast< std::add_pointer_t<HadithValidation>>(_a[1]))); break;
        case 1: _t->quranValidated((*reinterpret_cast< std::add_pointer_t<QuranValidation>>(_a[1]))); break;
        case 2: _t->scholarResult((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->connectionTested((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onHadithReply(); break;
        case 6: _t->onQuranReply(); break;
        case 7: _t->onScholarReply(); break;
        case 8: _t->onConnectionReply(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (IslamicBridge::*)(const HadithValidation & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IslamicBridge::hadithValidated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (IslamicBridge::*)(const QuranValidation & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IslamicBridge::quranValidated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (IslamicBridge::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IslamicBridge::scholarResult)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (IslamicBridge::*)(bool , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IslamicBridge::connectionTested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (IslamicBridge::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IslamicBridge::errorOccurred)) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject IslamicBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_IslamicBridge.offsetsAndSize,
    qt_meta_data_IslamicBridge,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_IslamicBridge_t
, QtPrivate::TypeAndForceComplete<IslamicBridge, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const HadithValidation &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QuranValidation &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *IslamicBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *IslamicBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_IslamicBridge.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int IslamicBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void IslamicBridge::hadithValidated(const HadithValidation & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void IslamicBridge::quranValidated(const QuranValidation & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void IslamicBridge::scholarResult(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void IslamicBridge::connectionTested(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void IslamicBridge::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
