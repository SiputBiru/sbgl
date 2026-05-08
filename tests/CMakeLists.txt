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
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include 
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
    )
    target_link_libraries(${name} PRIVATE sbgl m)
endmacro()

add_internal_test(math_test math_test.c)
add_internal_test(arena_test arena_test.c)

# Engine tests (require core modules)
add_engine_test(batcher_test batcher_test.c)
add_engine_test(sort_test sort_test.c)
add_engine_test(heightmap_test heightmap_test.c)
add_engine_test(voxel_logic_test voxel_logic_test.c)
