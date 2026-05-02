add_executable(hello_window hello_window.c)
target_link_libraries(hello_window PRIVATE sbgl)

add_executable(input_test input_test.c)
target_link_libraries(input_test PRIVATE sbgl)
