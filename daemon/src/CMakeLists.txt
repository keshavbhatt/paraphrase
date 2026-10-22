add_subdirectory(util)
add_subdirectory(skills)
add_subdirectory(engine)

configure_file(app/version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/paraphrase/version.h" @ONLY)

# The daemon's entry point. For now it reports its version and llama.cpp's view of the CPU,
# which proves the library links and loads; the engine and the API arrive in later M1 steps.
qt_add_executable(paraphrase-daemon app/main.cpp)
paraphrase_set_warnings(paraphrase-daemon)
target_include_directories(paraphrase-daemon PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(paraphrase-daemon PRIVATE Qt6::Core llama)
