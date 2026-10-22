# paraphrase-app: the desktop client (PLAN D1). M2 starts with the standalone main window
# (ADR-013); the popup, tray, hotkey, capture and inject backends follow. A D-Bus client:
# it never links the engine.
find_package(Qt6 ${PARAPHRASE_QT_MIN_VERSION} REQUIRED COMPONENTS Gui Widgets Svg)

# Capture, insert and shortcut backends (PLAN 6): interfaces, ranking and the fallbacks that
# work everywhere; session-specific backends join here.
add_library(paraphrase_integration STATIC
    src/integration/backends.h
    src/integration/clipboard_backends.cpp
    src/integration/integrations.cpp
)
paraphrase_set_warnings(paraphrase_integration)
target_include_directories(paraphrase_integration PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(paraphrase_integration PUBLIC Qt6::Gui)

add_library(paraphrase_ui STATIC
    src/ui/about_page.cpp
    src/ui/action_bar.cpp
    src/ui/app_flow.cpp
    src/ui/bars.cpp
    src/ui/change_list.cpp
    src/ui/chip_button.cpp
    src/ui/download_panel.cpp
    src/ui/hardware_chips.cpp
    src/ui/input_pages.cpp
    src/ui/input_pane.cpp
    src/ui/installed_models_table.cpp
    src/ui/languages.cpp
    src/ui/languages_section.cpp
    src/ui/main_window.cpp
    src/ui/model_card.cpp
    src/ui/model_info.cpp
    src/ui/model_page.cpp
    src/ui/models_controller.cpp
    src/ui/models_settings_page.cpp
    src/ui/output_pane.cpp
    src/ui/request_runner.cpp
    src/ui/result_views.cpp
    src/ui/sample_run.cpp
    src/ui/settings_nav.cpp
    src/ui/settings_window.cpp
    src/ui/setup_window.cpp
    src/ui/skill_catalog.cpp
    src/ui/step_header.cpp
    src/ui/text_page.cpp
    src/ui/theme.cpp
    src/ui/tier_pickers.cpp
    src/ui/try_page.cpp
    src/ui/widgets.cpp
    src/ui/window_style.cpp
)
paraphrase_set_warnings(paraphrase_ui)
target_include_directories(paraphrase_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
# The language shown next to the word count comes from the same detector the daemon uses.
target_link_libraries(paraphrase_ui PUBLIC paraphrase_client paraphrase_util Qt6::Widgets Qt6::Svg)
qt_add_resources(paraphrase_ui "icons"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        icons/alert.svg icons/check.svg icons/copy.svg icons/down.svg icons/forward.svg
        icons/info.svg icons/lock.svg icons/pen.svg icons/refresh.svg icons/stop.svg
        icons/undo.svg icons/x.svg
)

qt_add_executable(paraphrase-app src/main.cpp)
paraphrase_set_warnings(paraphrase-app)
target_include_directories(paraphrase-app PRIVATE "${CMAKE_BINARY_DIR}/daemon/src/generated")
target_link_libraries(paraphrase-app PRIVATE paraphrase_ui)

if(PARAPHRASE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
