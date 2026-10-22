# The window against the real daemon service with a fake engine (daemon/tests/fake_daemon.h).
paraphrase_add_test(tst_main_window tst_main_window.cpp paraphrase_ui paraphrase_api)
target_include_directories(tst_main_window PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests")
target_sources(tst_main_window PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests/fake_daemon.h")

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
