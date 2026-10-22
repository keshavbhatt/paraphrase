# The window against the real daemon service with a fake engine (daemon/tests/fake_daemon.h).
paraphrase_add_test(tst_main_window tst_main_window.cpp paraphrase_ui paraphrase_api)
target_include_directories(tst_main_window PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests")
target_sources(tst_main_window PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests/fake_daemon.h")

# Backend ranking and the fallbacks that always work.
paraphrase_add_test(tst_integrations tst_integrations.cpp paraphrase_integration)

# The popup flow against the real daemon service with a fake engine and fake backends.
paraphrase_add_test(tst_popup tst_popup.cpp paraphrase_popup paraphrase_api)
target_include_directories(tst_popup PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests")
target_sources(tst_popup PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests/fake_daemon.h")
