cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# CAN Messages Module
# ---------------------------------------------------------------------------
project(core LANGUAGES CXX)

add_library(core
    CanMessage.cpp
    CanProcessor.cpp
    CanReader.cpp
    CanWriter.cpp
    CommCan.cpp
)

target_include_directories(core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_compile_features(core PUBLIC cxx_std_17)
