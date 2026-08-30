#pragma once

#include "CoolingControl.hpp"
#include "CoolingInputs.hpp"

#include <atomic>
#include <thread>

/// @brief Executes the cooling-control algorithm periodically.
class ControlTask
{
public:
    explicit ControlTask(CoolingInputs& inputs);

    /// @brief Initializes the control task.
    /// @return true if initialization succeeds.
    bool Init();

    /// @brief Starts the periodic control thread.
    void Start();

    /// @brief Requests termination of the periodic control thread.
    void Stop();

    /// @brief Waits for the control thread to finish.
    void Join();

private:
    void Run();

    CoolingInputs& Inputs;
    CoolingController Cooling;

    std::thread Thread;
    std::atomic<bool> Running{false};

    bool Initialized = false;
};