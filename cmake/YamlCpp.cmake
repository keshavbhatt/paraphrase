# yaml-cpp (third_party/yaml-cpp, pinned submodule) reads skill files (ADR-014): static, no
# tools, tests, contrib or install rules.
foreach(opt YAML_CPP_BUILD_TOOLS YAML_CPP_BUILD_TESTS YAML_CPP_BUILD_CONTRIB YAML_CPP_INSTALL
        YAML_CPP_FORMAT_SOURCE YAML_BUILD_SHARED_LIBS)
    set(${opt} OFF CACHE BOOL "" FORCE)
endforeach()

# SYSTEM: third-party headers, so our strict warnings do not apply to them.
add_subdirectory(third_party/yaml-cpp SYSTEM)
