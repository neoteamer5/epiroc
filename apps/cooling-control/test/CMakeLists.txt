# ---------------------------------------------------------------------------
# Cooling Control Unit Tests
# ---------------------------------------------------------------------------

find_package(GTest REQUIRED)

add_executable(cooling-control-tests
    StateMachineTest.cpp
    PIDControllerTest.cpp
    CoolingControllerTest.cpp
)

target_link_libraries(cooling-control-tests
    PRIVATE
        cooling-control
        GTest::gtest_main
)

target_compile_features(cooling-control-tests
    PRIVATE
        cxx_std_17
)

include(GoogleTest)

gtest_discover_tests(cooling-control-tests)
