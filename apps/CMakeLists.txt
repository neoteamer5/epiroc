cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# Apps Module (PLC + Dashboard)
# ---------------------------------------------------------------------------
project(apps LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Cooling control library
#
# Provides:
#   - CoolingController
#   - StateMachine
#   - PIDController
#   - CoolingThresholds
#
# The Dashboard application links against this library.
# ---------------------------------------------------------------------------
add_subdirectory(cooling-control)

# ---------------------------------------------------------------------------
# PLC application
# ---------------------------------------------------------------------------
add_subdirectory(plc)

# ---------------------------------------------------------------------------
# Dashboard application
# ---------------------------------------------------------------------------
add_subdirectory(dashboard)