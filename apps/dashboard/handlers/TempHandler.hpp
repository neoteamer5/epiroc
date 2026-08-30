#pragma once

#include "MsgHandler.hpp"
#include "CoolingControl.hpp"

/// @brief Handles the J1939 temperature PGN.
///
/// Extracts coolant temperature and passes the latest temperature
/// information to the cooling-control subsystem.
class TempHandler : public MsgHandler
{
public:
    /// @brief Initialize the temperature handler and cooling controller.
    /// @return true if initialization succeeds.
    bool Init() override;

    /// @brief Handle TEMPERATURE PGN (FEEE).
    /// @param msg Incoming CAN message.
    void handle(const CanMessage& msg) override;

    /// @brief Execute the periodic cooling control loop.
    /// @param dt Elapsed control-loop time in seconds.
    void Update(double dt);

private:
    CoolingController Cooling;

    double Temperature = 0.0;

    bool CoolingEnabled = true;
    bool ShutdownRequested = false;
    bool SensorValid = false;
    bool PumpHealthy = true;
};