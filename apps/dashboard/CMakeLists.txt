cmake_minimum_required(VERSION 3.10)
project(can_reader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Build the library
add_library(can_reader
    can_reader.cpp

)

# 2. Include directory (optional but clean)
target_include_directories(can_reader
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# 3. Build the executable
add_executable(can_reader_demo
    main.cpp
)

# 4. Link the library + rt
target_link_libraries(can_reader_demo
    can_reader
    rt
)
