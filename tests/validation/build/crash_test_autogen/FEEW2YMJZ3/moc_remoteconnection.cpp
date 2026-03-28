/****************************************************************************
** Meta object code from reading C++ file 'remoteconnection.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/core/remoteconnection.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'remoteconnection.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RemoteConnection_t {
    const uint offsetsAndSize[34];
    char stringdata0[205];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_RemoteConnection_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_RemoteConnection_t qt_meta_stringdata_RemoteConnection = {
    {
QT_MOC_LITERAL(0, 16), // "RemoteConnection"
QT_MOC_LITERAL(17, 9), // "connected"
QT_MOC_LITERAL(27, 0), // ""
QT_MOC_LITERAL(28, 12), // "disconnected"
QT_MOC_LITERAL(41, 15), // "connectionError"
QT_MOC_LITERAL(57, 5), // "error"
QT_MOC_LITERAL(63, 16), // "downloadComplete"
QT_MOC_LITERAL(80, 10), // "remotePath"
QT_MOC_LITERAL(91, 9), // "localPath"
QT_MOC_LITERAL(101, 14), // "uploadComplete"
QT_MOC_LITERAL(116, 15), // "directoryListed"
QT_MOC_LITERAL(132, 21), // "QList<RemoteFileInfo>"
QT_MOC_LITERAL(154, 5), // "files"
QT_MOC_LITERAL(160, 14), // "operationError"
QT_MOC_LITERAL(175, 9), // "operation"
QT_MOC_LITERAL(185, 8), // "progress"
QT_MOC_LITERAL(194, 10) // "percentage"

    },
    "RemoteConnection\0connected\0\0disconnected\0"
    "connectionError\0error\0downloadComplete\0"
    "remotePath\0localPath\0uploadComplete\0"
    "directoryListed\0QList<RemoteFileInfo>\0"
    "files\0operationError\0operation\0progress\0"
    "percentage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RemoteConnection[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    1 /* Public */,
       3,    0,   63,    2, 0x06,    2 /* Public */,
       4,    1,   64,    2, 0x06,    3 /* Public */,
       6,    2,   67,    2, 0x06,    5 /* Public */,
       9,    1,   72,    2, 0x06,    8 /* Public */,
      10,    1,   75,    2, 0x06,   10 /* Public */,
      13,    2,   78,    2, 0x06,   12 /* Public */,
      15,    1,   83,    2, 0x06,   15 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   14,    5,
    QMetaType::Void, QMetaType::Int,   16,

       0        // eod
};

void RemoteConnection::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RemoteConnection *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->connectionError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->downloadComplete((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->uploadComplete((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->directoryListed((*reinterpret_cast< std::add_pointer_t<QList<RemoteFileInfo>>>(_a[1]))); break;
        case 6: _t->operationError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->progress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RemoteConnection::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteConnection::connected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RemoteConnection::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteConnection::disconnected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (RemoteConnection::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteConnection::connectionError)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (RemoteConnection::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteConnection::downloadComplete)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (RemoteConnection::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteConnection::uploadComplete)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (RemoteConnection::*)(const QVector<RemoteFileInfo> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteConnection::directoryListed)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (RemoteConnection::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteConnection::operationError)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (RemoteConnection::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteConnection::progress)) {
                *result = 7;
                return;
            }
        }
    }
}

const QMetaObject RemoteConnection::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_RemoteConnection.offsetsAndSize,
    qt_meta_data_RemoteConnection,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_RemoteConnection_t
, QtPrivate::TypeAndForceComplete<RemoteConnection, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QVector<RemoteFileInfo> &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>



>,
    nullptr
} };


const QMetaObject *RemoteConnection::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RemoteConnection::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RemoteConnection.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int RemoteConnection::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void RemoteConnection::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void RemoteConnection::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void RemoteConnection::connectionError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void RemoteConnection::downloadComplete(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void RemoteConnection::uploadComplete(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void RemoteConnection::directoryListed(const QVector<RemoteFileInfo> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void RemoteConnection::operationError(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void RemoteConnection::progress(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
struct qt_meta_stringdata_RemoteFileManager_t {
    const uint offsetsAndSize[14];
    char stringdata0[79];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_RemoteFileManager_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_RemoteFileManager_t qt_meta_stringdata_RemoteFileManager = {
    {
QT_MOC_LITERAL(0, 17), // "RemoteFileManager"
QT_MOC_LITERAL(18, 16), // "remoteFileOpened"
QT_MOC_LITERAL(35, 0), // ""
QT_MOC_LITERAL(36, 9), // "localPath"
QT_MOC_LITERAL(46, 10), // "remotePath"
QT_MOC_LITERAL(57, 15), // "remoteFileSaved"
QT_MOC_LITERAL(73, 5) // "error"

    },
    "RemoteFileManager\0remoteFileOpened\0\0"
    "localPath\0remotePath\0remoteFileSaved\0"
    "error"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RemoteFileManager[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   32,    2, 0x06,    1 /* Public */,
       5,    1,   37,    2, 0x06,    4 /* Public */,
       6,    1,   40,    2, 0x06,    6 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString,    6,

       0        // eod
};

void RemoteFileManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RemoteFileManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->remoteFileOpened((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->remoteFileSaved((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->error((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RemoteFileManager::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteFileManager::remoteFileOpened)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RemoteFileManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteFileManager::remoteFileSaved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (RemoteFileManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RemoteFileManager::error)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject RemoteFileManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_RemoteFileManager.offsetsAndSize,
    qt_meta_data_RemoteFileManager,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_RemoteFileManager_t
, QtPrivate::TypeAndForceComplete<RemoteFileManager, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>



>,
    nullptr
} };


const QMetaObject *RemoteFileManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RemoteFileManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RemoteFileManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int RemoteFileManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void RemoteFileManager::remoteFileOpened(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void RemoteFileManager::remoteFileSaved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void RemoteFileManager::error(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
