cmake_minimum_required(VERSION 3.10)
project(can_reader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Build the library
add_library(can_reader
    SHARED
        can_reader.cpp
        can_reader_c_api.cpp
        CANReader.cpp

)

# 2. Include directory (optional but clean)
target_include_directories(can_reader
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# 3. Build the executable
add_executable(can_reader_demo
    main.cpp
)
target_link_libraries(can_reader_demo
    can_reader
    rt
)

# 4. for test
add_executable(test_c_api
    test_c_api.cpp
)
target_link_libraries(test_c_api
    can_reader
    rt
)

