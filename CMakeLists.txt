cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# Project
# ---------------------------------------------------------------------------
project(EVControlSystem
    VERSION 1.0.0
    DESCRIPTION "EV Central Control + Apps"
    LANGUAGES C CXX
)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(BUILD_APPS "Build application modules" ON)
option(BUILD_CORE "Build central control core" ON)

# ---------------------------------------------------------------------------
# Standard CMake testing support
#
# include(CTest) creates the standard BUILD_TESTING option.
# BUILD_TESTING defaults to ON.
#
# When BUILD_TESTING is ON, include(CTest) also calls enable_testing().
#
# Run all registered tests:
#   ctest --test-dir build --output-on-failure
#
# Run only selected registered tests by name:
#   ctest --test-dir build \
#       -R "CoolingCalibrationTest|StateMachineTest|PIDControllerTest|CoolingControllerTest" \
#       --output-on-failure
#
# Run the Cooling Control GTest executable directly:
#   ./build/bin/cooling_control_tests
# ---------------------------------------------------------------------------
include(CTest)

# ---------------------------------------------------------------------------
# Output directories
# ---------------------------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
# find_package(Threads REQUIRED)
# find_package(fmt REQUIRED)

# ---------------------------------------------------------------------------
# Core
# ---------------------------------------------------------------------------
if(BUILD_CORE)
    add_subdirectory(core)
endif()

# ---------------------------------------------------------------------------
# Applications
# ---------------------------------------------------------------------------
if(BUILD_APPS)
    add_subdirectory(apps)
endif()

# ---------------------------------------------------------------------------
# Optional top-level tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()