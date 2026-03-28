cmake_minimum_required(VERSION 3.16)
project(PrimeEdit VERSION 1.0.0 LANGUAGES CXX)

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
    Concurrent
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
        Concurrent
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
    src/core/charsetdetector.cpp
    src/core/gitgutter.cpp
    src/core/llmevaluator.cpp
    src/core/islamicbridge.cpp
    src/core/schemavalidator.cpp
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
    src/ui/gitcommitdialog.cpp
    src/ui/toolbarmanager.cpp
    src/ui/columneditor.cpp
    src/ui/notificationbar.cpp
    src/ui/recoverydialog.cpp
    src/ui/functionlist.cpp
    src/ui/macrodialog.cpp
    src/ui/documentmap.cpp
    src/ui/lspbridge.cpp
    src/ui/evalresultwidget.cpp
    src/ui/endpointconfigdialog.cpp
    src/ui/validationpanel.cpp
    src/ui/plugindialog.cpp
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
    src/core/charsetdetector.h
    src/core/gitgutter.h
    src/core/llmevaluator.h
    src/core/islamicbridge.h
    src/core/schemavalidator.h
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
    src/ui/gitcommitdialog.h
    src/ui/toolbarmanager.h
    src/ui/columneditor.h
    src/ui/notificationbar.h
    src/ui/recoverydialog.h
    src/ui/functionlist.h
    src/ui/macrodialog.h
    src/ui/documentmap.h
    src/ui/lspbridge.h
    src/ui/evalresultwidget.h
    src/ui/endpointconfigdialog.h
    src/ui/validationpanel.h
    src/ui/plugindialog.h
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
add_executable(prime-edit
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${SYNTAX_SOURCES}
    ${UTILS_SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

set_target_properties(prime-edit PROPERTIES OUTPUT_NAME "PrimeEdit")

target_include_directories(prime-edit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(prime-edit PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Concurrent
)

# Optional ICU support for enhanced charset detection
find_package(ICU QUIET COMPONENTS uc data)
if(ICU_FOUND)
    target_link_libraries(prime-edit PRIVATE ICU::uc ICU::data)
    target_compile_definitions(prime-edit PRIVATE HAVE_ICU=1)
endif()

# Install rules
install(TARGETS prime-edit
    RUNTIME DESTINATION bin
)

install(FILES resources/prime-edit.desktop
    DESTINATION share/applications
)

install(FILES resources/icons/notepad-supreme.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME prime-edit.svg
)

install(FILES resources/icons/prime_edit_icon.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME prime-edit.png
)

# Post-build: install icon + desktop file for dev use
add_custom_command(TARGET prime-edit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/resources/icons/prime_edit_icon.png
        $<TARGET_FILE_DIR:prime-edit>/prime-edit.png
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $ENV{HOME}/.local/share/icons/hicolor/256x256/apps
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/resources/icons/prime_edit_icon.png
        $ENV{HOME}/.local/share/icons/hicolor/256x256/apps/prime-edit.png
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $ENV{HOME}/.local/share/applications
    COMMENT "Installing PrimeEdit icon and launcher"
)

# Generate desktop file with absolute paths for dev builds
file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/prime-edit.desktop CONTENT
"[Desktop Entry]
Name=PrimeEdit
Comment=The editor for structured knowledge work
GenericName=Text Editor
Exec=$<TARGET_FILE:prime-edit> %F
Icon=$ENV{HOME}/.local/share/icons/hicolor/256x256/apps/prime-edit.png
Terminal=false
Type=Application
Categories=Utility;TextEditor;Development;
StartupWMClass=PrimeEdit
"
)

# Copy desktop file to user applications dir after build
add_custom_command(TARGET prime-edit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/prime-edit.desktop
        $ENV{HOME}/.local/share/applications/prime-edit.desktop
    COMMENT "Installing PrimeEdit desktop launcher"
)
