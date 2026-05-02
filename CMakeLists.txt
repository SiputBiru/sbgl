cmake_minimum_required(VERSION 3.15)
project(SBgl VERSION 0.1.0 LANGUAGES C)

# --- LSP Support ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Options ---
option(SBGL_USE_WAYLAND "Use native Wayland instead of X11" ON)

# --- Compiler Settings ---
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /wd4201)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Find Dependencies ---
find_package(Vulkan REQUIRED)

# --- Source Files ---
set(SBGL_SOURCES
    src/core/sbgl_core.c
    src/backend/sbgl_backend_vulkan.c
)

set(PLATFORM_LIBS "")

if(WIN32)
    # Placeholder for future Win32 split
    list(APPEND SBGL_SOURCES src/platform/win32/window.c src/platform/win32/input.c)
    list(APPEND PLATFORM_LIBS user32 gdi32 shell32)
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
        
        list(APPEND SBGL_SOURCES 
            src/platform/linux/window.c 
            src/platform/linux/input.c
            "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c"
        )
        include_directories("${CMAKE_CURRENT_BINARY_DIR}")
        list(APPEND PLATFORM_LIBS ${WAYLAND_CLIENT_LIBRARIES})
        add_definitions(-DSBGL_PLATFORM_WAYLAND)
    else()
        # Placeholder for X11 split
        find_package(X11 REQUIRED)
        list(APPEND SBGL_SOURCES src/platform/linux/window_x11.c src/platform/linux/input_x11.c)
        list(APPEND PLATFORM_LIBS X11)
        add_definitions(-DSBGL_PLATFORM_X11)
    endif()
endif()

# --- Library Target ---
add_library(sbgl STATIC ${SBGL_SOURCES})

if(SBGL_USE_WAYLAND AND NOT WIN32)
    set_source_files_properties(src/platform/linux/window.c PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
    set_source_files_properties(src/platform/linux/input.c PROPERTIES OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")
endif()

# --- Include Directories ---
target_include_directories(sbgl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
)

# --- Link Libraries ---
target_link_libraries(sbgl
    PRIVATE
        Vulkan::Vulkan
        ${PLATFORM_LIBS}
)

# --- Optional Examples ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()
