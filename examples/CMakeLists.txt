if(SBGL_BUILD_STANDALONE)
    # Standalone mode: Compile library sources directly into each executable
    
    # In CMake < 3.18, the GENERATED property is directory-scoped.
    # We must mark the Wayland protocol file as generated in this directory too.
    if(SBGL_USE_WAYLAND AND NOT WIN32)
        set_source_files_properties("${CMAKE_BINARY_DIR}/xdg-shell-protocol.c" PROPERTIES GENERATED TRUE)
    endif()

    foreach(example hello_window input_test input_mouse)
        add_executable(${example} ${example}.c ${SBGL_SOURCES})
        
        if(SBGL_USE_WAYLAND AND NOT WIN32)
            add_dependencies(${example} wayland_protocols)
        endif()

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

    add_executable(input_mouse input_mouse.c)
    target_link_libraries(input_mouse PRIVATE sbgl)
endif()
