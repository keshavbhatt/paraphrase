# Backend ranking and the fallbacks that always work.
paraphrase_add_test(tst_integrations tst_integrations.cpp paraphrase_integration)

# The windows and the popup against the real daemon service with a fake engine, on a private
# socket, with model downloads from a local test server (app_daemon.h).
paraphrase_add_test(tst_main_window tst_main_window.cpp paraphrase_ui paraphrase_api)
paraphrase_add_test(tst_setup_window tst_setup_window.cpp paraphrase_ui paraphrase_api)
paraphrase_add_test(tst_settings_window tst_settings_window.cpp paraphrase_ui paraphrase_api)
paraphrase_add_test(tst_popup tst_popup.cpp paraphrase_popup paraphrase_api)
foreach(test tst_main_window tst_setup_window tst_settings_window tst_popup)
    target_include_directories(${test} PRIVATE "${PROJECT_SOURCE_DIR}/daemon/tests")
    target_sources(${test} PRIVATE
        "${PROJECT_SOURCE_DIR}/daemon/tests/fake_daemon.h"
        "${PROJECT_SOURCE_DIR}/daemon/tests/test_http_server.h" # Q_OBJECT: automoc must see it
        app_daemon.h)
endforeach()
