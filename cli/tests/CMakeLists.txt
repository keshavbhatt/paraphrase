# The CLI end to end: the real binary, the real daemon in private mode, the real model.
paraphrase_add_test(tst_cli tst_cli.cpp)
target_compile_definitions(tst_cli PRIVATE
    PARAPHRASE_CLI="$<TARGET_FILE:paraphrase>"
    PARAPHRASE_DAEMON_BIN="$<TARGET_FILE:paraphrase-daemon>")
add_dependencies(tst_cli paraphrase paraphrase-daemon)
