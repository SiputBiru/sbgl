cmake_minimum_required(VERSION 3.15)
project(SBgl VERSION 0.1.0 LANGUAGES C)

# --- Enable Testing ---
enable_testing()

# --- LSP Support ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Options ---
option(SBGL_DOCS_ONLY "Only build documentation (bypass all system dependencies)" OFF)

if(NOT SBGL_DOCS_ONLY)
  # --- Default Build Type ---
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  endif()

  option(SBGL_USE_WAYLAND "Use native Wayland instead of X11" ON)
  option(SBGL_BUILD_EXAMPLES "Build example applications" OFF)
  option(SBGL_BUILD_TESTS "Build test applications" OFF)
  option(SBGL_BUILD_STANDALONE "Build examples by compiling sources directly (no libsbgl.a)" OFF)

  # --- Compiler Settings ---
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED ON)

  # --- Tool Discovery ---
  # The build process requires specific external utilities for shader compilation
  # and data embedding. These programs are located once at the top level to
  # ensure they are available for all sub-modules and functions.
  find_program(GLSLC_EXE NAMES glslc)
  find_program(XXD_EXE NAMES xxd)

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

  # --- Source Files ---
  set(SBGL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sbgl_core.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sbgl_log.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sbgl_sort.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/sbgl_batcher.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/voxel/sbgl_voxel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/voxel/sbgl_pool.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/backend/sbgl_backend_vulkan.c
    )

  set(PLATFORM_LIBS "")
  set(SBGL_PLATFORM_DEFINITIONS "")

  if(WIN32)
    list(APPEND SBGL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win32/window.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/win32/input.c
        )
    if(MINGW)
      list(APPEND PLATFORM_LIBS user32 gdi32 shell32)
    endif()
  elseif(UNIX AND NOT APPLE)
    if(SBGL_USE_WAYLAND)
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
      pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)

      # The path to the system-installed Wayland protocol XML files is
      # retrieved via pkg-config to ensure compatibility across different
      # Linux distributions.
      pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

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
      list(APPEND PLATFORM_LIBS ${WAYLAND_CLIENT_LIBRARIES} rt)
      list(APPEND SBGL_PLATFORM_DEFINITIONS SBGL_PLATFORM_WAYLAND)
    else()
      find_package(X11 REQUIRED)
      list(APPEND SBGL_SOURCES
                ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/window_x11.c
                ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/linux/input_x11.c
            )
      list(APPEND PLATFORM_LIBS X11 Xfixes m)
      list(APPEND SBGL_PLATFORM_DEFINITIONS SBGL_PLATFORM_X11)
    endif()
  endif()

  # --- Volk Integration ---
  # SBgl uses volk to manage Vulkan function pointers and handle multi-context
  # dispatching without the overhead of static linking to the Vulkan loader.
  include(FetchContent)
  FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk.git
        GIT_TAG vulkan-sdk-1.4.350
    )
  FetchContent_MakeAvailable(volk)

  if(WIN32)
    target_compile_definitions(volk INTERFACE VK_USE_PLATFORM_WIN32_KHR)
    target_compile_definitions(volk PRIVATE VK_USE_PLATFORM_WIN32_KHR)
  elseif(UNIX AND NOT APPLE)
    if(SBGL_USE_WAYLAND)
      target_compile_definitions(volk INTERFACE VK_USE_PLATFORM_WAYLAND_KHR)
      target_compile_definitions(volk PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
    else()
      target_compile_definitions(volk INTERFACE VK_USE_PLATFORM_XLIB_KHR)
      target_compile_definitions(volk PRIVATE VK_USE_PLATFORM_XLIB_KHR)
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

    target_compile_definitions(sbgl PRIVATE ${SBGL_PLATFORM_DEFINITIONS})

    target_include_directories(sbgl
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_BINARY_DIR}
        )

    if(WIN32 AND MINGW)
      target_include_directories(sbgl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
    endif()

    # Allow subprojects to find the headers
    target_include_directories(sbgl SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_libraries(sbgl
            PRIVATE
                ${PLATFORM_LIBS}
                volk
        )
  endif()

  # --- Shader Compilation Functions ---

  # Shaders are compiled from GLSL source files into SPIR-V binary format
  # using the Vulkan shader compiler. This transformation is necessary for
  # the graphics backend to process and execute the shader logic on the GPU.
  function(sbgl_compile_shader_to_spv SHADER_SOURCE OUTPUT_SPV)
    if(NOT GLSLC_EXE)
      message(FATAL_ERROR "The shader compiler (glslc) was not found during tool discovery.")
    endif()

    add_custom_command(
            OUTPUT ${OUTPUT_SPV}
            COMMAND ${GLSLC_EXE} ${SHADER_SOURCE} -o ${OUTPUT_SPV}
            DEPENDS ${SHADER_SOURCE}
            COMMENT "Compiling ${SHADER_SOURCE} to ${OUTPUT_SPV}"
            VERBATIM
        )
  endfunction()

  # available to the rendering system.
  function(sbgl_convert_spv_to_header INPUT_SPV OUTPUT_HEADER)
    if(NOT XXD_EXE)
      message(FATAL_ERROR "The data embedding utility (xxd) was not found during tool discovery.")
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

  # UNCONDITIONAL: Shader compilation (library depends on these headers) ===
  # sbgl_voxel.c includes generated headers like voxel_gen.h. This shader
  # compilation must happen whether sbgl is the top-level project or fetched
  # as a dependency, so the library always has access to its required headers.
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/shaders/CMakeLists.txt")
    add_subdirectory(examples/shaders ${CMAKE_CURRENT_BINARY_DIR}/example_shaders)
  endif()
  if(NOT SBGL_BUILD_STANDALONE)
    target_include_directories(sbgl PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/example_shaders/shaders)
    add_dependencies(sbgl example_shaders)
  endif()

  # CONDITIONAL: Example executables and tests (only when sbgl is top-level) ===
  if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if(SBGL_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
      add_subdirectory(examples)
    endif()
    if(SBGL_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
      add_subdirectory(tests)
    endif()
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
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )

  add_custom_target(docs-clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOXYGEN_OUT_DIR}
        COMMENT "Cleaning documentation..."
    )
endif()
