# llama.cpp (third_party/llama.cpp, pinned submodule) built as a library for the daemon
# (PLAN D2). Only the libraries: no tools, server, examples or tests, and no network code.
#
# Developer builds: static, tuned to this machine (GGML_NATIVE).
# Packages (PARAPHRASE_PORTABLE_CPU): shared, one CPU library per instruction-set level,
# the best one loaded at run time (ADR-008).

set(LLAMA_BUILD_COMMON ON CACHE BOOL "" FORCE)
foreach(opt
        LLAMA_BUILD_TESTS LLAMA_BUILD_TOOLS LLAMA_BUILD_EXAMPLES LLAMA_BUILD_SERVER
        LLAMA_BUILD_APP LLAMA_BUILD_UI LLAMA_BUILD_HTML LLAMA_OPENSSL LLAMA_SUBPROCESS
        LLAMA_USE_PREBUILT_UI)
    set(${opt} OFF CACHE BOOL "" FORCE)
endforeach()

if(PARAPHRASE_PORTABLE_CPU)
    set(GGML_NATIVE OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    set(GGML_BACKEND_DL ON CACHE BOOL "" FORCE)
    set(GGML_CPU_ALL_VARIANTS ON CACHE BOOL "" FORCE)
else()
    set(GGML_NATIVE ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()
set(GGML_VULKAN ${PARAPHRASE_VULKAN} CACHE BOOL "" FORCE)

# SYSTEM: its headers are third-party, so our strict warnings do not apply to them.
add_subdirectory(third_party/llama.cpp SYSTEM)
message(STATUS "llama.cpp: portable=${PARAPHRASE_PORTABLE_CPU} vulkan=${PARAPHRASE_VULKAN}")
