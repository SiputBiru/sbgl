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
    )
endmacro()

macro(add_engine_test name src)
    if(SBGL_BUILD_STANDALONE)
        add_executable(${name} ${src} ${SBGL_SOURCES})
        
        # Ensure Wayland protocols are generated before compilation if needed
        if(SBGL_USE_WAYLAND AND NOT WIN32)
            add_dependencies(${name} wayland_protocols)
        endif()
        
        target_compile_definitions(${name} PRIVATE ${SBGL_PLATFORM_DEFINITIONS})
        
        target_include_directories(${name} PRIVATE 
            ${CMAKE_SOURCE_DIR}/include 
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/core
            ${PROJECT_BINARY_DIR}
        )
        target_link_libraries(${name} PRIVATE ${PLATFORM_LIBS} m)
    else()
        add_executable(${name} ${src})
        target_include_directories(${name} PRIVATE 
            ${CMAKE_SOURCE_DIR}/include 
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/core
        )
        target_link_libraries(${name} PRIVATE sbgl m)
    endif()
endmacro()

add_internal_test(math_test math_test.c)
add_internal_test(arena_test arena_test.c)
add_internal_test(core_flags_test core_flags_test.c)

# Engine tests (require core modules)
add_engine_test(batcher_test batcher_test.c)
add_engine_test(packet_packing_test packet_packing_test.c)
add_engine_test(sort_test sort_test.c)
add_engine_test(heightmap_test heightmap_test.c)
add_engine_test(voxel_logic_test voxel_logic_test.c)
