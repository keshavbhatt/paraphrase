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
    src/ui/action_bar.cpp
    src/ui/bars.cpp
    src/ui/change_list.cpp
    src/ui/chip_button.cpp
    src/ui/input_pages.cpp
    src/ui/input_pane.cpp
    src/ui/languages.cpp
    src/ui/main_window.cpp
    src/ui/output_pane.cpp
    src/ui/request_runner.cpp
    src/ui/result_views.cpp
    src/ui/skill_catalog.cpp
    src/ui/theme.cpp
    src/ui/widgets.cpp
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
