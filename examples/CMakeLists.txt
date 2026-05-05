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
    # Standalone mode: Compile library sources directly into each executable
    
    # In CMake < 3.18, the GENERATED property is directory-scoped.
    # must mark the Wayland protocol file as generated in this directory too.
    if(SBGL_USE_WAYLAND AND NOT WIN32)
        set_source_files_properties("${CMAKE_BINARY_DIR}/xdg-shell-protocol.c" PROPERTIES GENERATED TRUE)
    endif()

    foreach(example window/hello_window input/input_keyboard input/input_mouse camera/camera_main triangle/triangle_main)
        # Extract target name (e.g. hello_window from window/hello_window)
        get_filename_component(target_name ${example} NAME)
        
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
    # Standard mode: Link against libsbgl.Static library
    
    macro(add_example name src)
        add_executable(${name} ${src})
        add_dependencies(${name} example_shaders)
        target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
        target_link_libraries(${name} PRIVATE sbgl)
    endmacro()

    add_example(hello_window window/hello_window.c)
    add_example(input_keyboard input/input_keyboard.c)
    add_example(input_mouse input/input_mouse.c)
    add_example(camera_main camera/camera_main.c)
    add_example(triangle_main triangle/triangle_main.c)
endif()
