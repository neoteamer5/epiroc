#pragma once

#include "CoolingControl.hpp"
#include "CoolingInputs.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>

class ControlTask
{
public:
    explicit ControlTask(CoolingInputs& inputs);

    /// @brief Initializes the periodic cooling control task.
    /// @return true if initialization succeeds.
    bool Init();

    /// @brief Starts the periodic control thread.
    void Start();

    /// @brief Stops the periodic control thread.
    void Stop();

    /// @brief Waits for the periodic control thread to finish.
    void Join();

private:
    void Run();

    CoolingInputs& Inputs;
    CoolingController Cooling;

    std::thread Thread;
    std::atomic<bool> Running{false};

    bool Initialized = false;
    bool CommandSent = false;

    uint16_t LastFanSpeed = 0;

    std::chrono::steady_clock::time_point LastCommandTime{};
};