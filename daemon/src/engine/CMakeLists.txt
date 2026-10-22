# engine: the llama.cpp engine and the skill runner. The only library that includes llama.cpp
# headers (DOCS/CODING_STANDARDS.md 2 and 10).
add_library(paraphrase_engine STATIC
    llama_engine.cpp
    skill_runner.cpp
)
paraphrase_set_warnings(paraphrase_engine)
target_include_directories(paraphrase_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(paraphrase_engine
    PUBLIC paraphrase_skills paraphrase_util Qt6::Core
    PRIVATE llama llama-common
)
