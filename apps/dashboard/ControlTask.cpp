#include "ControlTask.hpp"

#include "CanCommand.hpp"
#include "CanProcessor.hpp"

namespace
{
constexpr auto CONTROL_PERIOD = std::chrono::milliseconds(100);
constexpr auto COMMAND_RESEND_PERIOD = std::chrono::milliseconds(500);

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

    CommandSent = false;
    LastFanSpeed = 0;
    LastCommandTime = {};

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
    Thread = std::thread(&ControlTask::Run, this);
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
    auto nextWakeup = std::chrono::steady_clock::now();

    while (Running)
    {
        nextWakeup += CONTROL_PERIOD;

        const auto now = std::chrono::steady_clock::now();

        const CoolingInputSnapshot input = Inputs.GetSnapshot();

        Cooling.Update(
            input.Temperature,
            input.CoolingEnabled,
            input.ShutdownRequested,
            input.SensorValid,
            input.PumpHealthy,
            CONTROL_DT_SECONDS);

        const uint16_t fanSpeed = static_cast<uint16_t>(Cooling.GetFanSpeed());

        const bool fanChanged = !CommandSent || fanSpeed != LastFanSpeed;

        const bool resendRequired = CommandSent && (now - LastCommandTime >= COMMAND_RESEND_PERIOD);

        if (fanChanged || resendRequired)
        {
            CanCommand cmd{};
            cmd.fan = fanSpeed;

            if (CanProcessor::Instance().PushCommand(cmd))
            {
                LastFanSpeed = fanSpeed;
                LastCommandTime = now;
                CommandSent = true;
            }
        }

        std::this_thread::sleep_until(nextWakeup);
    }
}