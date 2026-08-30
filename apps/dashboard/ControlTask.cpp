#include "ControlTask.hpp"

#include "CanCommand.hpp"
#include "CanProcessor.hpp"

#include <chrono>
#include <cstdint>

namespace
{
constexpr auto CONTROL_PERIOD =
    std::chrono::milliseconds(100);

constexpr double CONTROL_DT_SECONDS = 0.1;
}

ControlTask::ControlTask(CoolingInputs& inputs)
    : Inputs(inputs)
{
}

bool ControlTask::Init()
{
    if (!Cooling.Init())
    {
        Initialized = false;
        return false;
    }

    Initialized = true;

    return true;
}

void ControlTask::Start()
{
    if (!Initialized || Running)
    {
        return;
    }

    Running = true;

    Thread = std::thread(
        &ControlTask::Run,
        this);
}

void ControlTask::Stop()
{
    Running = false;
}

void ControlTask::Join()
{
    if (Thread.joinable())
    {
        Thread.join();
    }
}

void ControlTask::Run()
{
    while (Running)
    {
        const auto start =
            std::chrono::steady_clock::now();

        const CoolingInputSnapshot input =
            Inputs.GetSnapshot();

        Cooling.Update(
            input.Temperature,
            input.CoolingEnabled,
            input.ShutdownRequested,
            input.SensorValid,
            input.PumpHealthy,
            CONTROL_DT_SECONDS);

        CanCommand cmd{};

        cmd.fan =
            static_cast<std::uint8_t>(
                Cooling.GetFanSpeed());

        CanProcessor::Instance().PushCommand(cmd);

        std::this_thread::sleep_until(
            start + CONTROL_PERIOD);
    }
}