cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# Apps Module (PLC + Dashboard + Common CAN Messages)
# ---------------------------------------------------------------------------
project(apps LANGUAGES CXX)

# Common CAN message utilities
add_subdirectory(common)

# PLC application
add_subdirectory(plc)

# Dashboard application
add_subdirectory(dashboard)
