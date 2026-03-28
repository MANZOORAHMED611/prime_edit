/****************************************************************************
** Meta object code from reading C++ file 'fileexplorer.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/fileexplorer.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fileexplorer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FileExplorer_t {
    const uint offsetsAndSize[32];
    char stringdata0[178];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_FileExplorer_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_FileExplorer_t qt_meta_stringdata_FileExplorer = {
    {
QT_MOC_LITERAL(0, 12), // "FileExplorer"
QT_MOC_LITERAL(13, 17), // "fileDoubleClicked"
QT_MOC_LITERAL(31, 0), // ""
QT_MOC_LITERAL(32, 8), // "filePath"
QT_MOC_LITERAL(41, 12), // "fileSelected"
QT_MOC_LITERAL(54, 15), // "rootPathChanged"
QT_MOC_LITERAL(70, 4), // "path"
QT_MOC_LITERAL(75, 19), // "onItemDoubleClicked"
QT_MOC_LITERAL(95, 11), // "QModelIndex"
QT_MOC_LITERAL(107, 5), // "index"
QT_MOC_LITERAL(113, 13), // "onItemClicked"
QT_MOC_LITERAL(127, 12), // "onOpenFolder"
QT_MOC_LITERAL(140, 6), // "onGoUp"
QT_MOC_LITERAL(147, 9), // "onRefresh"
QT_MOC_LITERAL(157, 15), // "onFilterChanged"
QT_MOC_LITERAL(173, 4) // "text"

    },
    "FileExplorer\0fileDoubleClicked\0\0"
    "filePath\0fileSelected\0rootPathChanged\0"
    "path\0onItemDoubleClicked\0QModelIndex\0"
    "index\0onItemClicked\0onOpenFolder\0"
    "onGoUp\0onRefresh\0onFilterChanged\0text"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FileExplorer[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   68,    2, 0x06,    1 /* Public */,
       4,    1,   71,    2, 0x06,    3 /* Public */,
       5,    1,   74,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    1,   77,    2, 0x08,    7 /* Private */,
      10,    1,   80,    2, 0x08,    9 /* Private */,
      11,    0,   83,    2, 0x08,   11 /* Private */,
      12,    0,   84,    2, 0x08,   12 /* Private */,
      13,    0,   85,    2, 0x08,   13 /* Private */,
      14,    1,   86,    2, 0x08,   14 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   15,

       0        // eod
};

void FileExplorer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FileExplorer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->fileDoubleClicked((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->fileSelected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->rootPathChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onItemDoubleClicked((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 4: _t->onItemClicked((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 5: _t->onOpenFolder(); break;
        case 6: _t->onGoUp(); break;
        case 7: _t->onRefresh(); break;
        case 8: _t->onFilterChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FileExplorer::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FileExplorer::fileDoubleClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FileExplorer::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FileExplorer::fileSelected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (FileExplorer::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FileExplorer::rootPathChanged)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject FileExplorer::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_FileExplorer.offsetsAndSize,
    qt_meta_data_FileExplorer,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_FileExplorer_t
, QtPrivate::TypeAndForceComplete<FileExplorer, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>


>,
    nullptr
} };


const QMetaObject *FileExplorer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FileExplorer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FileExplorer.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int FileExplorer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void FileExplorer::fileDoubleClicked(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FileExplorer::fileSelected(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void FileExplorer::rootPathChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
