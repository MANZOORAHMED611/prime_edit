cmake_minimum_required(VERSION 3.16)
project(OliveNotepad VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt - prefer Qt6, fall back to Qt5
find_package(Qt6 QUIET COMPONENTS
    Core
    Gui
    Widgets
    PrintSupport
    Network
    Svg
)

if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found, trying Qt5...")
    find_package(Qt5 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        PrintSupport
        Network
        Svg
    )
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

# Define source files
set(CORE_SOURCES
    src/core/document.cpp
    src/core/documentmanager.cpp
    src/core/encoding.cpp
    src/core/piecetable.cpp
    src/core/session.cpp
    src/core/macrorecorder.cpp
    src/core/lspclient.cpp
    src/core/lspmanager.cpp
    src/core/pluginmanager.cpp
    src/core/remoteconnection.cpp
    src/core/largefile.cpp
    src/core/searchengine.cpp
)

set(UI_SOURCES
    src/ui/mainwindow.cpp
    src/ui/mainwindow_extended.cpp
    src/ui/tabwidget.cpp
    src/ui/editor.cpp
    src/ui/statusbar.cpp
    src/ui/searchdialog.cpp
    src/ui/searchresultspanel.cpp
    src/ui/incrementalsearchbar.cpp
    src/ui/preferencesdialog.cpp
    src/ui/linenumberarea.cpp
    src/ui/fileexplorer.cpp
    src/ui/commandpalette.cpp
    src/ui/gitstatusdelegate.cpp
    src/ui/terminalwidget.cpp
    src/ui/completionpopup.cpp
    src/ui/theme.cpp
    src/ui/themeeditor.cpp
    src/ui/markdownpreview.cpp
    src/ui/hexviewer.cpp
    src/ui/remoteconnectiondialog.cpp
    src/ui/gitdiffviewer.cpp
    src/ui/toolbarmanager.cpp
    src/ui/columneditor.cpp
)

set(SYNTAX_SOURCES
    src/syntax/highlighter.cpp
    src/syntax/languagemanager.cpp
)

set(UTILS_SOURCES
    src/utils/fileutils.cpp
    src/utils/settings.cpp
)

set(HEADERS
    src/core/document.h
    src/core/documentmanager.h
    src/core/encoding.h
    src/core/piecetable.h
    src/core/session.h
    src/core/macrorecorder.h
    src/core/lspclient.h
    src/core/lspmanager.h
    src/core/plugininterface.h
    src/core/pluginmanager.h
    src/core/remoteconnection.h
    src/core/largefile.h
    src/core/searchengine.h
    src/ui/mainwindow.h
    src/ui/tabwidget.h
    src/ui/editor.h
    src/ui/statusbar.h
    src/ui/searchdialog.h
    src/ui/searchresultspanel.h
    src/ui/incrementalsearchbar.h
    src/ui/preferencesdialog.h
    src/ui/linenumberarea.h
    src/ui/fileexplorer.h
    src/ui/commandpalette.h
    src/ui/gitstatusdelegate.h
    src/ui/terminalwidget.h
    src/ui/completionpopup.h
    src/ui/theme.h
    src/ui/themeeditor.h
    src/ui/markdownpreview.h
    src/ui/hexviewer.h
    src/ui/remoteconnectiondialog.h
    src/ui/gitdiffviewer.h
    src/ui/toolbarmanager.h
    src/ui/columneditor.h
    src/syntax/highlighter.h
    src/syntax/languagemanager.h
    src/utils/fileutils.h
    src/utils/settings.h
)

# Resources
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(olive-notepad
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${SYNTAX_SOURCES}
    ${UTILS_SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

set_target_properties(olive-notepad PROPERTIES OUTPUT_NAME "Olive Notepad")

target_include_directories(olive-notepad PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(olive-notepad PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
)

# Install rules
install(TARGETS olive-notepad
    RUNTIME DESTINATION bin
)

install(FILES resources/olive-notepad.desktop
    DESTINATION share/applications
)

install(FILES resources/icons/notepad-supreme.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME olive-notepad.svg
)

install(FILES resources/icons/olive_notepad_icon.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME olive-notepad.png
)
