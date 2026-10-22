# The window against the real daemon service with a fake engine (daemon/tests/fake_daemon.h).
paraphrase_add_test(tst_main_window tst_main_window.cpp paraphrase_ui paraphrase_api)
target_include_directories(tst_main_window PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests")
target_sources(tst_main_window PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests/fake_daemon.h")

# Backend ranking and the fallbacks that always work.
paraphrase_add_test(tst_integrations tst_integrations.cpp paraphrase_integration)

# The first-run and settings windows, with downloads from a local test server.
foreach(test tst_setup_window tst_settings_window)
    paraphrase_add_test(${test} ${test}.cpp paraphrase_ui paraphrase_api)
    target_include_directories(${test} PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests")
    target_sources(${test} PRIVATE
        "${PROJECT_SOURCE_DIR}/daemon/tests/fake_daemon.h"
        "${PROJECT_SOURCE_DIR}/daemon/tests/test_http_server.h" # Q_OBJECT: automoc must see it
        app_daemon.h)
endforeach()
