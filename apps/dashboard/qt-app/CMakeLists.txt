cmake_minimum_required(VERSION 3.22)

# ---------------------------------------------------------------------------
# Qt dependencies
# ---------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    QuickLayouts
)

qt_standard_project_setup()

# ---------------------------------------------------------------------------
# Native C++ / QML Dashboard
# ---------------------------------------------------------------------------
qt_add_executable(CoolingDashboard
    qml_main.cpp

    QtDashboardBackend.cpp
    QtDashboardBackend.hpp
)

qt_add_qml_module(CoolingDashboard
    URI CoolingDashboard
    VERSION 1.0
    NO_PLUGIN

    QML_FILES
        qml/Main.qml
        qml/Gauge.qml
        qml/Warning.qml
    
    RESOURCES
        gauges/speedometer.svg
        gauges/tachometer.svg
        gauges/coolant.svg
        gauges/fuel.svg
        gauges/warning.svg
)

target_include_directories(CoolingDashboard
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../handlers
)

target_link_libraries(CoolingDashboard
    PRIVATE
        can_reader
        cooling-control
        dashboard_backend

        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickLayouts
)