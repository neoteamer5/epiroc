#pragma once

#include "CoolingInputs.hpp"
#include "ControlTask.hpp"
#include "TempHandler.hpp"

/// @brief Owns and coordinates Dashboard backend components.
///
/// @details
/// DashboardBackend is responsible for:
/// - initializing CAN/J1939 infrastructure,
/// - owning application message handlers,
/// - registering handlers with CanProcessor,
/// - starting backend worker threads,
/// - starting the periodic control task,
/// - joining all backend threads.
///
/// Handler objects are owned by DashboardBackend so their lifetime extends
/// beyond CanProcessor message dispatch.
class DashboardBackend
{
public:
    /// @brief Initializes the Dashboard backend.
    /// @return true if initialization succeeds.
    bool Init();

    /// @brief Starts CAN processing and periodic control execution.
    void Start();

    /// @brief Waits for backend worker threads to finish.
    void Join();

private:
    CoolingInputs Inputs;
    TempHandler Temp{Inputs};
    ControlTask Control{Inputs};

    bool Initialized = false;
};