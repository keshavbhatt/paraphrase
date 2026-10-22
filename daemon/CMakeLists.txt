# paraphrase-daemon: owns the model and does all inference (PLAN D1). QtCore and, from the
# API step, QtDBus; no GUI. Each source directory is its own static library so the
# dependency direction is enforced by the linker:
#
#   app --> api --> engine --> skills --> util
#                   models --> util

add_subdirectory(src)

if(PARAPHRASE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
