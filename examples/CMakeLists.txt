# Example executables for sbgl
#
# Shader compilation is handled unconditionally by examples/shaders/CMakeLists.txt
# (added from the top-level CMakeLists.txt). The generated shader headers are at
# ${PROJECT_BINARY_DIR}/example_shaders/shaders/.

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
            ${CMAKE_SOURCE_DIR}/src/core
            ${CMAKE_SOURCE_DIR}/src/voxel
            ${CMAKE_CURRENT_BINARY_DIR}
            ${PROJECT_BINARY_DIR}/example_shaders/shaders
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
        ${PROJECT_BINARY_DIR}/example_shaders/shaders
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
            ${PROJECT_BINARY_DIR}/example_shaders/shaders
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
