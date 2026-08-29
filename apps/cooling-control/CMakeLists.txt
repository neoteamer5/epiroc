# ---------------------------------------------------------------------------
# Cooling Control Library
# ---------------------------------------------------------------------------

add_library(cooling_control STATIC
    CoolingControl.cpp
)

target_include_directories(cooling_control
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(cooling_control
    PUBLIC
        cxx_std_17
)

# ---------------------------------------------------------------------------
# Unit Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    add_subdirectory(test)
endif()