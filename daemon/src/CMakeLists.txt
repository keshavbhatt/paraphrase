add_subdirectory(util)
add_subdirectory(skills)
add_subdirectory(engine)
add_subdirectory(models)
add_subdirectory(api)

configure_file(app/version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/paraphrase/version.h" @ONLY)

# The daemon: the D-Bus API over the engine (PLAN D1, 5.1).
qt_add_executable(paraphrase-daemon app/main.cpp)
paraphrase_set_warnings(paraphrase-daemon)
target_include_directories(paraphrase-daemon PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
# Development builds find the skills in the source tree; packages install them.
target_compile_definitions(paraphrase-daemon PRIVATE
    PARAPHRASE_SOURCE_SKILLS_DIR="${PROJECT_SOURCE_DIR}/skills")
target_link_libraries(paraphrase-daemon PRIVATE paraphrase_api Qt6::Core Qt6::DBus llama)

include(GNUInstallDirs)
install(TARGETS paraphrase-daemon RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
# main.cpp looks for the skills in <bindir>/../share/paraphrase/skills.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/skills/ DESTINATION ${CMAKE_INSTALL_DATADIR}/paraphrase/skills)
