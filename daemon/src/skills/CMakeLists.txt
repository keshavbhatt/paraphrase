# skills: the skill registry, YAML loader, template rendering and request inputs (PLAN D4).
# QtCore, yaml-cpp and util only (DOCS/CODING_STANDARDS.md 2).
add_library(paraphrase_skills STATIC
    request_inputs.cpp
    skill.cpp
    skill_registry.cpp
    user_actions.cpp
    template_renderer.cpp
)
paraphrase_set_warnings(paraphrase_skills)
target_include_directories(paraphrase_skills PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(paraphrase_skills
    PUBLIC paraphrase_util Qt6::Core
    PRIVATE yaml-cpp::yaml-cpp
)
