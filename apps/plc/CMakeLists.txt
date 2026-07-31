cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# PLC Simulator
# ---------------------------------------------------------------------------
project(plc LANGUAGES CXX)

add_executable(plc
    Simulator.cpp
)

target_link_libraries(plc
    PRIVATE
        can-messages
)

target_compile_features(plc PUBLIC cxx_std_17)
