cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# Apps Module (PLC + Dashboard)
# ---------------------------------------------------------------------------
project(apps LANGUAGES CXX)

# PLC application
add_subdirectory(plc)

# Dashboard application
add_subdirectory(dashboard)
