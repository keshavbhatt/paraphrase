# Backend ranking and the fallbacks that always work.
paraphrase_add_test(tst_integrations tst_integrations.cpp paraphrase_integration)
# Putting a result back, with fakes: the window check and the clipboard restore.
paraphrase_add_test(tst_paste_insert tst_paste_insert.cpp paraphrase_integration)
# The desktop backends against a headless KWin (scripts/desktop-test.sh); skipped elsewhere.
paraphrase_add_test(tst_x11_backends tst_x11_backends.cpp paraphrase_integration Qt6::Widgets)
if(TARGET paraphrase_wayland_protocols)
    paraphrase_add_test(tst_wayland_backends tst_wayland_backends.cpp paraphrase_integration)
    # Read-only checks on the user's own session, by hand (PARAPHRASE_TEST_LIVE=1).
    paraphrase_add_test(tst_live_session tst_live_session.cpp paraphrase_integration)
endif()

# The windows and the popup against the real daemon service with a fake engine, on a private
# socket, with model downloads from a local test server (app_daemon.h).
paraphrase_add_test(tst_main_window tst_main_window.cpp paraphrase_ui paraphrase_api)
paraphrase_add_test(tst_setup_window tst_setup_window.cpp paraphrase_shell paraphrase_ui paraphrase_api)
paraphrase_add_test(tst_settings_window tst_settings_window.cpp paraphrase_ui paraphrase_api)
paraphrase_add_test(tst_popup tst_popup.cpp paraphrase_popup paraphrase_api)
foreach(test tst_main_window tst_setup_window tst_settings_window tst_popup)
    target_include_directories(${test} PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests")
    target_sources(${test} PRIVATE
        "${PROJECT_SOURCE_DIR}/daemon/tests/fake_daemon.h"
        "${PROJECT_SOURCE_DIR}/daemon/tests/test_http_server.h" # Q_OBJECT: automoc must see it
        app_daemon.h)
endforeach()

# The facts a rewrite must keep, checked like the eval's protected tokens.
paraphrase_add_test(tst_facts_check tst_facts_check.cpp paraphrase_ui)

# Every action with the real models, by hand only (PARAPHRASE_TEST_REAL=1); it skips otherwise.
paraphrase_add_test(tst_real_models tst_real_models.cpp paraphrase_popup)

# The README's screenshots with the real models, by hand only (scripts/screenshots.sh); it
# skips otherwise.
paraphrase_add_test(tst_screenshots tst_screenshots.cpp paraphrase_popup paraphrase_ui paraphrase_api)

# The opt-in history: sealing, the wrong-key guard and delete all, with a fake keyring.
paraphrase_add_test(tst_history tst_history.cpp paraphrase_history)
