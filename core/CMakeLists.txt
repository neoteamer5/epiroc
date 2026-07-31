cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# CAN Messages Module
# ---------------------------------------------------------------------------
project(can-messages LANGUAGES CXX)

add_library(can-messages
    CanMessages.cpp
    SocketCAN.cpp
)

target_include_directories(can-messages
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(can-messages PUBLIC cxx_std_17)
