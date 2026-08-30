# Build a native C++ test executable to verify the C API exported
# by libcan_reader.so independently of the Python/Qt application.
add_executable(test_c_api
    test/test_c_api.cpp
)

target_link_libraries(test_c_api
    PRIVATE
        can_reader
        Threads::Threads
        ${RT_LIB}
)