# paraphrase: the command-line client (PLAN 4, DOCS/api.md). A D-Bus client only: it never
# links the engine.
qt_add_executable(paraphrase
    src/cli.cpp
    src/daemon_client.cpp
    src/main.cpp
)
paraphrase_set_warnings(paraphrase)
target_include_directories(paraphrase PRIVATE "${CMAKE_BINARY_DIR}/daemon/src/generated")
# Development builds start the daemon from the build tree; packages install both side by side.
target_compile_definitions(paraphrase PRIVATE
    PARAPHRASE_BUILD_DAEMON="$<TARGET_FILE:paraphrase-daemon>")
target_link_libraries(paraphrase PRIVATE Qt6::Core Qt6::DBus)
add_dependencies(paraphrase paraphrase-daemon)

if(PARAPHRASE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
