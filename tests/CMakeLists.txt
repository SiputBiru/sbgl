# Internal tests for math and memory management.
# These tests are self-contained and do not require the full engine graphics stack.

macro(add_internal_test name src)
    add_executable(${name} ${src})
    if(UNIX AND NOT APPLE)
        target_link_libraries(${name} PRIVATE m)
    endif()
    target_include_directories(${name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include 
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/voxel
    )
endmacro()

macro(add_engine_test name src)
    if(SBGL_BUILD_STANDALONE)
        # In CMake < 3.18, the GENERATED property is directory-scoped.
        # must mark the Wayland protocol file as generated in this directory too.
        if(SBGL_USE_WAYLAND AND NOT WIN32)
            set_source_files_properties("${PROJECT_BINARY_DIR}/xdg-shell-protocol.c" PROPERTIES GENERATED TRUE)
            set_source_files_properties("${PROJECT_BINARY_DIR}/pointer-constraints-unstable-v1-protocol.c" PROPERTIES GENERATED TRUE)
            set_source_files_properties("${PROJECT_BINARY_DIR}/relative-pointer-unstable-v1-protocol.c" PROPERTIES GENERATED TRUE)
        endif()

        add_executable(${name} ${src} ${SBGL_SOURCES})
        
        add_dependencies(${name} example_shaders)

        # Ensure Wayland protocols are generated before compilation if needed
        if(SBGL_USE_WAYLAND AND NOT WIN32)
            add_dependencies(${name} wayland_protocols)
        endif()
        
        target_compile_definitions(${name} PRIVATE ${SBGL_PLATFORM_DEFINITIONS})
        
        target_include_directories(${name} PRIVATE 
            ${CMAKE_SOURCE_DIR}/include 
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/core
            ${CMAKE_SOURCE_DIR}/src/voxel
            ${PROJECT_BINARY_DIR}
        )
        target_link_libraries(${name} PRIVATE ${PLATFORM_LIBS} m volk)
    else()
        add_executable(${name} ${src})
        add_dependencies(${name} example_shaders)
        target_include_directories(${name} PRIVATE 
            ${CMAKE_SOURCE_DIR}/include 
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/core
            ${CMAKE_SOURCE_DIR}/src/voxel
        )
        target_link_libraries(${name} PRIVATE sbgl m)
    endif()
endmacro()

add_internal_test(math_test math_test.c)
add_internal_test(arena_test arena_test.c)
add_internal_test(core_flags_test core_flags_test.c)
add_internal_test(checkerboard_logic_test checkerboard_logic_test.c)
add_internal_test(voxel_bitmask_test voxel_bitmask_test.c)
add_internal_test(input_test input_test.c)
add_internal_test(gpu_memory_test gpu_memory_test.c)

if(SBGL_BUILD_STANDALONE)
    add_executable(voxel_pool_test voxel_pool_test.c ${SBGL_SOURCES})
    target_link_libraries(voxel_pool_test PRIVATE ${PLATFORM_LIBS} m volk)
    target_compile_definitions(voxel_pool_test PRIVATE ${SBGL_PLATFORM_DEFINITIONS})
else()
    add_executable(voxel_pool_test voxel_pool_test.c)
    target_link_libraries(voxel_pool_test PRIVATE sbgl m)
endif()
target_include_directories(voxel_pool_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/voxel
    ${CMAKE_SOURCE_DIR}/examples
    ${PROJECT_BINARY_DIR}
)

# Engine tests (require core modules)
add_engine_test(batcher_test batcher_test.c)
add_engine_test(packet_packing_test packet_packing_test.c)
add_engine_test(sort_test sort_test.c)
add_engine_test(heightmap_test heightmap_test.c)
add_engine_test(voxel_logic_test voxel_logic_test.c)
add_engine_test(compute_test compute_test.c)
add_engine_test(voxel_compute_test voxel_compute_test.c)
add_engine_test(voxel_visibility_test voxel_visibility_test.c)
