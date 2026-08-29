enable_testing()

add_library(cooling_control
    CoolingControl.cpp
)

target_include_directories(cooling_control
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(cooling_control PUBLIC cxx_std_17)

add_subdirectory(test)