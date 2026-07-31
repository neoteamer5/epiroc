cmake_minimum_required(VERSION 3.22)

# Project metadata
project(EVControlSystem
    VERSION 1.0.0
    DESCRIPTION "EV Central Control + Apps"
    LANGUAGES C CXX
)

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_APPS  "Build application modules" ON)
option(BUILD_CORE  "Build central control core" ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Dependencies (extend as needed)
# find_package(Threads REQUIRED)
# find_package(fmt REQUIRED)

# Subdirectories
if(BUILD_CORE)
    add_subdirectory(core)
endif()

if(BUILD_APPS)
    add_subdirectory(apps)
endif()

# Optional tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
