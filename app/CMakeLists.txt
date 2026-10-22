# paraphrase-app: the desktop client (PLAN D1). M2 starts with the standalone main window
# (ADR-013); the popup, tray, hotkey, capture and inject backends follow. A D-Bus client:
# it never links the engine.
find_package(Qt6 ${PARAPHRASE_QT_MIN_VERSION} REQUIRED COMPONENTS DBus Gui Widgets Svg)

# Capture, insert and shortcut backends (PLAN 6, DOCS/platform-matrix.md): interfaces, the
# X11 and Wayland backends, ranking, and the fallbacks that work everywhere.
add_library(paraphrase_integration STATIC
    src/integration/backends.h
    src/integration/clipboard_backends.cpp
    src/integration/integrations.cpp
    src/integration/kwin_window_tracker.cpp
    src/integration/paste_insert.cpp
    src/integration/portal.cpp
    src/integration/portal_paste_keys.cpp
    src/integration/portal_shortcut.cpp
    src/integration/qt_clipboard_access.cpp
    src/integration/selection_captures.cpp
    src/integration/session_backends.cpp
    src/integration/x11.cpp
    src/integration/x11_backends.cpp
)
paraphrase_set_warnings(paraphrase_integration)
target_include_directories(paraphrase_integration PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# X11: Qt's own xcb connection, plus XTEST for the paste (libxcb, libxcb-xtest0 at runtime;
# pacman: libxcb).
find_library(XCB_LIBRARY xcb REQUIRED)
find_library(XCB_XTEST_LIBRARY xcb-xtest REQUIRED)
target_link_libraries(paraphrase_integration PUBLIC Qt6::Gui Qt6::DBus ${XCB_XTEST_LIBRARY} ${XCB_LIBRARY})

# Wayland data-control (ext-data-control-v1): reading the selection without focus. The
# protocol code is generated from wayland-protocols; without them the clipboard fallbacks
# remain (pacman: wayland, wayland-protocols).
find_library(WAYLAND_CLIENT_LIBRARY wayland-client)
find_path(WAYLAND_CLIENT_INCLUDE_DIR wayland-client.h)
find_program(WAYLAND_SCANNER wayland-scanner)
find_file(EXT_DATA_CONTROL_XML ext-data-control-v1.xml
    PATH_SUFFIXES share/wayland-protocols/staging/ext-data-control)
if(WAYLAND_CLIENT_LIBRARY AND WAYLAND_CLIENT_INCLUDE_DIR AND WAYLAND_SCANNER AND EXT_DATA_CONTROL_XML)
    set(_dc_dir "${CMAKE_CURRENT_BINARY_DIR}/wayland")
    file(MAKE_DIRECTORY "${_dc_dir}")
    add_custom_command(
        OUTPUT "${_dc_dir}/ext-data-control-v1-client-protocol.h" "${_dc_dir}/ext-data-control-v1-protocol.c"
        COMMAND "${WAYLAND_SCANNER}" client-header "${EXT_DATA_CONTROL_XML}"
                "${_dc_dir}/ext-data-control-v1-client-protocol.h"
        COMMAND "${WAYLAND_SCANNER}" private-code "${EXT_DATA_CONTROL_XML}"
                "${_dc_dir}/ext-data-control-v1-protocol.c"
        DEPENDS "${EXT_DATA_CONTROL_XML}"
        COMMENT "Generating the ext-data-control-v1 Wayland protocol code")
    # Generated C: built without the project's warnings.
    add_library(paraphrase_wayland_protocols STATIC "${_dc_dir}/ext-data-control-v1-protocol.c")
    target_include_directories(paraphrase_wayland_protocols SYSTEM PUBLIC "${_dc_dir}" "${WAYLAND_CLIENT_INCLUDE_DIR}")
    target_link_libraries(paraphrase_wayland_protocols PUBLIC ${WAYLAND_CLIENT_LIBRARY})
    target_sources(paraphrase_integration PRIVATE src/integration/data_control_client.cpp
                   "${_dc_dir}/ext-data-control-v1-client-protocol.h")
    target_link_libraries(paraphrase_integration PUBLIC paraphrase_wayland_protocols)
    target_compile_definitions(paraphrase_integration PUBLIC PARAPHRASE_HAVE_DATA_CONTROL)
else()
    message(STATUS "Wayland data-control: off (wayland-client, wayland-scanner or wayland-protocols missing)")
endif()

add_library(paraphrase_ui STATIC
    src/ui/settings/about_page.cpp
    src/ui/settings/action_editor.cpp
    src/ui/settings/action_list.cpp
    src/ui/settings/actions_page.cpp
    src/ui/parts/action_bar.cpp
    src/ui/main/bars.cpp
    src/ui/parts/change_list.cpp
    src/ui/common/chip_button.cpp
    src/ui/common/flow_layout.cpp
    src/ui/setup/download_panel.cpp
    src/ui/setup/hardware_chips.cpp
    src/ui/parts/input_pages.cpp
    src/ui/main/input_pane.cpp
    src/ui/settings/installed_models_table.cpp
    src/ui/core/languages.cpp
    src/ui/settings/languages_section.cpp
    src/ui/main/main_window.cpp
    src/ui/setup/model_card.cpp
    src/ui/models/model_info.cpp
    src/ui/setup/model_page.cpp
    src/ui/models/models_controller.cpp
    src/ui/settings/models_settings_page.cpp
    src/ui/main/output_pane.cpp
    src/ui/core/request_runner.cpp
    src/ui/core/result_summary.cpp
    src/ui/parts/result_views.cpp
    src/ui/setup/sample_run.cpp
    src/ui/settings/settings_nav.cpp
    src/ui/settings/settings_window.cpp
    src/ui/setup/setup_window.cpp
    src/ui/setup/shortcut_page.cpp
    src/ui/core/skill_catalog.cpp
    src/ui/core/user_actions_client.cpp
    src/ui/setup/step_header.cpp
    src/ui/settings/text_page.cpp
    src/ui/common/theme.cpp
    src/ui/settings/tier_pickers.cpp
    src/ui/setup/try_page.cpp
    src/ui/common/widgets.cpp
    src/ui/common/window_style.cpp
)
paraphrase_set_warnings(paraphrase_ui)
target_include_directories(paraphrase_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
# The language shown next to the word count comes from the same detector the daemon uses.
target_link_libraries(paraphrase_ui PUBLIC paraphrase_client paraphrase_util Qt6::Widgets Qt6::Svg)
qt_add_resources(paraphrase_ui "icons"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        icons/alert.svg icons/check.svg icons/copy.svg icons/down.svg icons/expand.svg icons/forward.svg icons/left.svg icons/right.svg
        icons/info.svg icons/lock.svg icons/pen.svg icons/refresh.svg icons/stop.svg
        icons/undo.svg icons/x.svg icons/down-light.svg icons/down-dark.svg
)

# The popup: a floating panel over any app, driven by the integration backends.
add_library(paraphrase_popup STATIC
    src/popup/popup_controller.cpp
    src/popup/popup_pages.cpp
    src/popup/popup_window.cpp
)
paraphrase_set_warnings(paraphrase_popup)
target_link_libraries(paraphrase_popup PUBLIC paraphrase_ui paraphrase_integration)

# The running app: window, popup, tray, backends and the D-Bus service, wired together.
find_package(Qt6 ${PARAPHRASE_QT_MIN_VERSION} REQUIRED COMPONENTS DBus)
add_library(paraphrase_shell STATIC
    src/shell/app_flow.cpp
    src/shell/app_service.cpp
    src/shell/application.cpp
    src/shell/tray_icon.cpp
)
paraphrase_set_warnings(paraphrase_shell)
target_link_libraries(paraphrase_shell PUBLIC paraphrase_popup Qt6::DBus)

qt_add_executable(paraphrase-app src/main.cpp)
paraphrase_set_warnings(paraphrase-app)
target_include_directories(paraphrase-app PRIVATE "${CMAKE_BINARY_DIR}/daemon/src/generated")
target_link_libraries(paraphrase-app PRIVATE paraphrase_shell)

if(PARAPHRASE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Desktop integration: the menu entry with its "Work on the selected text" action, the icon
# and the store metadata (PLAN 13).
include(GNUInstallDirs)
install(TARGETS paraphrase-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/com.ktechpit.paraphrase.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/com.ktechpit.paraphrase.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES data/com.ktechpit.paraphrase.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
