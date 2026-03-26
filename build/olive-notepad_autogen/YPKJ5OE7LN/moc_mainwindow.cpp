/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    const uint offsetsAndSize[256];
    char stringdata0[1835];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 10), // "MainWindow"
QT_MOC_LITERAL(11, 13), // "convertToANSI"
QT_MOC_LITERAL(25, 0), // ""
QT_MOC_LITERAL(26, 13), // "convertToUTF8"
QT_MOC_LITERAL(40, 16), // "convertToUTF8BOM"
QT_MOC_LITERAL(57, 15), // "convertToUCS2BE"
QT_MOC_LITERAL(73, 15), // "convertToUCS2LE"
QT_MOC_LITERAL(89, 16), // "convertToWindows"
QT_MOC_LITERAL(106, 13), // "convertToUnix"
QT_MOC_LITERAL(120, 12), // "convertToMac"
QT_MOC_LITERAL(133, 7), // "newFile"
QT_MOC_LITERAL(141, 8), // "openFile"
QT_MOC_LITERAL(150, 8), // "filePath"
QT_MOC_LITERAL(159, 8), // "saveFile"
QT_MOC_LITERAL(168, 10), // "saveFileAs"
QT_MOC_LITERAL(179, 12), // "saveAllFiles"
QT_MOC_LITERAL(192, 9), // "closeFile"
QT_MOC_LITERAL(202, 5), // "index"
QT_MOC_LITERAL(208, 13), // "closeAllFiles"
QT_MOC_LITERAL(222, 9), // "printFile"
QT_MOC_LITERAL(232, 14), // "reloadFromDisk"
QT_MOC_LITERAL(247, 14), // "increaseIndent"
QT_MOC_LITERAL(262, 14), // "decreaseIndent"
QT_MOC_LITERAL(277, 8), // "goToLine"
QT_MOC_LITERAL(286, 4), // "line"
QT_MOC_LITERAL(291, 12), // "onTabChanged"
QT_MOC_LITERAL(304, 19), // "onTabCloseRequested"
QT_MOC_LITERAL(324, 18), // "onDocumentModified"
QT_MOC_LITERAL(343, 8), // "modified"
QT_MOC_LITERAL(352, 23), // "onCursorPositionChanged"
QT_MOC_LITERAL(376, 4), // "undo"
QT_MOC_LITERAL(381, 4), // "redo"
QT_MOC_LITERAL(386, 3), // "cut"
QT_MOC_LITERAL(390, 4), // "copy"
QT_MOC_LITERAL(395, 5), // "paste"
QT_MOC_LITERAL(401, 9), // "selectAll"
QT_MOC_LITERAL(411, 13), // "duplicateLine"
QT_MOC_LITERAL(425, 10), // "deleteLine"
QT_MOC_LITERAL(436, 10), // "moveLineUp"
QT_MOC_LITERAL(447, 12), // "moveLineDown"
QT_MOC_LITERAL(460, 13), // "toggleComment"
QT_MOC_LITERAL(474, 12), // "blockComment"
QT_MOC_LITERAL(487, 13), // "streamComment"
QT_MOC_LITERAL(501, 18), // "sortLinesAscending"
QT_MOC_LITERAL(520, 19), // "sortLinesDescending"
QT_MOC_LITERAL(540, 20), // "removeDuplicateLines"
QT_MOC_LITERAL(561, 16), // "removeEmptyLines"
QT_MOC_LITERAL(578, 26), // "removeEmptyLinesWithBlanks"
QT_MOC_LITERAL(605, 31), // "removeConsecutiveDuplicateLines"
QT_MOC_LITERAL(637, 9), // "joinLines"
QT_MOC_LITERAL(647, 10), // "splitLines"
QT_MOC_LITERAL(658, 22), // "trimTrailingWhitespace"
QT_MOC_LITERAL(681, 21), // "trimLeadingWhitespace"
QT_MOC_LITERAL(703, 22), // "trimLeadingAndTrailing"
QT_MOC_LITERAL(726, 10), // "eolToSpace"
QT_MOC_LITERAL(737, 23), // "removeUnnecessaryBlanks"
QT_MOC_LITERAL(761, 10), // "tabToSpace"
QT_MOC_LITERAL(772, 13), // "spaceToTabAll"
QT_MOC_LITERAL(786, 17), // "spaceToTabLeading"
QT_MOC_LITERAL(804, 11), // "toUpperCase"
QT_MOC_LITERAL(816, 11), // "toLowerCase"
QT_MOC_LITERAL(828, 11), // "toTitleCase"
QT_MOC_LITERAL(840, 14), // "toSentenceCase"
QT_MOC_LITERAL(855, 12), // "toInvertCase"
QT_MOC_LITERAL(868, 12), // "toRandomCase"
QT_MOC_LITERAL(881, 4), // "find"
QT_MOC_LITERAL(886, 8), // "findNext"
QT_MOC_LITERAL(895, 12), // "findPrevious"
QT_MOC_LITERAL(908, 7), // "replace"
QT_MOC_LITERAL(916, 14), // "goToLineDialog"
QT_MOC_LITERAL(931, 21), // "showIncrementalSearch"
QT_MOC_LITERAL(953, 23), // "onSearchResultActivated"
QT_MOC_LITERAL(977, 18), // "onFindAllInCurrent"
QT_MOC_LITERAL(996, 18), // "onFindAllInAllOpen"
QT_MOC_LITERAL(1015, 13), // "onFindInFiles"
QT_MOC_LITERAL(1029, 16), // "onCountRequested"
QT_MOC_LITERAL(1046, 26), // "onIncrementalSearchChanged"
QT_MOC_LITERAL(1073, 4), // "text"
QT_MOC_LITERAL(1078, 14), // "toggleWordWrap"
QT_MOC_LITERAL(1093, 17), // "toggleLineNumbers"
QT_MOC_LITERAL(1111, 16), // "toggleWhitespace"
QT_MOC_LITERAL(1128, 15), // "toggleEndOfLine"
QT_MOC_LITERAL(1144, 17), // "toggleIndentGuide"
QT_MOC_LITERAL(1162, 16), // "toggleWrapSymbol"
QT_MOC_LITERAL(1179, 14), // "toggleTerminal"
QT_MOC_LITERAL(1194, 16), // "toggleFullScreen"
QT_MOC_LITERAL(1211, 21), // "toggleDistractionFree"
QT_MOC_LITERAL(1233, 18), // "syncVerticalScroll"
QT_MOC_LITERAL(1252, 20), // "syncHorizontalScroll"
QT_MOC_LITERAL(1273, 6), // "zoomIn"
QT_MOC_LITERAL(1280, 7), // "zoomOut"
QT_MOC_LITERAL(1288, 9), // "resetZoom"
QT_MOC_LITERAL(1298, 9), // "showAbout"
QT_MOC_LITERAL(1308, 15), // "showPreferences"
QT_MOC_LITERAL(1324, 18), // "showCommandPalette"
QT_MOC_LITERAL(1343, 19), // "startRecordingMacro"
QT_MOC_LITERAL(1363, 18), // "stopRecordingMacro"
QT_MOC_LITERAL(1382, 13), // "playbackMacro"
QT_MOC_LITERAL(1396, 9), // "saveMacro"
QT_MOC_LITERAL(1406, 9), // "loadMacro"
QT_MOC_LITERAL(1416, 14), // "openRecentFile"
QT_MOC_LITERAL(1431, 16), // "clearRecentFiles"
QT_MOC_LITERAL(1448, 21), // "updateRecentFilesMenu"
QT_MOC_LITERAL(1470, 14), // "toggleBookmark"
QT_MOC_LITERAL(1485, 12), // "nextBookmark"
QT_MOC_LITERAL(1498, 16), // "previousBookmark"
QT_MOC_LITERAL(1515, 17), // "clearAllBookmarks"
QT_MOC_LITERAL(1533, 7), // "foldAll"
QT_MOC_LITERAL(1541, 9), // "unfoldAll"
QT_MOC_LITERAL(1551, 10), // "toggleFold"
QT_MOC_LITERAL(1562, 16), // "foldCurrentLevel"
QT_MOC_LITERAL(1579, 18), // "unfoldCurrentLevel"
QT_MOC_LITERAL(1598, 14), // "closeOtherTabs"
QT_MOC_LITERAL(1613, 16), // "closeTabsToRight"
QT_MOC_LITERAL(1630, 15), // "closeTabsToLeft"
QT_MOC_LITERAL(1646, 15), // "moveToOtherView"
QT_MOC_LITERAL(1662, 16), // "cloneToOtherView"
QT_MOC_LITERAL(1679, 11), // "previousTab"
QT_MOC_LITERAL(1691, 7), // "nextTab"
QT_MOC_LITERAL(1699, 10), // "columnMode"
QT_MOC_LITERAL(1710, 12), // "columnEditor"
QT_MOC_LITERAL(1723, 14), // "beginEndSelect"
QT_MOC_LITERAL(1738, 16), // "launchInTerminal"
QT_MOC_LITERAL(1755, 20), // "openContainingFolder"
QT_MOC_LITERAL(1776, 16), // "updateWindowMenu"
QT_MOC_LITERAL(1793, 11), // "switchToTab"
QT_MOC_LITERAL(1805, 17), // "toggleAlwaysOnTop"
QT_MOC_LITERAL(1823, 11) // "showSummary"

    },
    "MainWindow\0convertToANSI\0\0convertToUTF8\0"
    "convertToUTF8BOM\0convertToUCS2BE\0"
    "convertToUCS2LE\0convertToWindows\0"
    "convertToUnix\0convertToMac\0newFile\0"
    "openFile\0filePath\0saveFile\0saveFileAs\0"
    "saveAllFiles\0closeFile\0index\0closeAllFiles\0"
    "printFile\0reloadFromDisk\0increaseIndent\0"
    "decreaseIndent\0goToLine\0line\0onTabChanged\0"
    "onTabCloseRequested\0onDocumentModified\0"
    "modified\0onCursorPositionChanged\0undo\0"
    "redo\0cut\0copy\0paste\0selectAll\0"
    "duplicateLine\0deleteLine\0moveLineUp\0"
    "moveLineDown\0toggleComment\0blockComment\0"
    "streamComment\0sortLinesAscending\0"
    "sortLinesDescending\0removeDuplicateLines\0"
    "removeEmptyLines\0removeEmptyLinesWithBlanks\0"
    "removeConsecutiveDuplicateLines\0"
    "joinLines\0splitLines\0trimTrailingWhitespace\0"
    "trimLeadingWhitespace\0trimLeadingAndTrailing\0"
    "eolToSpace\0removeUnnecessaryBlanks\0"
    "tabToSpace\0spaceToTabAll\0spaceToTabLeading\0"
    "toUpperCase\0toLowerCase\0toTitleCase\0"
    "toSentenceCase\0toInvertCase\0toRandomCase\0"
    "find\0findNext\0findPrevious\0replace\0"
    "goToLineDialog\0showIncrementalSearch\0"
    "onSearchResultActivated\0onFindAllInCurrent\0"
    "onFindAllInAllOpen\0onFindInFiles\0"
    "onCountRequested\0onIncrementalSearchChanged\0"
    "text\0toggleWordWrap\0toggleLineNumbers\0"
    "toggleWhitespace\0toggleEndOfLine\0"
    "toggleIndentGuide\0toggleWrapSymbol\0"
    "toggleTerminal\0toggleFullScreen\0"
    "toggleDistractionFree\0syncVerticalScroll\0"
    "syncHorizontalScroll\0zoomIn\0zoomOut\0"
    "resetZoom\0showAbout\0showPreferences\0"
    "showCommandPalette\0startRecordingMacro\0"
    "stopRecordingMacro\0playbackMacro\0"
    "saveMacro\0loadMacro\0openRecentFile\0"
    "clearRecentFiles\0updateRecentFilesMenu\0"
    "toggleBookmark\0nextBookmark\0"
    "previousBookmark\0clearAllBookmarks\0"
    "foldAll\0unfoldAll\0toggleFold\0"
    "foldCurrentLevel\0unfoldCurrentLevel\0"
    "closeOtherTabs\0closeTabsToRight\0"
    "closeTabsToLeft\0moveToOtherView\0"
    "cloneToOtherView\0previousTab\0nextTab\0"
    "columnMode\0columnEditor\0beginEndSelect\0"
    "launchInTerminal\0openContainingFolder\0"
    "updateWindowMenu\0switchToTab\0"
    "toggleAlwaysOnTop\0showSummary"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
     123,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  752,    2, 0x0a,    1 /* Public */,
       3,    0,  753,    2, 0x0a,    2 /* Public */,
       4,    0,  754,    2, 0x0a,    3 /* Public */,
       5,    0,  755,    2, 0x0a,    4 /* Public */,
       6,    0,  756,    2, 0x0a,    5 /* Public */,
       7,    0,  757,    2, 0x0a,    6 /* Public */,
       8,    0,  758,    2, 0x0a,    7 /* Public */,
       9,    0,  759,    2, 0x0a,    8 /* Public */,
      10,    0,  760,    2, 0x0a,    9 /* Public */,
      11,    1,  761,    2, 0x0a,   10 /* Public */,
      11,    0,  764,    2, 0x2a,   12 /* Public | MethodCloned */,
      13,    0,  765,    2, 0x0a,   13 /* Public */,
      14,    0,  766,    2, 0x0a,   14 /* Public */,
      15,    0,  767,    2, 0x0a,   15 /* Public */,
      16,    1,  768,    2, 0x0a,   16 /* Public */,
      16,    0,  771,    2, 0x2a,   18 /* Public | MethodCloned */,
      18,    0,  772,    2, 0x0a,   19 /* Public */,
      19,    0,  773,    2, 0x0a,   20 /* Public */,
      20,    0,  774,    2, 0x0a,   21 /* Public */,
      21,    0,  775,    2, 0x0a,   22 /* Public */,
      22,    0,  776,    2, 0x0a,   23 /* Public */,
      23,    1,  777,    2, 0x0a,   24 /* Public */,
      25,    1,  780,    2, 0x08,   26 /* Private */,
      26,    1,  783,    2, 0x08,   28 /* Private */,
      27,    1,  786,    2, 0x08,   30 /* Private */,
      29,    0,  789,    2, 0x08,   32 /* Private */,
      30,    0,  790,    2, 0x08,   33 /* Private */,
      31,    0,  791,    2, 0x08,   34 /* Private */,
      32,    0,  792,    2, 0x08,   35 /* Private */,
      33,    0,  793,    2, 0x08,   36 /* Private */,
      34,    0,  794,    2, 0x08,   37 /* Private */,
      35,    0,  795,    2, 0x08,   38 /* Private */,
      36,    0,  796,    2, 0x08,   39 /* Private */,
      37,    0,  797,    2, 0x08,   40 /* Private */,
      38,    0,  798,    2, 0x08,   41 /* Private */,
      39,    0,  799,    2, 0x08,   42 /* Private */,
      40,    0,  800,    2, 0x08,   43 /* Private */,
      41,    0,  801,    2, 0x08,   44 /* Private */,
      42,    0,  802,    2, 0x08,   45 /* Private */,
      43,    0,  803,    2, 0x08,   46 /* Private */,
      44,    0,  804,    2, 0x08,   47 /* Private */,
      45,    0,  805,    2, 0x08,   48 /* Private */,
      46,    0,  806,    2, 0x08,   49 /* Private */,
      47,    0,  807,    2, 0x08,   50 /* Private */,
      48,    0,  808,    2, 0x08,   51 /* Private */,
      49,    0,  809,    2, 0x08,   52 /* Private */,
      50,    0,  810,    2, 0x08,   53 /* Private */,
      51,    0,  811,    2, 0x08,   54 /* Private */,
      52,    0,  812,    2, 0x08,   55 /* Private */,
      53,    0,  813,    2, 0x08,   56 /* Private */,
      54,    0,  814,    2, 0x08,   57 /* Private */,
      55,    0,  815,    2, 0x08,   58 /* Private */,
      56,    0,  816,    2, 0x08,   59 /* Private */,
      57,    0,  817,    2, 0x08,   60 /* Private */,
      58,    0,  818,    2, 0x08,   61 /* Private */,
      59,    0,  819,    2, 0x08,   62 /* Private */,
      60,    0,  820,    2, 0x08,   63 /* Private */,
      61,    0,  821,    2, 0x08,   64 /* Private */,
      62,    0,  822,    2, 0x08,   65 /* Private */,
      63,    0,  823,    2, 0x08,   66 /* Private */,
      64,    0,  824,    2, 0x08,   67 /* Private */,
      65,    0,  825,    2, 0x08,   68 /* Private */,
      66,    0,  826,    2, 0x08,   69 /* Private */,
      67,    0,  827,    2, 0x08,   70 /* Private */,
      68,    0,  828,    2, 0x08,   71 /* Private */,
      69,    0,  829,    2, 0x08,   72 /* Private */,
      70,    0,  830,    2, 0x08,   73 /* Private */,
      71,    2,  831,    2, 0x08,   74 /* Private */,
      72,    0,  836,    2, 0x08,   77 /* Private */,
      73,    0,  837,    2, 0x08,   78 /* Private */,
      74,    0,  838,    2, 0x08,   79 /* Private */,
      75,    0,  839,    2, 0x08,   80 /* Private */,
      76,    1,  840,    2, 0x08,   81 /* Private */,
      78,    0,  843,    2, 0x08,   83 /* Private */,
      79,    0,  844,    2, 0x08,   84 /* Private */,
      80,    0,  845,    2, 0x08,   85 /* Private */,
      81,    0,  846,    2, 0x08,   86 /* Private */,
      82,    0,  847,    2, 0x08,   87 /* Private */,
      83,    0,  848,    2, 0x08,   88 /* Private */,
      84,    0,  849,    2, 0x08,   89 /* Private */,
      85,    0,  850,    2, 0x08,   90 /* Private */,
      86,    0,  851,    2, 0x08,   91 /* Private */,
      87,    0,  852,    2, 0x08,   92 /* Private */,
      88,    0,  853,    2, 0x08,   93 /* Private */,
      89,    0,  854,    2, 0x08,   94 /* Private */,
      90,    0,  855,    2, 0x08,   95 /* Private */,
      91,    0,  856,    2, 0x08,   96 /* Private */,
      92,    0,  857,    2, 0x08,   97 /* Private */,
      93,    0,  858,    2, 0x08,   98 /* Private */,
      94,    0,  859,    2, 0x08,   99 /* Private */,
      95,    0,  860,    2, 0x08,  100 /* Private */,
      96,    0,  861,    2, 0x08,  101 /* Private */,
      97,    0,  862,    2, 0x08,  102 /* Private */,
      98,    0,  863,    2, 0x08,  103 /* Private */,
      99,    0,  864,    2, 0x08,  104 /* Private */,
     100,    0,  865,    2, 0x08,  105 /* Private */,
     101,    0,  866,    2, 0x08,  106 /* Private */,
     102,    0,  867,    2, 0x08,  107 /* Private */,
     103,    0,  868,    2, 0x08,  108 /* Private */,
     104,    0,  869,    2, 0x08,  109 /* Private */,
     105,    0,  870,    2, 0x08,  110 /* Private */,
     106,    0,  871,    2, 0x08,  111 /* Private */,
     107,    0,  872,    2, 0x08,  112 /* Private */,
     108,    0,  873,    2, 0x08,  113 /* Private */,
     109,    0,  874,    2, 0x08,  114 /* Private */,
     110,    0,  875,    2, 0x08,  115 /* Private */,
     111,    0,  876,    2, 0x08,  116 /* Private */,
     112,    0,  877,    2, 0x08,  117 /* Private */,
     113,    0,  878,    2, 0x08,  118 /* Private */,
     114,    0,  879,    2, 0x08,  119 /* Private */,
     115,    0,  880,    2, 0x08,  120 /* Private */,
     116,    0,  881,    2, 0x08,  121 /* Private */,
     117,    0,  882,    2, 0x08,  122 /* Private */,
     118,    0,  883,    2, 0x08,  123 /* Private */,
     119,    0,  884,    2, 0x08,  124 /* Private */,
     120,    0,  885,    2, 0x08,  125 /* Private */,
     121,    0,  886,    2, 0x08,  126 /* Private */,
     122,    0,  887,    2, 0x08,  127 /* Private */,
     123,    0,  888,    2, 0x08,  128 /* Private */,
     124,    0,  889,    2, 0x08,  129 /* Private */,
     125,    1,  890,    2, 0x08,  130 /* Private */,
     126,    0,  893,    2, 0x08,  132 /* Private */,
     127,    0,  894,    2, 0x08,  133 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::Int,   17,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   24,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void, QMetaType::Bool,   28,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   12,   24,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   77,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->convertToANSI(); break;
        case 1: _t->convertToUTF8(); break;
        case 2: _t->convertToUTF8BOM(); break;
        case 3: _t->convertToUCS2BE(); break;
        case 4: _t->convertToUCS2LE(); break;
        case 5: _t->convertToWindows(); break;
        case 6: _t->convertToUnix(); break;
        case 7: _t->convertToMac(); break;
        case 8: _t->newFile(); break;
        case 9: _t->openFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->openFile(); break;
        case 11: _t->saveFile(); break;
        case 12: _t->saveFileAs(); break;
        case 13: _t->saveAllFiles(); break;
        case 14: { bool _r = _t->closeFile((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 15: { bool _r = _t->closeFile();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 16: { bool _r = _t->closeAllFiles();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 17: _t->printFile(); break;
        case 18: _t->reloadFromDisk(); break;
        case 19: _t->increaseIndent(); break;
        case 20: _t->decreaseIndent(); break;
        case 21: _t->goToLine((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 22: _t->onTabChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 23: _t->onTabCloseRequested((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 24: _t->onDocumentModified((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 25: _t->onCursorPositionChanged(); break;
        case 26: _t->undo(); break;
        case 27: _t->redo(); break;
        case 28: _t->cut(); break;
        case 29: _t->copy(); break;
        case 30: _t->paste(); break;
        case 31: _t->selectAll(); break;
        case 32: _t->duplicateLine(); break;
        case 33: _t->deleteLine(); break;
        case 34: _t->moveLineUp(); break;
        case 35: _t->moveLineDown(); break;
        case 36: _t->toggleComment(); break;
        case 37: _t->blockComment(); break;
        case 38: _t->streamComment(); break;
        case 39: _t->sortLinesAscending(); break;
        case 40: _t->sortLinesDescending(); break;
        case 41: _t->removeDuplicateLines(); break;
        case 42: _t->removeEmptyLines(); break;
        case 43: _t->removeEmptyLinesWithBlanks(); break;
        case 44: _t->removeConsecutiveDuplicateLines(); break;
        case 45: _t->joinLines(); break;
        case 46: _t->splitLines(); break;
        case 47: _t->trimTrailingWhitespace(); break;
        case 48: _t->trimLeadingWhitespace(); break;
        case 49: _t->trimLeadingAndTrailing(); break;
        case 50: _t->eolToSpace(); break;
        case 51: _t->removeUnnecessaryBlanks(); break;
        case 52: _t->tabToSpace(); break;
        case 53: _t->spaceToTabAll(); break;
        case 54: _t->spaceToTabLeading(); break;
        case 55: _t->toUpperCase(); break;
        case 56: _t->toLowerCase(); break;
        case 57: _t->toTitleCase(); break;
        case 58: _t->toSentenceCase(); break;
        case 59: _t->toInvertCase(); break;
        case 60: _t->toRandomCase(); break;
        case 61: _t->find(); break;
        case 62: _t->findNext(); break;
        case 63: _t->findPrevious(); break;
        case 64: _t->replace(); break;
        case 65: _t->goToLineDialog(); break;
        case 66: _t->showIncrementalSearch(); break;
        case 67: _t->onSearchResultActivated((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 68: _t->onFindAllInCurrent(); break;
        case 69: _t->onFindAllInAllOpen(); break;
        case 70: _t->onFindInFiles(); break;
        case 71: _t->onCountRequested(); break;
        case 72: _t->onIncrementalSearchChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 73: _t->toggleWordWrap(); break;
        case 74: _t->toggleLineNumbers(); break;
        case 75: _t->toggleWhitespace(); break;
        case 76: _t->toggleEndOfLine(); break;
        case 77: _t->toggleIndentGuide(); break;
        case 78: _t->toggleWrapSymbol(); break;
        case 79: _t->toggleTerminal(); break;
        case 80: _t->toggleFullScreen(); break;
        case 81: _t->toggleDistractionFree(); break;
        case 82: _t->syncVerticalScroll(); break;
        case 83: _t->syncHorizontalScroll(); break;
        case 84: _t->zoomIn(); break;
        case 85: _t->zoomOut(); break;
        case 86: _t->resetZoom(); break;
        case 87: _t->showAbout(); break;
        case 88: _t->showPreferences(); break;
        case 89: _t->showCommandPalette(); break;
        case 90: _t->startRecordingMacro(); break;
        case 91: _t->stopRecordingMacro(); break;
        case 92: _t->playbackMacro(); break;
        case 93: _t->saveMacro(); break;
        case 94: _t->loadMacro(); break;
        case 95: _t->openRecentFile(); break;
        case 96: _t->clearRecentFiles(); break;
        case 97: _t->updateRecentFilesMenu(); break;
        case 98: _t->toggleBookmark(); break;
        case 99: _t->nextBookmark(); break;
        case 100: _t->previousBookmark(); break;
        case 101: _t->clearAllBookmarks(); break;
        case 102: _t->foldAll(); break;
        case 103: _t->unfoldAll(); break;
        case 104: _t->toggleFold(); break;
        case 105: _t->foldCurrentLevel(); break;
        case 106: _t->unfoldCurrentLevel(); break;
        case 107: _t->closeOtherTabs(); break;
        case 108: _t->closeTabsToRight(); break;
        case 109: _t->closeTabsToLeft(); break;
        case 110: _t->moveToOtherView(); break;
        case 111: _t->cloneToOtherView(); break;
        case 112: _t->previousTab(); break;
        case 113: _t->nextTab(); break;
        case 114: _t->columnMode(); break;
        case 115: _t->columnEditor(); break;
        case 116: _t->beginEndSelect(); break;
        case 117: _t->launchInTerminal(); break;
        case 118: _t->openContainingFolder(); break;
        case 119: _t->updateWindowMenu(); break;
        case 120: _t->switchToTab((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 121: _t->toggleAlwaysOnTop(); break;
        case 122: _t->showSummary(); break;
        default: ;
        }
    }
}

const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSize,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t
, QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 123)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 123;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 123)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 123;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
