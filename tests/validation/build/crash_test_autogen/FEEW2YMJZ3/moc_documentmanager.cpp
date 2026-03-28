/****************************************************************************
** Meta object code from reading C++ file 'documentmanager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/core/documentmanager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'documentmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DocumentManager_t {
    const uint offsetsAndSize[32];
    char stringdata0[228];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_DocumentManager_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_DocumentManager_t qt_meta_stringdata_DocumentManager = {
    {
QT_MOC_LITERAL(0, 15), // "DocumentManager"
QT_MOC_LITERAL(16, 15), // "documentCreated"
QT_MOC_LITERAL(32, 0), // ""
QT_MOC_LITERAL(33, 9), // "Document*"
QT_MOC_LITERAL(43, 8), // "document"
QT_MOC_LITERAL(52, 14), // "documentOpened"
QT_MOC_LITERAL(67, 14), // "documentClosed"
QT_MOC_LITERAL(82, 23), // "documentModifiedChanged"
QT_MOC_LITERAL(106, 18), // "recentFilesChanged"
QT_MOC_LITERAL(125, 22), // "fileExternallyModified"
QT_MOC_LITERAL(148, 8), // "filePath"
QT_MOC_LITERAL(157, 25), // "onDocumentModifiedChanged"
QT_MOC_LITERAL(183, 8), // "modified"
QT_MOC_LITERAL(192, 16), // "saveRecoveryData"
QT_MOC_LITERAL(209, 13), // "onFileChanged"
QT_MOC_LITERAL(223, 4) // "path"

    },
    "DocumentManager\0documentCreated\0\0"
    "Document*\0document\0documentOpened\0"
    "documentClosed\0documentModifiedChanged\0"
    "recentFilesChanged\0fileExternallyModified\0"
    "filePath\0onDocumentModifiedChanged\0"
    "modified\0saveRecoveryData\0onFileChanged\0"
    "path"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DocumentManager[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   68,    2, 0x06,    1 /* Public */,
       5,    1,   71,    2, 0x06,    3 /* Public */,
       6,    1,   74,    2, 0x06,    5 /* Public */,
       7,    1,   77,    2, 0x06,    7 /* Public */,
       8,    0,   80,    2, 0x06,    9 /* Public */,
       9,    1,   81,    2, 0x06,   10 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      11,    1,   84,    2, 0x08,   12 /* Private */,
      13,    0,   87,    2, 0x08,   14 /* Private */,
      14,    1,   88,    2, 0x08,   15 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   10,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   15,

       0        // eod
};

void DocumentManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DocumentManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->documentCreated((*reinterpret_cast< std::add_pointer_t<Document*>>(_a[1]))); break;
        case 1: _t->documentOpened((*reinterpret_cast< std::add_pointer_t<Document*>>(_a[1]))); break;
        case 2: _t->documentClosed((*reinterpret_cast< std::add_pointer_t<Document*>>(_a[1]))); break;
        case 3: _t->documentModifiedChanged((*reinterpret_cast< std::add_pointer_t<Document*>>(_a[1]))); break;
        case 4: _t->recentFilesChanged(); break;
        case 5: _t->fileExternallyModified((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->onDocumentModifiedChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->saveRecoveryData(); break;
        case 8: _t->onFileChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Document* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Document* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Document* >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< Document* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DocumentManager::*)(Document * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DocumentManager::documentCreated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DocumentManager::*)(Document * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DocumentManager::documentOpened)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DocumentManager::*)(Document * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DocumentManager::documentClosed)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DocumentManager::*)(Document * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DocumentManager::documentModifiedChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DocumentManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DocumentManager::recentFilesChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (DocumentManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DocumentManager::fileExternallyModified)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject DocumentManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_DocumentManager.offsetsAndSize,
    qt_meta_data_DocumentManager,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_DocumentManager_t
, QtPrivate::TypeAndForceComplete<DocumentManager, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<Document *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<Document *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<Document *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<Document *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>


>,
    nullptr
} };


const QMetaObject *DocumentManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DocumentManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DocumentManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DocumentManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void DocumentManager::documentCreated(Document * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DocumentManager::documentOpened(Document * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DocumentManager::documentClosed(Document * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DocumentManager::documentModifiedChanged(Document * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void DocumentManager::recentFilesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void DocumentManager::fileExternallyModified(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
