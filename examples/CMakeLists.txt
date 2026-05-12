# Define shader files and generated outputs
set(TRI_VERT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.vert")
set(TRI_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/triangle.frag")
set(INT_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/interactive.frag")
set(PYR_VERT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/pyramid.vert")
set(PYR_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/pyramid.frag")
set(BATCH_VERT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/batching.vert")
set(BATCH_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/batching.frag")
set(VOXEL_VERT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/voxel.vert")
set(VOXEL_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/voxel.frag")
set(VOXEL3D_VERT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/voxel3D.vert")
set(VOXEL3D_FRAG "${CMAKE_CURRENT_SOURCE_DIR}/shaders/voxel3D.frag")
set(VOXEL_CULL "${CMAKE_CURRENT_SOURCE_DIR}/shaders/voxel_cull.comp")
set(VOXEL_GEN "${CMAKE_CURRENT_SOURCE_DIR}/shaders/voxel_gen.comp")
set(VOXEL_SHELL "${CMAKE_CURRENT_SOURCE_DIR}/shaders/voxel_shell.comp")
set(VOXEL_MESH "${CMAKE_CURRENT_SOURCE_DIR}/shaders/voxel_mesh.comp")
set(COMPUTE_TEST "${CMAKE_CURRENT_SOURCE_DIR}/shaders/compute_test.comp")

set(TRI_VERT_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.vert.spv")
set(TRI_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/triangle.frag.spv")
set(INT_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/interactive.frag.spv")
set(PYR_VERT_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/pyramid.vert.spv")
set(PYR_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/pyramid.frag.spv")
set(BATCH_VERT_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/batching.vert.spv")
set(BATCH_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/batching.frag.spv")
set(VOXEL_VERT_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/voxel.vert.spv")
set(VOXEL_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/voxel.frag.spv")
set(VOXEL3D_VERT_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/voxel3D.vert.spv")
set(VOXEL3D_FRAG_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/voxel3D.frag.spv")
set(VOXEL_CULL_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/voxel_cull.comp.spv")
set(VOXEL_GEN_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/voxel_gen.comp.spv")
set(VOXEL_SHELL_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/voxel_shell.comp.spv")
set(VOXEL_MESH_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/voxel_mesh.comp.spv")
set(COMPUTE_TEST_SPV "${CMAKE_CURRENT_BINARY_DIR}/shaders/compute_test.comp.spv")

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
sbgl_compile_shader_to_spv(${BATCH_VERT} ${BATCH_VERT_SPV})
sbgl_compile_shader_to_spv(${BATCH_FRAG} ${BATCH_FRAG_SPV})
sbgl_compile_shader_to_spv(${VOXEL_VERT} ${VOXEL_VERT_SPV})
sbgl_compile_shader_to_spv(${VOXEL_FRAG} ${VOXEL_FRAG_SPV})
sbgl_compile_shader_to_spv(${VOXEL3D_VERT} ${VOXEL3D_VERT_SPV})
sbgl_compile_shader_to_spv(${VOXEL3D_FRAG} ${VOXEL3D_FRAG_SPV})
sbgl_compile_shader_to_spv(${VOXEL_CULL} ${VOXEL_CULL_SPV})
sbgl_compile_shader_to_spv(${VOXEL_GEN} ${VOXEL_GEN_SPV})
sbgl_compile_shader_to_spv(${VOXEL_SHELL} ${VOXEL_SHELL_SPV})
sbgl_compile_shader_to_spv(${VOXEL_MESH} ${VOXEL_MESH_SPV})
sbgl_compile_shader_to_spv(${COMPUTE_TEST} ${COMPUTE_TEST_SPV})

sbgl_convert_spv_to_header(${TRI_VERT_SPV} ${TRI_VERT_H})
sbgl_convert_spv_to_header(${TRI_FRAG_SPV} ${TRI_FRAG_H})

# Create a target for shaders to help with dependencies
add_custom_target(example_shaders DEPENDS 
    ${TRI_VERT_SPV} ${TRI_FRAG_SPV} ${INT_FRAG_SPV} 
    ${PYR_VERT_SPV} ${PYR_FRAG_SPV}
    ${BATCH_VERT_SPV} ${BATCH_FRAG_SPV}
    ${VOXEL_VERT_SPV} ${VOXEL_FRAG_SPV}
    ${VOXEL3D_VERT_SPV} ${VOXEL3D_FRAG_SPV} ${VOXEL_CULL_SPV}
    ${VOXEL_GEN_SPV} ${VOXEL_SHELL_SPV} ${VOXEL_MESH_SPV}
    ${COMPUTE_TEST_SPV}
    ${TRI_VERT_H} ${TRI_FRAG_H}
)

if(SBGL_BUILD_STANDALONE)
    # Standalone mode: Compile library sources directly into each executable
    
    # In CMake < 3.18, the GENERATED property is directory-scoped.
    # must mark the Wayland protocol file as generated in this directory too.
    if(SBGL_USE_WAYLAND AND NOT WIN32)
        set_source_files_properties("${PROJECT_BINARY_DIR}/xdg-shell-protocol.c" PROPERTIES GENERATED TRUE)
        set_source_files_properties("${PROJECT_BINARY_DIR}/pointer-constraints-unstable-v1-protocol.c" PROPERTIES GENERATED TRUE)
        set_source_files_properties("${PROJECT_BINARY_DIR}/relative-pointer-unstable-v1-protocol.c" PROPERTIES GENERATED TRUE)
    endif()

    foreach(example window/hello_window input/input_keyboard input/input_mouse input/mouse_mode camera/camera_main triangle/triangle_main batching/batch_main voxels/voxel_main)
        # Extract target name (e.g. hello_window from window/hello_window)
        get_filename_component(target_name ${example} NAME)
        
        add_executable(${target_name} ${example}.c ${SBGL_SOURCES})
        
        add_dependencies(${target_name} example_shaders)

        if(SBGL_USE_WAYLAND AND NOT WIN32)
            add_dependencies(${target_name} wayland_protocols)
        endif()

        target_compile_definitions(${target_name} PRIVATE ${SBGL_PLATFORM_DEFINITIONS})

        target_include_directories(${target_name} PRIVATE 
            ${CMAKE_SOURCE_DIR}/include 
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_CURRENT_BINARY_DIR}
            ${PROJECT_BINARY_DIR}
        )
        
        target_link_libraries(${target_name} PRIVATE 
            ${PLATFORM_LIBS}
            volk
        )

        # Ensure Wayland protocols are generated before compilation
        if(SBGL_USE_WAYLAND AND NOT WIN32)
            set_source_files_properties(${CMAKE_SOURCE_DIR}/src/platform/linux/window_wayland.c PROPERTIES OBJECT_DEPENDS "${PROJECT_BINARY_DIR}/xdg-shell-client-protocol.h")
            set_source_files_properties(${CMAKE_SOURCE_DIR}/src/platform/linux/input_wayland.c PROPERTIES OBJECT_DEPENDS "${PROJECT_BINARY_DIR}/xdg-shell-client-protocol.h")
        endif()
    endforeach()

    # Voxel3D_main
    add_executable(voxel3D_main voxels/voxel3D_main.c ${SBGL_SOURCES})
    add_dependencies(voxel3D_main example_shaders)
    if(SBGL_USE_WAYLAND AND NOT WIN32)
        add_dependencies(voxel3D_main wayland_protocols)
    endif()
    target_compile_definitions(voxel3D_main PRIVATE ${SBGL_PLATFORM_DEFINITIONS})
    target_include_directories(voxel3D_main PRIVATE 
        ${CMAKE_SOURCE_DIR}/include 
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/examples/voxels
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_BINARY_DIR}
    )
    target_link_libraries(voxel3D_main PRIVATE ${PLATFORM_LIBS} volk)
else()
    # Standard mode: Link against libsbgl.Static library
    
    macro(add_example name src)
        add_executable(${name} ${src})
        add_dependencies(${name} example_shaders)

        target_compile_definitions(${name} PRIVATE ${SBGL_PLATFORM_DEFINITIONS})

        target_include_directories(${name} PRIVATE 
            ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/examples/voxels
        )
        target_link_libraries(${name} PRIVATE sbgl m)
    endmacro()

    add_example(hello_window window/hello_window.c)
    add_example(input_keyboard input/input_keyboard.c)
    add_example(input_mouse input/input_mouse.c)
    add_example(mouse_mode input/mouse_mode.c)
    add_example(camera_main camera/camera_main.c)
    add_example(triangle_main triangle/triangle_main.c)
    add_example(batch_main batching/batch_main.c)
    add_example(voxel_main voxels/voxel_main.c)
    add_example(voxel3D_main voxels/voxel3D_main.c)
endif()
