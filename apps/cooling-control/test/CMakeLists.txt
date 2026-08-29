# ---------------------------------------------------------------------------
# Cooling Control Unit Tests
# ---------------------------------------------------------------------------

find_package(GTest REQUIRED)

add_executable(cooling_control_tests
    StateMachineTest.cpp
    PIDControllerTest.cpp
    CoolingControllerTest.cpp
)

target_link_libraries(cooling_control_tests
    PRIVATE
        cooling_control
        GTest::gtest_main
)

target_compile_features(cooling_control_tests
    PRIVATE
        cxx_std_17
)

include(GoogleTest)

gtest_discover_tests(cooling_control_tests)
