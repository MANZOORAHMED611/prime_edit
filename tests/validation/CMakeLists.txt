# Built from the top-level CMakeLists (PRIMEEDIT_BUILD_TESTS=ON); both suites
# link the same prime-edit-core object library as the app.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

foreach(suite test_runner crash_test)
    if(suite STREQUAL "test_runner")
        add_executable(${suite} test_all_features.cpp)
    else()
        add_executable(${suite} crash_test.cpp)
    endif()
    target_link_libraries(${suite} PRIVATE prime-edit-core Qt${QT_VERSION_MAJOR}::Test)
    add_test(NAME ${suite} COMMAND ${suite})
    # Headless, and never pointed at the user's real HOME/XDG dirs.
    set_tests_properties(${suite} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${CMAKE_CURRENT_BINARY_DIR}/home-${suite};XDG_CONFIG_HOME=${CMAKE_CURRENT_BINARY_DIR}/home-${suite}/.config;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/home-${suite}/.local/share;XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/home-${suite}/.cache"
        TIMEOUT 300)
endforeach()
