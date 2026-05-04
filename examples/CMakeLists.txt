if(SBGL_BUILD_STANDALONE)
    # ... (rest of standalone logic)
endif()

# Define shader files and generated outputs
set(TRI_VERT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert")
set(TRI_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag")
set(INT_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/interactive.frag")
set(PYR_VERT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/pyramid.vert")
set(PYR_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/pyramid.frag")

set(TRI_VERT_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.vert.spv")
set(TRI_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.frag.spv")
set(INT_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/interactive.frag.spv")
set(PYR_VERT_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/pyramid.vert.spv")
set(PYR_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/pyramid.frag.spv")

set(TRI_VERT_H "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle_vert.h")
set(TRI_FRAG_H "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle_frag.h")

# Ensure output directories exist
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")

# Register shader compilation tasks
sbgl_compile_shader_to_spv(${TRI_VERT} ${TRI_VERT_SPV})
sbgl_compile_shader_to_spv(${TRI_FRAG} ${TRI_FRAG_SPV})
sbgl_compile_shader_to_spv(${INT_FRAG} ${INT_FRAG_SPV})
sbgl_compile_shader_to_spv(${PYR_VERT} ${PYR_VERT_SPV})
sbgl_compile_shader_to_spv(${PYR_FRAG} ${PYR_FRAG_SPV})

sbgl_convert_spv_to_header(${TRI_VERT_SPV} ${TRI_VERT_H})
sbgl_convert_spv_to_header(${TRI_FRAG_SPV} ${TRI_FRAG_H})

# Create a target for shaders to help with dependencies
add_custom_target(example_shaders DEPENDS 
    ${TRI_VERT_SPV} ${TRI_FRAG_SPV} ${INT_FRAG_SPV} 
    ${PYR_VERT_SPV} ${PYR_FRAG_SPV}
    ${TRI_VERT_H} ${TRI_FRAG_H}
)

if(SBGL_BUILD_STANDALONE)
    # ...
    # Standalone mode: Compile library sources directly into each executable
    
    # In CMake < 3.18, the GENERATED property is directory-scoped.
    # must mark the Wayland protocol file as generated in this directory too.
    if(SBGL_USE_WAYLAND AND NOT WIN32)
        set_source_files_properties("${CMAKE_BINARY_DIR}/xdg-shell-protocol.c" PROPERTIES GENERATED TRUE)
    endif()

    foreach(example window/hello_window input/input_keyboard input/input_mouse test/math_test camera/cameraPersp_example camera/cameraOrtho_example camera/camera3d_pyramid triangle/draw_triangle triangle/draw_interactive_triangle triangle/draw_hardcoded_triangle)
        # Extract target name (e.g. hello_window from window/hello_window)
        get_filename_component(target_name ${example} NAME)
        # Special case for input targets to match old names if needed, 
        # but let's just use the filenames for simplicity.
        
        add_executable(${target_name} ${example}.c ${SBGL_SOURCES})
        
        add_dependencies(${target_name} example_shaders)

        if(SBGL_USE_WAYLAND AND NOT WIN32)
            add_dependencies(${target_name} wayland_protocols)
        endif()

        target_include_directories(${target_name} PRIVATE 
            ${CMAKE_SOURCE_DIR}/include 
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_BINARY_DIR}
        )
        
        target_link_libraries(${target_name} PRIVATE 
            ${PLATFORM_LIBS}
        )

        # Ensure Wayland protocols are generated before compilation
        if(SBGL_USE_WAYLAND AND NOT WIN32)
            set_source_files_properties(${CMAKE_SOURCE_DIR}/src/platform/linux/window_wayland.c PROPERTIES OBJECT_DEPENDS "${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h")
            set_source_files_properties(${CMAKE_SOURCE_DIR}/src/platform/linux/input_wayland.c PROPERTIES OBJECT_DEPENDS "${CMAKE_BINARY_DIR}/xdg-shell-client-protocol.h")
        endif()
    endforeach()
else()
    # Standard mode: Link against libsbgl.a
    add_executable(hello_window window/hello_window.c)
    target_link_libraries(hello_window PRIVATE sbgl)

    add_executable(input_keyboard input/input_keyboard.c)
    target_link_libraries(input_keyboard PRIVATE sbgl)

    add_executable(input_mouse input/input_mouse.c)
    target_link_libraries(input_mouse PRIVATE sbgl)

    add_executable(math_test test/math_test.c)
    target_link_libraries(math_test PRIVATE sbgl)

    add_executable(arena_test test/arena_test.c)
    target_link_libraries(arena_test PRIVATE sbgl)

    add_executable(cameraPersp_example camera/cameraPersp_example.c)
    target_link_libraries(cameraPersp_example PRIVATE sbgl)

    add_executable(cameraOrtho_example camera/cameraOrtho_example.c)
    target_link_libraries(cameraOrtho_example PRIVATE sbgl)

    add_executable(camera3d_pyramid camera/camera3d_pyramid.c)
    add_dependencies(camera3d_pyramid example_shaders)
    target_link_libraries(camera3d_pyramid PRIVATE sbgl)

    add_executable(draw_triangle triangle/draw_triangle.c)
    add_dependencies(draw_triangle example_shaders)
    target_link_libraries(draw_triangle PRIVATE sbgl)

    add_executable(draw_interactive_triangle triangle/draw_interactive_triangle.c)
    add_dependencies(draw_interactive_triangle example_shaders)
    target_link_libraries(draw_interactive_triangle PRIVATE sbgl)

    add_executable(draw_hardcoded_triangle triangle/draw_hardcoded_triangle.c)
    add_dependencies(draw_hardcoded_triangle example_shaders)
    target_include_directories(draw_hardcoded_triangle PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(draw_hardcoded_triangle PRIVATE sbgl)
endif()
