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
    )
endmacro()

add_internal_test(math_test math_test.c)
add_internal_test(arena_test arena_test.c)
