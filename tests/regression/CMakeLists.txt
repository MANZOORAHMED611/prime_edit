# Phase 1 regression suites (spec docs/superpowers/specs/2026-11-26-phase1-data-loss-fixes.md,
# section 9.2). One executable per file, registered by explicit name: a suite that
# is missing from disk fails the configure instead of silently not running.
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

set(REGRESSION_SUITES
    test_t1_1_save
    test_t1_2_flags
    test_t1_3_encoding
    test_t1_4_persistence
    test_t1_5_cache
    test_t1_6_instance
    test_t1_7_watch
    test_t1_10_session_autosave
    test_t1_11a_direct_write
    test_t1_11b_drafts
    test_t1_12_short_write
    test_t2_1_modified
    test_t2_2_sync
    test_t2_3_folding
    test_t2_4_readonly
    test_t3_1_split
    test_t3_2_save_ui
    test_t3_3_encoding_ui
    test_t3_4_reload
    test_i1_roundtrip
    test_npp_qw1
)

foreach(suite IN LISTS REGRESSION_SUITES)
    add_executable(${suite} ${suite}.cpp support.h)
    target_include_directories(${suite} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../validation)
    target_link_libraries(${suite} PRIVATE prime-edit-core Qt${QT_VERSION_MAJOR}::Test)
    # Process tests (T1.6) launch $PRIMEEDIT_BIN, so building a suite builds the app.
    add_dependencies(${suite} prime-edit)
    add_test(NAME ${suite} COMMAND ${suite})

    # Same isolation as tests/validation, plus a scratch XDG_RUNTIME_DIR so the
    # single-instance socket can never reach a live editor. Qt requires 0700.
    set(home ${CMAKE_CURRENT_BINARY_DIR}/home-${suite})
    file(MAKE_DIRECTORY ${home}/run)
    # chmod, not file(CHMOD): that needs CMake 3.19 and the minimum is 3.16.
    execute_process(COMMAND chmod 700 ${home}/run)
    set_tests_properties(${suite} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;HOME=${home};XDG_CONFIG_HOME=${home}/.config;XDG_DATA_HOME=${home}/.local/share;XDG_CACHE_HOME=${home}/.cache;XDG_RUNTIME_DIR=${home}/run;PRIMEEDIT_BIN=$<TARGET_FILE:prime-edit>"
        # Only a passing stub exits 77 (support.h). Not SKIP_REGULAR_EXPRESSION:
        # a matching line marks the test skipped even when it exits nonzero.
        SKIP_RETURN_CODE 77
        TIMEOUT 300)
endforeach()

# P1.12 case 5: a failed fdatasync is not reachable on ext4 from CTest, so the
# receipt runs this driver under tools/fdatasync_eio_shim.c. Not a test and not
# built by default.
add_executable(sync_failure_driver EXCLUDE_FROM_ALL tools/sync_failure_driver.cpp)
target_include_directories(sync_failure_driver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../validation)
target_link_libraries(sync_failure_driver PRIVATE prime-edit-core)
