/****************************************************************************
** Meta object code from reading C++ file 'lspclient.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/lspclient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lspclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LSPClient_t {
    const uint offsetsAndSize[56];
    char stringdata0[362];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_LSPClient_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_LSPClient_t qt_meta_stringdata_LSPClient = {
    {
QT_MOC_LITERAL(0, 9), // "LSPClient"
QT_MOC_LITERAL(10, 11), // "initialized"
QT_MOC_LITERAL(22, 0), // ""
QT_MOC_LITERAL(23, 16), // "shutdownComplete"
QT_MOC_LITERAL(40, 16), // "completionResult"
QT_MOC_LITERAL(57, 21), // "QList<CompletionItem>"
QT_MOC_LITERAL(79, 5), // "items"
QT_MOC_LITERAL(85, 11), // "hoverResult"
QT_MOC_LITERAL(97, 7), // "content"
QT_MOC_LITERAL(105, 16), // "definitionResult"
QT_MOC_LITERAL(122, 8), // "Location"
QT_MOC_LITERAL(131, 8), // "location"
QT_MOC_LITERAL(140, 16), // "referencesResult"
QT_MOC_LITERAL(157, 15), // "QList<Location>"
QT_MOC_LITERAL(173, 9), // "locations"
QT_MOC_LITERAL(183, 20), // "diagnosticsPublished"
QT_MOC_LITERAL(204, 3), // "uri"
QT_MOC_LITERAL(208, 17), // "QList<Diagnostic>"
QT_MOC_LITERAL(226, 11), // "diagnostics"
QT_MOC_LITERAL(238, 12), // "renameResult"
QT_MOC_LITERAL(251, 13), // "workspaceEdit"
QT_MOC_LITERAL(265, 13), // "errorOccurred"
QT_MOC_LITERAL(279, 5), // "error"
QT_MOC_LITERAL(285, 11), // "onReadyRead"
QT_MOC_LITERAL(297, 14), // "onProcessError"
QT_MOC_LITERAL(312, 22), // "QProcess::ProcessError"
QT_MOC_LITERAL(335, 17), // "onProcessFinished"
QT_MOC_LITERAL(353, 8) // "exitCode"

    },
    "LSPClient\0initialized\0\0shutdownComplete\0"
    "completionResult\0QList<CompletionItem>\0"
    "items\0hoverResult\0content\0definitionResult\0"
    "Location\0location\0referencesResult\0"
    "QList<Location>\0locations\0"
    "diagnosticsPublished\0uri\0QList<Diagnostic>\0"
    "diagnostics\0renameResult\0workspaceEdit\0"
    "errorOccurred\0error\0onReadyRead\0"
    "onProcessError\0QProcess::ProcessError\0"
    "onProcessFinished\0exitCode"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LSPClient[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   86,    2, 0x06,    1 /* Public */,
       3,    0,   87,    2, 0x06,    2 /* Public */,
       4,    1,   88,    2, 0x06,    3 /* Public */,
       7,    1,   91,    2, 0x06,    5 /* Public */,
       9,    1,   94,    2, 0x06,    7 /* Public */,
      12,    1,   97,    2, 0x06,    9 /* Public */,
      15,    2,  100,    2, 0x06,   11 /* Public */,
      19,    1,  105,    2, 0x06,   14 /* Public */,
      21,    1,  108,    2, 0x06,   16 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      23,    0,  111,    2, 0x08,   18 /* Private */,
      24,    1,  112,    2, 0x08,   19 /* Private */,
      26,    1,  115,    2, 0x08,   21 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 17,   16,   18,
    QMetaType::Void, QMetaType::QJsonObject,   20,
    QMetaType::Void, QMetaType::QString,   22,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 25,   22,
    QMetaType::Void, QMetaType::Int,   27,

       0        // eod
};

void LSPClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LSPClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->initialized(); break;
        case 1: _t->shutdownComplete(); break;
        case 2: _t->completionResult((*reinterpret_cast< std::add_pointer_t<QList<CompletionItem>>>(_a[1]))); break;
        case 3: _t->hoverResult((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->definitionResult((*reinterpret_cast< std::add_pointer_t<Location>>(_a[1]))); break;
        case 5: _t->referencesResult((*reinterpret_cast< std::add_pointer_t<QList<Location>>>(_a[1]))); break;
        case 6: _t->diagnosticsPublished((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<Diagnostic>>>(_a[2]))); break;
        case 7: _t->renameResult((*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[1]))); break;
        case 8: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->onReadyRead(); break;
        case 10: _t->onProcessError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 11: _t->onProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LSPClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::initialized)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::shutdownComplete)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QVector<CompletionItem> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::completionResult)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::hoverResult)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const Location & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::definitionResult)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QVector<Location> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::referencesResult)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QString & , const QVector<Diagnostic> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::diagnosticsPublished)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::renameResult)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (LSPClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&LSPClient::errorOccurred)) {
                *result = 8;
                return;
            }
        }
    }
}

const QMetaObject LSPClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_LSPClient.offsetsAndSize,
    qt_meta_data_LSPClient,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_LSPClient_t
, QtPrivate::TypeAndForceComplete<LSPClient, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QVector<CompletionItem> &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const Location &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QVector<Location> &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<const QVector<Diagnostic> &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QProcess::ProcessError, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>


>,
    nullptr
} };


const QMetaObject *LSPClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LSPClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LSPClient.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int LSPClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void LSPClient::initialized()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LSPClient::shutdownComplete()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void LSPClient::completionResult(const QVector<CompletionItem> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void LSPClient::hoverResult(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void LSPClient::definitionResult(const Location & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void LSPClient::referencesResult(const QVector<Location> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void LSPClient::diagnosticsPublished(const QString & _t1, const QVector<Diagnostic> & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void LSPClient::renameResult(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void LSPClient::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
