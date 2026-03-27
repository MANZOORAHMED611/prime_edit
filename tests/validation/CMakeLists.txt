cmake_minimum_required(VERSION 3.16)
project(PrimeEditTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 QUIET COMPONENTS Core Gui Widgets PrintSupport Network Svg Test)
if(NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets PrintSupport Network Svg Test)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src)

set(TEST_SOURCES
    test_all_features.cpp
    ${SRC_DIR}/core/piecetable.cpp
    ${SRC_DIR}/core/document.cpp
    ${SRC_DIR}/core/documentmanager.cpp
    ${SRC_DIR}/core/encoding.cpp
    ${SRC_DIR}/core/charsetdetector.cpp
    ${SRC_DIR}/core/searchengine.cpp
    ${SRC_DIR}/core/schemavalidator.cpp
    ${SRC_DIR}/core/largefile.cpp
    ${SRC_DIR}/core/macrorecorder.cpp
    ${SRC_DIR}/core/session.cpp
    ${SRC_DIR}/core/lspclient.cpp
    ${SRC_DIR}/core/lspmanager.cpp
    ${SRC_DIR}/core/pluginmanager.cpp
    ${SRC_DIR}/core/remoteconnection.cpp
    ${SRC_DIR}/core/llmevaluator.cpp
    ${SRC_DIR}/core/islamicbridge.cpp
    ${SRC_DIR}/syntax/highlighter.cpp
    ${SRC_DIR}/syntax/languagemanager.cpp
    ${SRC_DIR}/utils/settings.cpp
    ${SRC_DIR}/utils/fileutils.cpp
    ${SRC_DIR}/ui/theme.cpp
    ${SRC_DIR}/ui/editor.cpp
    ${SRC_DIR}/ui/linenumberarea.cpp
    ${SRC_DIR}/ui/tabwidget.cpp
    ${SRC_DIR}/ui/statusbar.cpp
    ${SRC_DIR}/ui/mainwindow.cpp
    ${SRC_DIR}/ui/mainwindow_extended.cpp
    ${SRC_DIR}/ui/toolbarmanager.cpp
    ${SRC_DIR}/ui/searchdialog.cpp
    ${SRC_DIR}/ui/searchresultspanel.cpp
    ${SRC_DIR}/ui/incrementalsearchbar.cpp
    ${SRC_DIR}/ui/completionpopup.cpp
    ${SRC_DIR}/ui/columneditor.cpp
    ${SRC_DIR}/ui/documentmap.cpp
    ${SRC_DIR}/ui/functionlist.cpp
    ${SRC_DIR}/ui/notificationbar.cpp
    ${SRC_DIR}/ui/validationpanel.cpp
    ${SRC_DIR}/ui/evalresultwidget.cpp
    ${SRC_DIR}/ui/endpointconfigdialog.cpp
    ${SRC_DIR}/ui/macrodialog.cpp
    ${SRC_DIR}/ui/recoverydialog.cpp
    ${SRC_DIR}/ui/lspbridge.cpp
    ${SRC_DIR}/ui/preferencesdialog.cpp
    ${SRC_DIR}/ui/commandpalette.cpp
    ${SRC_DIR}/ui/terminalwidget.cpp
    ${SRC_DIR}/ui/fileexplorer.cpp
    ${SRC_DIR}/ui/gitstatusdelegate.cpp
    ${SRC_DIR}/ui/gitdiffviewer.cpp
    ${SRC_DIR}/ui/hexviewer.cpp
    ${SRC_DIR}/ui/markdownpreview.cpp
    ${SRC_DIR}/ui/themeeditor.cpp
    ${SRC_DIR}/ui/remoteconnectiondialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/resources.qrc
)

add_executable(test_runner ${TEST_SOURCES})

target_include_directories(test_runner PRIVATE ${SRC_DIR})

target_link_libraries(test_runner PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Test
)

find_package(ICU QUIET COMPONENTS uc data)
if(ICU_FOUND)
    target_link_libraries(test_runner PRIVATE ICU::uc ICU::data)
    target_compile_definitions(test_runner PRIVATE HAVE_ICU=1)
endif()
