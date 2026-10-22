# Test helpers shared by daemon/tests and cli/tests: Qt Test, the environment tests run in,
# and paraphrase_add_test(). Included from the root when PARAPHRASE_BUILD_TESTS is on.
find_package(Qt6 ${PARAPHRASE_QT_MIN_VERSION} REQUIRED COMPONENTS Test)

set(PARAPHRASE_TEST_ENV
    "QT_QPA_PLATFORM=offscreen"
    "QT_LOGGING_RULES=paraphrase.*.debug=false"
    "QT_FORCE_STDERR_LOGGING=1"
)
# Dev builds (scripts/dev-build.sh) run the tests against the kf6-core24 runtime snap, the
# libraries the shipped snap uses.
if(PARAPHRASE_SNAP_RUNTIME)
    list(APPEND PARAPHRASE_TEST_ENV
        "LD_LIBRARY_PATH=${PARAPHRASE_SNAP_RUNTIME_LIB_PATH}"
        "QT_PLUGIN_PATH=${PARAPHRASE_SNAP_RUNTIME}/usr/lib/x86_64-linux-gnu/qt6/plugins"
    )
endif()

# Data shared with the Python eval harness (skills, datasets, test vectors) lives at the
# repository root.
set(PARAPHRASE_REPO_ROOT "${PROJECT_SOURCE_DIR}")

function(paraphrase_add_test name source)
    qt_add_executable(${name} ${source})
    paraphrase_set_warnings(${name})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARGN})
    target_compile_definitions(${name} PRIVATE
        PARAPHRASE_REPO_ROOT="${PARAPHRASE_REPO_ROOT}")
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "${PARAPHRASE_TEST_ENV}")
endfunction()
