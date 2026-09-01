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
#
# Dependency direction:
#
#   can_reader_demo
#       |
#       +----> can_reader
#       |        |
#       |        +----> CAN/J1939 infrastructure
#       |
#       +----> cooling-control
#       |        |
#       |        +----> CoolingController
#       |        +----> StateMachine
#       |        +----> PIDController
#       |
#       +----> Threads::Threads
#
# Application-specific components such as DashboardBackend,
# ControlTask, and TempHandler belong to the dashboard application.
#
# The dependency direction shall remain:
#
#   dashboard --> can_reader
#   dashboard --> cooling-control
#
# The following reverse dependencies shall be avoided:
#
#   can_reader       -X-> dashboard
#   cooling-control  -X-> dashboard
#   cooling-control  -X-> can_reader
#
# This keeps the CAN infrastructure reusable and keeps the
# cooling-control library independent of CAN transport and
# application scheduling.
add_executable(can_reader_demo
    main.cpp
    DashboardBackend.cpp
    ControlTask.cpp
    handlers/TempHandler.cpp
)

target_include_directories(can_reader_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/handlers
)

target_link_libraries(can_reader_demo
    PRIVATE
        can_reader
        cooling-control
        Threads::Threads
        ${RT_LIB}
)

add_subdirectory(api)
