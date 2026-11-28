find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

add_executable(tst_markdown tst_markdown.cpp)
target_include_directories(tst_markdown PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../validation)
target_link_libraries(tst_markdown PRIVATE prime-edit-core Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME tst_markdown COMMAND tst_markdown)
set_tests_properties(tst_markdown PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${CMAKE_CURRENT_BINARY_DIR}/home;XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/home/.config;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/home/.local/share;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/home/.cache"
    TIMEOUT 120)
