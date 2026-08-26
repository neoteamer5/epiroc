cmake_minimum_required(VERSION 3.10)
project(can_reader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

find_library(RT_LIB rt)
if(NOT RT_LIB)
    message(FATAL_ERROR "librt not found — required for CANReader")
endif()

find_path(SOCKETCAN_INCLUDE_DIR linux/can.h)
if(NOT SOCKETCAN_INCLUDE_DIR)
    message(FATAL_ERROR "SocketCAN headers not found (linux/can.h).")
endif()

include_directories(${SOCKETCAN_INCLUDE_DIR})

find_path(J1939_INCLUDE_DIR linux/can/j1939.h)
if(J1939_INCLUDE_DIR)
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_INCLUDES ${J1939_INCLUDE_DIR})
    check_symbol_exists(PF_J1939 "linux/can/j1939.h" HAVE_PF_J1939)

    if(HAVE_PF_J1939)
        add_definitions(-DHAVE_J1939)
        include_directories(${J1939_INCLUDE_DIR})
    endif()
endif()

# Build libcan_reader.so as a shared C API library for use by
# Python applications (via ctypes) or Qt-based applications.
add_library(can_reader SHARED
    api/can_reader_c_api.cpp
    api/CANReader.cpp
)

target_include_directories(can_reader
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
)

target_link_libraries(can_reader
    PUBLIC
        can-messages
        Threads::Threads
        ${RT_LIB}
)

# Build the Linux firmware demo application using libcan_reader.so.
# This demonstrates how the CAN reader can be integrated and used
# directly by a native firmware/application executable.
add_executable(can_reader_demo
    main.cpp
)

target_link_libraries(can_reader_demo
    PRIVATE
        can_reader
        Threads::Threads
        ${RT_LIB}
)

# Build a native C++ test executable to verify the C API exported
# by libcan_reader.so independently of the Python/Qt application.
add_executable(test_c_api
    api/test/test_c_api.cpp
)

target_link_libraries(test_c_api
    PRIVATE
        can_reader
        Threads::Threads
        ${RT_LIB}
)
