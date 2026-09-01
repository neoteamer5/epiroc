# ---------------------------------------------------------------------------
# Cooling Control Library
# ---------------------------------------------------------------------------

add_library(cooling-control STATIC
    CoolingControl.cpp
)

target_include_directories(cooling-control
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(cooling-control
    PUBLIC
        cxx_std_17
)

# ---------------------------------------------------------------------------
# Unit Tests
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    add_subdirectory(test)
endif()