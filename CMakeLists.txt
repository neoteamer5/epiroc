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
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_APPS  "Build application modules" ON)
option(BUILD_CORE  "Build central control core" ON)

# ---------------------------------------------------------------------------
# Output directories
# ---------------------------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------------------------------------------
# Testing
#
# Must be enabled before adding subdirectories that register tests.
# ---------------------------------------------------------------------------
if(BUILD_TESTS)
    enable_testing()
endif()

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
# Additional top-level tests
# ---------------------------------------------------------------------------
# Only keep this if ~/epiroc/tests/CMakeLists.txt actually exists and
# contains additional project-level tests.
#
# if(BUILD_TESTS)
#     add_subdirectory(tests)
# endif()
