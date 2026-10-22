cmake_minimum_required(VERSION 3.25)

project(paraphrase
    VERSION 0.1.0
    DESCRIPTION "Private, offline writing assistant"
    HOMEPAGE_URL "https://github.com/keshavbhatt/paraphrase"
    LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(PARAPHRASE_BUILD_TESTS "Build unit tests" ON)
option(PARAPHRASE_WERROR "Treat warnings as errors" OFF)
# Packages build llama.cpp for every CPU level and pick one at run time; developer builds
# are tuned to the machine that builds them (ADR-008).
option(PARAPHRASE_PORTABLE_CPU "Build llama.cpp for every CPU level (packages)" OFF)
option(PARAPHRASE_VULKAN "Build the llama.cpp Vulkan backend" OFF)

set(PARAPHRASE_SNAP_RUNTIME "" CACHE PATH "kf6-core24 runtime snap root for running tests (dev builds)")
set(PARAPHRASE_SNAP_RUNTIME_LIB_PATH "" CACHE STRING "LD_LIBRARY_PATH for the snap runtime (dev builds)")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Third-party code first, before our language standard and Qt strictness flags apply:
# llama.cpp keeps its own C++ standard and warnings.
include(LlamaCpp)
include(YamlCpp)

# ---------------------------------------------------------------------------
# Our code
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(Warnings)
include(Version)

# Qt 6.11: the version of the KDE snap and Flatpak runtimes (as Omnidict, its ADR-009).
# Components are added in the step that first uses them.
set(PARAPHRASE_QT_MIN_VERSION "6.11" CACHE STRING "Minimum required Qt6 version")
find_package(Qt6 ${PARAPHRASE_QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus Network)
qt_standard_project_setup()

add_compile_definitions(
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_NO_KEYWORDS
    QT_DISABLE_DEPRECATED_UP_TO=0x060b00
)

if(PARAPHRASE_BUILD_TESTS)
    enable_testing()
    include(Testing)
endif()

add_subdirectory(daemon)
