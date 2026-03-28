/****************************************************************************
** Meta object code from reading C++ file 'searchengine.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/core/searchengine.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'searchengine.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SearchEngine_t {
    const uint offsetsAndSize[28];
    char stringdata0[200];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SearchEngine_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SearchEngine_t qt_meta_stringdata_SearchEngine = {
    {
QT_MOC_LITERAL(0, 12), // "SearchEngine"
QT_MOC_LITERAL(13, 16), // "fileSearchResult"
QT_MOC_LITERAL(30, 0), // ""
QT_MOC_LITERAL(31, 12), // "SearchResult"
QT_MOC_LITERAL(44, 6), // "result"
QT_MOC_LITERAL(51, 18), // "fileSearchFinished"
QT_MOC_LITERAL(70, 9), // "totalHits"
QT_MOC_LITERAL(80, 10), // "totalFiles"
QT_MOC_LITERAL(91, 18), // "fileSearchProgress"
QT_MOC_LITERAL(110, 11), // "currentFile"
QT_MOC_LITERAL(122, 21), // "largeFileSearchResult"
QT_MOC_LITERAL(144, 23), // "largeFileSearchFinished"
QT_MOC_LITERAL(168, 23), // "largeFileSearchProgress"
QT_MOC_LITERAL(192, 7) // "percent"

    },
    "SearchEngine\0fileSearchResult\0\0"
    "SearchResult\0result\0fileSearchFinished\0"
    "totalHits\0totalFiles\0fileSearchProgress\0"
    "currentFile\0largeFileSearchResult\0"
    "largeFileSearchFinished\0largeFileSearchProgress\0"
    "percent"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SearchEngine[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       5,    2,   53,    2, 0x06,    3 /* Public */,
       8,    1,   58,    2, 0x06,    6 /* Public */,
      10,    1,   61,    2, 0x06,    8 /* Public */,
      11,    1,   64,    2, 0x06,   10 /* Public */,
      12,    1,   67,    2, 0x06,   12 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    6,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Int,   13,

       0        // eod
};

void SearchEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SearchEngine *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->fileSearchResult((*reinterpret_cast< std::add_pointer_t<SearchResult>>(_a[1]))); break;
        case 1: _t->fileSearchFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 2: _t->fileSearchProgress((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->largeFileSearchResult((*reinterpret_cast< std::add_pointer_t<SearchResult>>(_a[1]))); break;
        case 4: _t->largeFileSearchFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->largeFileSearchProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SearchEngine::*)(const SearchResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchEngine::fileSearchResult)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SearchEngine::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchEngine::fileSearchFinished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SearchEngine::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchEngine::fileSearchProgress)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SearchEngine::*)(const SearchResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchEngine::largeFileSearchResult)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SearchEngine::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchEngine::largeFileSearchFinished)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (SearchEngine::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchEngine::largeFileSearchProgress)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject SearchEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SearchEngine.offsetsAndSize,
    qt_meta_data_SearchEngine,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SearchEngine_t
, QtPrivate::TypeAndForceComplete<SearchEngine, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const SearchResult &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const SearchResult &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>



>,
    nullptr
} };


const QMetaObject *SearchEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SearchEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SearchEngine.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SearchEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void SearchEngine::fileSearchResult(const SearchResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SearchEngine::fileSearchFinished(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SearchEngine::fileSearchProgress(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SearchEngine::largeFileSearchResult(const SearchResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void SearchEngine::largeFileSearchFinished(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void SearchEngine::largeFileSearchProgress(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
