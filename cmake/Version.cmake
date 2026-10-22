# Derives version metadata for the build.
#
# PARAPHRASE_VERSION       -> "0.1.0" (from project(VERSION))
# PARAPHRASE_GIT_REVISION  -> short hash or "unknown"
#
# These reach the code through a generated header, never through string literals.

set(PARAPHRASE_VERSION "${PROJECT_VERSION}")

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}" rev-parse --short HEAD
        OUTPUT_VARIABLE PARAPHRASE_GIT_REVISION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    # Configure again when HEAD moves (commit, checkout), so the hash is never stale.
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${PROJECT_SOURCE_DIR}" rev-parse --absolute-git-dir
        OUTPUT_VARIABLE _paraphrase_git_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_paraphrase_git_dir AND EXISTS "${_paraphrase_git_dir}/logs/HEAD")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_paraphrase_git_dir}/logs/HEAD")
    endif()
endif()
if(NOT PARAPHRASE_GIT_REVISION)
    set(PARAPHRASE_GIT_REVISION "unknown")
endif()

message(STATUS "paraphrase ${PARAPHRASE_VERSION} (${PARAPHRASE_GIT_REVISION})")
