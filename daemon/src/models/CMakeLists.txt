# models: the built-in catalog, the models folder, downloads, the hardware probe and the tier
# choice. QtCore, QtNetwork and util only (DOCS/CODING_STANDARDS.md 2).
add_library(paraphrase_models STATIC
    hardware_probe.cpp
    model_catalog.cpp
    model_downloader.cpp
    model_store.cpp
    tier_selector.cpp
)
paraphrase_set_warnings(paraphrase_models)
target_include_directories(paraphrase_models PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(paraphrase_models PUBLIC paraphrase_util Qt6::Core Qt6::Network)
# The catalog is built in (ADR-015); the eval harness reads the same file.
qt_add_resources(paraphrase_models "model_catalog"
    PREFIX "/paraphrase"
    BASE "${PROJECT_SOURCE_DIR}"
    FILES "${PROJECT_SOURCE_DIR}/models.json"
)
