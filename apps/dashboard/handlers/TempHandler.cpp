#include "TempHandler.hpp"

#include "CanCommand.hpp"
#include "CanProcessor.hpp"

#include <cstdint>
#include <iostream>

bool TempHandler::Init()
{
    Temperature = 0.0;
    CoolingEnabled = true;
    ShutdownRequested = false;
    SensorValid = false;
    PumpHealthy = true;

    return Cooling.Init();
}

void TempHandler::handle(const CanMessage& msg)
{
    Temperature = static_cast<double>(msg.data[0]);
    SensorValid = true;

    std::cout << "TempHandler: TEMP="
              << Temperature
              << '\n';
}

void TempHandler::Update(double dt)
{
    Cooling.Update(
        Temperature,
        CoolingEnabled,
        ShutdownRequested,
        SensorValid,
        PumpHealthy,
        dt);

    CanCommand cmd{};

    cmd.fan = static_cast<uint8_t>(Cooling.GetFanSpeed());

    // Set pump command according to system requirement.
    // cmd.pump = ...

    CanProcessor::Instance().PushCommand(cmd);
}