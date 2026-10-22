# util: pure text logic with QtCore only (DOCS/CODING_STANDARDS.md 2). The language data is
# shared with the Python eval harness and embedded as :/paraphrase/language.json.
add_library(paraphrase_util STATIC
    language_data.cpp
    language_detector.cpp
)
paraphrase_set_warnings(paraphrase_util)
target_include_directories(paraphrase_util PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(paraphrase_util PUBLIC Qt6::Core)
qt_add_resources(paraphrase_util "language_data"
    PREFIX "/paraphrase"
    BASE "${PROJECT_SOURCE_DIR}/daemon/data"
    FILES "${PROJECT_SOURCE_DIR}/daemon/data/language.json"
)
