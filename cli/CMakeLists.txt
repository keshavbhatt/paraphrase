# paraphrase: the command-line client (PLAN 4, DOCS/api.md). A D-Bus client only: it never
# links the engine.
qt_add_executable(paraphrase
    src/cli.cpp
    src/main.cpp
)
paraphrase_set_warnings(paraphrase)
target_include_directories(paraphrase PRIVATE "${CMAKE_BINARY_DIR}/daemon/src/generated")
target_link_libraries(paraphrase PRIVATE paraphrase_client Qt6::Core Qt6::DBus)

if(PARAPHRASE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
