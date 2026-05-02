if(SBGL_BUILD_STANDALONE)
    # Standalone mode: Compile library sources directly into each executable
    foreach(example hello_window input_test)
        add_executable(${example} ${example}.c ${SBGL_SOURCES})
        
        target_include_directories(${example} PRIVATE 
            ${CMAKE_SOURCE_DIR}/include 
            ${CMAKE_SOURCE_DIR}/src
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

    add_executable(input_test input_test.c)
    target_link_libraries(input_test PRIVATE sbgl)
endif()
