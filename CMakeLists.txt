cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "Setting C++ standard to c++${CMAKE_CXX_STANDARD}")

set(CMAKE_MODULE_PATH
        "${CMAKE_MODULE_PATH}"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}"
        "${CMAKE_BINARY_DIR}/generators"
        "${CMAKE_BINARY_DIR}"
)
message(WARNING "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

project(aiecad
        VERSION 0.1.0
        DESCRIPTION "Spatial Wiring for AIE Grid"
        LANGUAGES CXX C
)

# TODO: installation rules to be determined
set(INCLUDE_INSTALL_DIR include CACHE STRING
        "The subdirectory where header files should be installed"
)
set(LIB_INSTALL_DIR lib CACHE STRING
        "The subdirectory where library files should be installed"
)
set(BIN_INSTALL_DIR bin CACHE STRING
        "The subdirectory where binary files should be installed"
)
set(CMAKE_INSTALL_DIR lib/cmake/aiecad CACHE STRING
        "The subdirectory where CMake configuration files should be installed"
)

include(settings/ProjectOptions)
aiecad_set_options()

set(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(AIECAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/aiecad")
string(REGEX REPLACE "(.)" [[\\\1]] AIECAD_DIR_REGEX_ESCAPED "${AIECAD_DIR}")

include(checks/AIECADPlatformChecks)
include(utils/AIECADAutoSource)

aiecad_auto_source(
        OUT_VAR             src_files
        DIRECTORIES         "${AIECAD_DIR}"
        EXCLUDE_DIRECTORIES
            "${AIECAD_DIR}/test"
            "${AIECAD_DIR}/memory"
            "${AIECAD_DIR}/aiecad-compiler"
        EXTENSIONS          cpp
        TRAVERSE            RECURSE
        FAIL_MODE           FATAL_ERROR
)

aiecad_auto_source(
        OUT_VAR             header_files
        DIRECTORIES         "${AIECAD_DIR}"
        EXCLUDE_DIRECTORIES
            "${AIECAD_DIR}/test"
            "${AIECAD_DIR}/memory"
            "${AIECAD_DIR}/aiecad-compiler"
        EXTENSIONS          h hpp
        TRAVERSE            RECURSE
        FAIL_MODE           FATAL_ERROR
)

include(ProjectDeps)
# -- locate ImGui root (where imgui.h lives) --
find_path(IMGUI_INCLUDE_DIR
    NAMES imgui.h
    HINTS
      "${AIECAD_IMGUI_BINDINGS_DIR}"
      "${PROJECT_SOURCE_DIR}/thirdparty/imgui"
      "${PROJECT_SOURCE_DIR}/external/imgui"
      "${PROJECT_SOURCE_DIR}/deps/imgui"
)

if(IMGUI_INCLUDE_DIR)
    message(STATUS "ImGui include dir found: ${IMGUI_INCLUDE_DIR}")
else()
    message(WARNING "Could not find imgui.h automatically. Set AIECAD_IMGUI_BINDINGS_DIR or IMGUI_INCLUDE_DIR to the ImGui root.")
endif()

include(checks/AIECADConfigs)

add_library(aiecad_deps INTERFACE)

if (AIECAD_INCLUDE_DIRS)
    target_include_directories(aiecad_deps INTERFACE ${AIECAD_INCLUDE_DIRS})
endif()
if (AIECAD_LINK_LIBS)
    target_link_libraries(aiecad_deps INTERFACE ${AIECAD_LINK_LIBS})
endif()
if (AIECAD_COMPILE_OPTS)
    target_compile_options(aiecad_deps INTERFACE ${AIECAD_COMPILE_OPTS})
endif()
if (AIECAD_COMPILE_DEFS)
    message(STATUS "adding compile defs...")
    target_compile_definitions(aiecad_deps INTERFACE ${AIECAD_COMPILE_DEFS} )
else ()
    message(STATUS "no compile defs...")
endif()

# Add SPDLOG_FMT_EXTERNAL to all targets that use spdlog
target_compile_definitions(aiecad_deps INTERFACE SPDLOG_FMT_EXTERNAL)

if (XRT_LIBDIR)
    set_target_properties(aiecad_deps PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            BUILD_RPATH   "${XRT_LIBDIR}"
            INSTALL_RPATH "$ORIGIN;${XRT_LIBDIR}"
    )
endif()

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/aiecad)
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config/aiecad-config.h.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/aiecad/aiecad-config.h"
)


# ---- optional manual override (keeps your auto-detect as default) ----
set(AIECAD_IMGUI_BINDINGS_DIR "" CACHE PATH "Path to Dear ImGui backends/ folder (leave empty to auto-detect)")

# ---- auto-detect backends dir by globbing the build tree (your approach) ----
if (NOT AIECAD_IMGUI_BINDINGS_DIR)
    file(GLOB_RECURSE IMGUI_BINDING_CANDIDATES
            "${CMAKE_BINARY_DIR}/**/imgui_impl_glfw.cpp")

    if (IMGUI_BINDING_CANDIDATES)
        list(REMOVE_DUPLICATES IMGUI_BINDING_CANDIDATES)

        # Prefer a directory that also contains imgui_impl_opengl3.cpp
        set(_FOUND_DIR "")
        foreach(_cand ${IMGUI_BINDING_CANDIDATES})
            get_filename_component(_dir "${_cand}" DIRECTORY)
            if (EXISTS "${_dir}/imgui_impl_opengl3.cpp")
                set(_FOUND_DIR "${_dir}")
                break()
            endif()
        endforeach()

        if (NOT _FOUND_DIR)
            # Fallback: take the directory of the first glfw impl we found
            get_filename_component(_FOUND_DIR "${IMGUI_BINDING_CANDIDATES}" DIRECTORY)
        endif()

        set(AIECAD_IMGUI_BINDINGS_DIR "${_FOUND_DIR}" CACHE PATH "" FORCE)
        message(STATUS "Found ImGui backends: ${AIECAD_IMGUI_BINDINGS_DIR}")
    else()
        message(FATAL_ERROR "Could not find imgui_impl_glfw.cpp in ${CMAKE_BINARY_DIR}")
    endif()
endif()

# ---- core library ----
list(APPEND aiecad_base_files ${src_files} ${header_files})
# ---- spdlog detection and robust linking ----
find_package(spdlog QUIET)

if (TARGET spdlog::spdlog)
    message(STATUS "Linking to spdlog::spdlog target")
    target_link_libraries(aiecad_deps INTERFACE spdlog::spdlog)
elseif (TARGET spdlog::spdlog_header_only)
    # Conan often exposes the header-only variant under this name
    message(STATUS "Linking to spdlog::spdlog_header_only target (Conan provided)")
    target_link_libraries(aiecad_deps INTERFACE spdlog::spdlog_header_only)
elseif (spdlog_FOUND)
    # find_package found spdlog but no imported target — try the conventional target name
    message(WARNING "spdlog found but no modern CMake target detected; attempting to use spdlog::spdlog")
    target_link_libraries(aiecad_deps INTERFACE spdlog::spdlog)
else()
    # Fallback for Conan legacy variables (if using Conan generator that sets these)
    if (DEFINED CONAN_INCLUDE_DIRS_spdlog OR DEFINED CONAN_LIBS_spdlog)
        message(STATUS "Using Conan-provided spdlog variables")
        if (DEFINED CONAN_INCLUDE_DIRS_spdlog)
            target_include_directories(aiecad_deps INTERFACE ${CONAN_INCLUDE_DIRS_spdlog})
        endif()
        if (DEFINED CONAN_LIBS_spdlog)
            target_link_libraries(aiecad_deps INTERFACE ${CONAN_LIBS_spdlog})
        endif()
    else()
        message(WARNING "spdlog not found. Run conan install, add spdlog as a dependency, or vendor spdlog.")
    endif()
endif()

add_library(aiecad_lib STATIC
        ${aiecad_base_files}
        ${CMAKE_CURRENT_BINARY_DIR}/aiecad/aiecad-config.h
        "${AIECAD_IMGUI_BINDINGS_DIR}/imgui_impl_glfw.cpp"
        "${AIECAD_IMGUI_BINDINGS_DIR}/imgui_impl_opengl3.cpp"
)

target_include_directories(aiecad_lib
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
      $<INSTALL_INTERFACE:include>
      "${AIECAD_IMGUI_BINDINGS_DIR}"                        # backends
      $<$<BOOL:${IMGUI_INCLUDE_DIR}>:${IMGUI_INCLUDE_DIR}> # ImGui root (imgui.h)
)


target_link_libraries(aiecad_lib PUBLIC aiecad_deps)

# Also link dependencies directly to ensure include directories propagate correctly
# This is needed because generator expressions in INTERFACE_INCLUDE_DIRECTORIES
# may not propagate correctly through interface libraries in all cases
if(TARGET glfw)
    target_link_libraries(aiecad_lib PUBLIC glfw)
endif()
if(TARGET spdlog::spdlog_header_only)
    target_link_libraries(aiecad_lib PUBLIC spdlog::spdlog_header_only)
endif()

target_compile_definitions(aiecad_lib PRIVATE
        IMGUI_IMPL_OPENGL_LOADER_GLAD
        IMGUI_ENABLE_DOCKING
        SPDLOG_FMT_EXTERNAL
)

# Add /utf-8 flag for fmt Unicode support
target_compile_options(aiecad_lib PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

if (AIECAD_BUILD_TESTS)
    include(AIECAD_Tests)
endif ()

if (AIECAD_BUILD_SANDBOX)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/sandbox")
    aiecad_build_sandbox()
endif ()

add_executable(aiecad_app main.cpp)
target_link_libraries(aiecad_app PRIVATE aiecad_lib)

# Ensure all dependencies are linked (static libraries don't propagate dependencies)
# Link through aiecad_deps which has all the dependencies
target_link_libraries(aiecad_app PRIVATE aiecad_deps)

# Explicitly link fmt and imgui since they might not propagate from static library
if(TARGET fmt::fmt)
    target_link_libraries(aiecad_app PRIVATE fmt::fmt)
endif()
if(TARGET imgui::imgui)
    target_link_libraries(aiecad_app PRIVATE imgui::imgui)
endif()