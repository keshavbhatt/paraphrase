# api: the D-Bus service, the request queue and the inference thread (PLAN 5.1, ADR-010).
# May use Qt6::DBus (DOCS/CODING_STANDARDS.md 2).
add_library(paraphrase_api STATIC
    daemon_service.cpp
    inference_worker.cpp
)
paraphrase_set_warnings(paraphrase_api)
target_include_directories(paraphrase_api PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(paraphrase_api PUBLIC paraphrase_engine paraphrase_models paraphrase_skills Qt6::Core Qt6::DBus)
