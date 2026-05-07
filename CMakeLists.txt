cmake_minimum_required(VERSION 3.15)
project(SBgl VERSION 0.1.0 LANGUAGES C)

# --- LSP Support ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Default Build Type ---
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# --- Options ---
option(SBGL_USE_WAYLAND "Use native Wayland instead of X11" ON)
option(SBGL_BUILD_EXAMPLES "Build example applications" OFF)
option(SBGL_BUILD_TESTS "Build test applications" OFF)
option(SBGL_BUILD_STANDALONE "Build examples by compiling sources directly (no libsbgl.a)" OFF)

# --- Compiler Settings ---
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /wd4201)
    # Strict mode for Debug
    add_compile_options($<$<CONFIG:Debug>:/WX>)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Strict mode for Debug
    add_compile_options($<$<CONFIG:Debug>:-Werror>)
    add_compile_options($<$<CONFIG:Debug>:-Wmissing-prototypes>)
    add_compile_options($<$<CONFIG:Debug>:-Wstrict-prototypes>)
endif()

# --- Find Dependencies ---
# Vulkan is no longer linked against statically to allow for dynamic loading.

# --- Source Files ---
set(SBGL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sbgl_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sbgl_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sbgl_sort.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sbgl_batcher.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend/sbgl_backend_vulkan.c
)

set(PLATFORM_LIBS "")

if(WIN32)
    list(APPEND SBGL_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win32/window.c 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win32/input.c
    )
    if(MINGW)
        list(APPEND PLATFORM_LIBS user32 gdi32 shell32)
        # Use local Vulkan headers if they exist (for cross-compilation)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
            include_directories("${CMAKE_CURRENT_SOURCE_DIR}/third_party")
        endif()
    endif()
elseif(UNIX AND NOT APPLE)
    if(SBGL_USE_WAYLAND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
        pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
        
        execute_process(
            COMMAND pkg-config --variable=pkgdatadir wayland-protocols
            OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
        set(POINTER_CONSTRAINTS_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml")
        set(RELATIVE_POINTER_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/relative-pointer/relative-pointer-unstable-v1.xml")
        
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h"
            COMMAND wayland-scanner client-header "${XDG_SHELL_XML}" "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h"
            DEPENDS "${XDG_SHELL_XML}"
        )
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
            COMMAND wayland-scanner private-code "${XDG_SHELL_XML}" "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
            DEPENDS "${XDG_SHELL_XML}"
        )

        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-client-protocol.h"
            COMMAND wayland-scanner client-header "${POINTER_CONSTRAINTS_XML}" "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-client-protocol.h"
            DEPENDS "${POINTER_CONSTRAINTS_XML}"
        )
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-protocol.c"
            COMMAND wayland-scanner private-code "${POINTER_CONSTRAINTS_XML}" "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-protocol.c"
            DEPENDS "${POINTER_CONSTRAINTS_XML}"
        )

        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-client-protocol.h"
            COMMAND wayland-scanner client-header "${RELATIVE_POINTER_XML}" "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-client-protocol.h"
            DEPENDS "${RELATIVE_POINTER_XML}"
        )
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-protocol.c"
            COMMAND wayland-scanner private-code "${RELATIVE_POINTER_XML}" "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-protocol.c"
            DEPENDS "${RELATIVE_POINTER_XML}"
        )

        add_custom_target(wayland_protocols 
            DEPENDS 
                "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h"
                "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
                "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-client-protocol.h"
                "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-protocol.c"
                "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-client-protocol.h"
                "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-protocol.c"
        )
        
        list(APPEND SBGL_SOURCES 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/window_wayland.c 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/input_wayland.c
            "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
            "${CMAKE_CURRENT_BINARY_DIR}/pointer-constraints-unstable-v1-protocol.c"
            "${CMAKE_CURRENT_BINARY_DIR}/relative-pointer-unstable-v1-protocol.c"
        )
        include_directories("${CMAKE_CURRENT_BINARY_DIR}")
        list(APPEND PLATFORM_LIBS ${WAYLAND_CLIENT_LIBRARIES})
        add_definitions(-DSBGL_PLATFORM_WAYLAND)
    else()
        find_package(X11 REQUIRED)
        list(APPEND SBGL_SOURCES 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/window_x11.c 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/input_x11.c
        )
        list(APPEND PLATFORM_LIBS X11 Xfixes m)
        add_definitions(-DSBGL_PLATFORM_X11)
    endif()
endif()

# --- Library Target ---
if(NOT SBGL_BUILD_STANDALONE)
    add_library(sbgl STATIC ${SBGL_SOURCES})

    if(SBGL_USE_WAYLAND AND NOT WIN32)
        add_dependencies(sbgl wayland_protocols)
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/window_wayland.c PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/input_wayland.c PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
    endif()

    target_include_directories(sbgl
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Allow subprojects to find the headers
    target_include_directories(sbgl SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(sbgl
        PRIVATE
            ${PLATFORM_LIBS}
    )
endif()

# --- Shader Compilation Functions ---
function(sbgl_compile_shader_to_spv SHADER_SOURCE OUTPUT_SPV)
    find_program(GLSLC_EXE NAMES glslc)
    if(NOT GLSLC_EXE)
        message(FATAL_ERROR "glslc not found! Required for shader compilation.")
    endif()

    add_custom_command(
        OUTPUT ${OUTPUT_SPV}
        COMMAND ${GLSLC_EXE} ${SHADER_SOURCE} -o ${OUTPUT_SPV}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling ${SHADER_SOURCE} to ${OUTPUT_SPV}"
        VERBATIM
    )
endfunction()

function(sbgl_convert_spv_to_header INPUT_SPV OUTPUT_HEADER)
    find_program(XXD_EXE NAMES xxd)
    if(NOT XXD_EXE)
        message(FATAL_ERROR "xxd not found! Required for shader header generation.")
    endif()

    get_filename_component(OUTPUT_DIR ${OUTPUT_HEADER} DIRECTORY)
    get_filename_component(INPUT_NAME ${INPUT_SPV} NAME)
    
    add_custom_command(
        OUTPUT ${OUTPUT_HEADER}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${INPUT_SPV} ${OUTPUT_DIR}/${INPUT_NAME}
        COMMAND ${XXD_EXE} -i ${INPUT_NAME} > ${OUTPUT_HEADER}
        DEPENDS ${INPUT_SPV}
        WORKING_DIRECTORY ${OUTPUT_DIR}
        COMMENT "Converting ${INPUT_SPV} to C header ${OUTPUT_HEADER}"
        VERBATIM
    )
endfunction()

# --- Optional Examples & Tests ---
# Only build examples/tests if SBgl is the top-level project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if(SBGL_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
        add_subdirectory(examples)
    endif()
    if(SBGL_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    endif()
endif()

# --- Documentation ---
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/docs")
    
    add_custom_target(docs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DOXYGEN_OUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "OUTPUT_DIRECTORY = ${DOXYGEN_OUT_DIR}" > ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.extra
        COMMAND ${CMAKE_COMMAND} -E cat ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.extra > ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.combined
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.combined
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DOXYGEN_OUT_DIR}/html/img
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/docs/img ${DOXYGEN_OUT_DIR}/html/img
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )

    add_custom_target(docs-clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOXYGEN_OUT_DIR}
        COMMENT "Cleaning documentation..."
    )
endif()
