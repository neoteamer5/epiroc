cmake_minimum_required(VERSION 3.22)

project(can_reader LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)

# Qt6 is required by the QML Dashboard.
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

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

# ---------------------------------------------------------------------------
# CAN Reader shared library
# ---------------------------------------------------------------------------
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
        core
        #Threads::Threads # for pthread_...
        #${RT_LIB} # for clock_... timer_...
)

# ---------------------------------------------------------------------------
# Dashboard backend library
# ---------------------------------------------------------------------------
add_library(dashboard_backend
    DashboardBackend.cpp
    ControlTask.cpp
    handlers/TempHandler.cpp
)

target_include_directories(dashboard_backend
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/handlers
)

target_link_libraries(dashboard_backend
    PUBLIC
        can_reader
        cooling-control
)

# ---------------------------------------------------------------------------
# Existing Linux firmware/demo application
# ---------------------------------------------------------------------------
add_executable(can_reader_demo
    main.cpp
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
        dashboard_backend
)


# ---------------------------------------------------------------------------
# API subdirectory
# ---------------------------------------------------------------------------
add_subdirectory(api)
add_subdirectory(qt-app)