if(SBGL_BUILD_STANDALONE)
    # ... (rest of standalone logic)
endif()

# Define shader files and generated outputs
set(TRI_VERT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert")
set(TRI_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag")
set(INT_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/interactive.frag")

set(TRI_VERT_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.vert.spv")
set(TRI_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.frag.spv")
set(INT_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/interactive.frag.spv")

set(TRI_VERT_H "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle_vert.h")
set(TRI_FRAG_H "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle_frag.h")

# Ensure output directories exist
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")

# Register shader compilation tasks
sbgl_compile_shader_to_spv(${TRI_VERT} ${TRI_VERT_SPV})
sbgl_compile_shader_to_spv(${TRI_FRAG} ${TRI_FRAG_SPV})
sbgl_compile_shader_to_spv(${INT_FRAG} ${INT_FRAG_SPV})

sbgl_convert_spv_to_header(${TRI_VERT_SPV} ${TRI_VERT_H})
sbgl_convert_spv_to_header(${TRI_FRAG_SPV} ${TRI_FRAG_H})

# Create a target for shaders to help with dependencies
add_custom_target(example_shaders DEPENDS 
    ${TRI_VERT_SPV} ${TRI_FRAG_SPV} ${INT_FRAG_SPV} 
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

    foreach(example hello_window input_keyboard input_mouse math_test cameraPersp_example cameraOrtho_example draw_triangle draw_interactive_triangle draw_hardcoded_triangle)
        add_executable(${example} ${example}.c ${SBGL_SOURCES})
        
        add_dependencies(${example} example_shaders)

        if(SBGL_USE_WAYLAND AND NOT WIN32)
            add_dependencies(${example} wayland_protocols)
        endif()

        target_include_directories(${example} PRIVATE 
            ${CMAKE_SOURCE_DIR}/include 
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_BINARY_DIR}
        )
        
        target_link_libraries(${example} PRIVATE 
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
    add_executable(hello_window hello_window.c)
    target_link_libraries(hello_window PRIVATE sbgl)

    add_executable(input_keyboard input_keyboard.c)
    target_link_libraries(input_keyboard PRIVATE sbgl)

    add_executable(input_mouse input_mouse.c)
    target_link_libraries(input_mouse PRIVATE sbgl)

    add_executable(math_test math_test.c)
    target_link_libraries(math_test PRIVATE sbgl)

    add_executable(cameraPersp_example cameraPersp_example.c)
    target_link_libraries(cameraPersp_example PRIVATE sbgl)

    add_executable(cameraOrtho_example cameraOrtho_example.c)
    target_link_libraries(cameraOrtho_example PRIVATE sbgl)

    add_executable(draw_triangle draw_triangle.c)
    add_dependencies(draw_triangle example_shaders)
    target_link_libraries(draw_triangle PRIVATE sbgl)

    add_executable(draw_interactive_triangle draw_interactive_triangle.c)
    add_dependencies(draw_interactive_triangle example_shaders)
    target_link_libraries(draw_interactive_triangle PRIVATE sbgl)

    add_executable(draw_hardcoded_triangle draw_hardcoded_triangle.c)
    add_dependencies(draw_hardcoded_triangle example_shaders)
    target_include_directories(draw_hardcoded_triangle PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(draw_hardcoded_triangle PRIVATE sbgl)
endif()
