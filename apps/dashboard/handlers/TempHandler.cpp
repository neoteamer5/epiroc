#include "TempHandler.hpp"

TempHandler::TempHandler(CoolingInputs& inputs)
    : Inputs(inputs)
{
}

bool TempHandler::Init()
{
    Inputs.SetSensorValid(false);

    return true;
}

void TempHandler::handle(const CanMessage& msg)
{
    const double temperature =
        static_cast<double>(msg.data[0]);

    Inputs.SetTemperature(temperature);
}