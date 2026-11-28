# Throwaway spike: build Scintilla's own Qt layer (ScintillaEditBase +
# ScintillaEdit) and Lexilla from source against system Qt 6, plus a demo app.
# Upstream ships only qmake .pro files; this file mirrors
# scintilla/qt/ScintillaEdit/ScintillaEdit.pro and lexilla/src/Lexilla.pro.
#
#   cmake -S spikes/scintilla -B <build> -G Ninja -DCMAKE_BUILD_TYPE=Release \
#         -DSCINTILLA_DIR=<src>/scintilla -DLEXILLA_DIR=<src>/lexilla
cmake_minimum_required(VERSION 3.16)
project(scintilla_spike LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(SCINTILLA_DIR "" CACHE PATH "Extracted scintilla/ directory (with generated ScintillaEdit.cpp)")
set(LEXILLA_DIR "" CACHE PATH "Extracted lexilla/ directory")
foreach(req SCINTILLA_DIR LEXILLA_DIR)
    if(NOT EXISTS "${${req}}/version.txt")
        message(FATAL_ERROR "${req} must point at an extracted source tree (got '${${req}}')")
    endif()
endforeach()
if(NOT EXISTS "${SCINTILLA_DIR}/qt/ScintillaEdit/ScintillaEdit.cpp")
    message(FATAL_ERROR "ScintillaEdit.cpp missing: run fetch_sources.sh (it runs WidgetGen.py)")
endif()

# Core5Compat: ScintillaEditBase uses QTextCodec for non-UTF-8 code pages.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Core5Compat)

# ---- Scintilla core + Qt layer ------------------------------------------
file(GLOB SCI_CORE_SOURCES "${SCINTILLA_DIR}/src/*.cxx")
set(SCI_QT "${SCINTILLA_DIR}/qt")
add_library(scintilla_qt STATIC
    ${SCI_CORE_SOURCES}
    ${SCI_QT}/ScintillaEditBase/PlatQt.cpp
    ${SCI_QT}/ScintillaEditBase/ScintillaQt.cpp
    ${SCI_QT}/ScintillaEditBase/ScintillaEditBase.cpp
    ${SCI_QT}/ScintillaEditBase/ScintillaEditBase.h
    ${SCI_QT}/ScintillaEditBase/ScintillaQt.h
    ${SCI_QT}/ScintillaEdit/ScintillaEdit.cpp
    ${SCI_QT}/ScintillaEdit/ScintillaEdit.h
    ${SCI_QT}/ScintillaEdit/ScintillaDocument.cpp
    ${SCI_QT}/ScintillaEdit/ScintillaDocument.h)
target_include_directories(scintilla_qt PUBLIC
    ${SCINTILLA_DIR}/include ${SCINTILLA_DIR}/src
    ${SCI_QT}/ScintillaEditBase ${SCI_QT}/ScintillaEdit)
target_compile_definitions(scintilla_qt PUBLIC SCINTILLA_QT=1 PRIVATE MAKING_LIBRARY=1
    $<$<CONFIG:Release>:NDEBUG=1>)
target_link_libraries(scintilla_qt PUBLIC Qt6::Widgets Qt6::Core5Compat)

# ---- Lexilla (all lexers, linked statically) ----------------------------
file(GLOB LEXILLA_SOURCES
    "${LEXILLA_DIR}/lexlib/*.cxx" "${LEXILLA_DIR}/lexers/*.cxx" "${LEXILLA_DIR}/src/Lexilla.cxx")
add_library(lexilla STATIC ${LEXILLA_SOURCES})
target_include_directories(lexilla PUBLIC ${LEXILLA_DIR}/include
    PRIVATE ${LEXILLA_DIR}/lexlib ${SCINTILLA_DIR}/include)
target_compile_definitions(lexilla PRIVATE $<$<CONFIG:Release>:NDEBUG=1>)

# ---- Demo ----------------------------------------------------------------
add_executable(scintilla_demo
    demo/main.cpp
    demo/demosetup.cpp demo/demosetup.h
    demo/changetracker.cpp demo/changetracker.h
    demo/toysave.cpp demo/toysave.h
    demo/selftest.cpp demo/selftest.h
    demo/bench.cpp demo/bench.h)
target_link_libraries(scintilla_demo PRIVATE scintilla_qt lexilla)
