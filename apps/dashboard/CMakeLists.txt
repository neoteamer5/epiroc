cmake_minimum_required(VERSION 3.10)
project(can_reader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# 1. Dependencies
# -----------------------------

# Threads (required for std::thread)
find_package(Threads REQUIRED)

# RT library (Linux only)
find_library(RT_LIB rt)
if(NOT RT_LIB)
    message(FATAL_ERROR "librt not found — required for CANReader")
endif()

# SocketCAN headers (Linux only)
find_path(SOCKETCAN_INCLUDE_DIR linux/can.h)
if(NOT SOCKETCAN_INCLUDE_DIR)
    message(FATAL_ERROR "SocketCAN headers not found (linux/can.h). Install: sudo apt install linux-headers-$(uname -r)")
endif()

include_directories(${SOCKETCAN_INCLUDE_DIR})

# Optional J1939 support (Linux only)
find_path(J1939_INCLUDE_DIR linux/can/j1939.h)
if(J1939_INCLUDE_DIR)
    # Check if PF_J1939 is actually defined in the header
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_INCLUDES ${J1939_INCLUDE_DIR})
    check_symbol_exists(PF_J1939 "linux/can/j1939.h" HAVE_PF_J1939)

    if(HAVE_PF_J1939)
        message(STATUS "J1939 stack fully supported")
        add_definitions(-DHAVE_J1939)
        include_directories(${J1939_INCLUDE_DIR})
    else()
        message(WARNING "J1939 header found, but PF_J1939 is NOT defined — kernel does NOT support J1939")
    endif()
else()
    message(WARNING "J1939 header not found — building WITHOUT PF_J1939 support")
endif()


# -----------------------------
# 2. Build shared library
# -----------------------------
add_library(can_reader SHARED
    can_reader.cpp
    can_reader_c_api.cpp
    CANReader.cpp
)

target_include_directories(can_reader
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(can_reader
    Threads::Threads
    ${RT_LIB}
)

# -----------------------------
# 3. Build main demo executable
# -----------------------------
add_executable(can_reader_demo
    main.cpp
)

target_link_libraries(can_reader_demo
    can_reader
    Threads::Threads
    ${RT_LIB}
)

# -----------------------------
# 4. Build C API test executable
# -----------------------------
add_executable(test_c_api
    test_c_api.cpp
)

target_link_libraries(test_c_api
    can_reader
    Threads::Threads
    ${RT_LIB}
)


