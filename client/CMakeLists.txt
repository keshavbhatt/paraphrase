# client: what every client of the daemon shares (the CLI now, the app from M2): finding or
# starting the daemon and talking to it (ADR-010), and the word diff clients show for
# proofread (PLAN D6). QtCore and QtDBus only; never the engine.
add_library(paraphrase_client STATIC
    src/daemon_client.cpp
    src/word_diff.cpp
)
paraphrase_set_warnings(paraphrase_client)
target_include_directories(paraphrase_client PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(paraphrase_client PUBLIC Qt6::Core Qt6::DBus)
# Development builds start the daemon from the build tree; packages install both side by side.
target_compile_definitions(paraphrase_client PRIVATE
    PARAPHRASE_BUILD_DAEMON="$<TARGET_FILE:paraphrase-daemon>")
add_dependencies(paraphrase_client paraphrase-daemon)

if(PARAPHRASE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
