#include "CoolingControl.hpp"

#include <algorithm>
#include <cmath>

namespace
{
bool AreThresholdsValid(const CoolingThresholds& thresholds)
{
    return std::isfinite(thresholds.CoolOn) &&
           std::isfinite(thresholds.CoolOff) &&
           std::isfinite(thresholds.HighCoolOn) &&
           std::isfinite(thresholds.HighCoolOff) &&
           thresholds.CoolOff < thresholds.CoolOn &&
           thresholds.HighCoolOff < thresholds.HighCoolOn &&
           thresholds.CoolOn < thresholds.HighCoolOn;
}
}

CoolingThresholds LoadCalibration()
{
    return CoolingThresholds{};
}

bool StateMachine::Init(const CoolingThresholds& thresholds)
{
    Initialized = false;
    State = CoolingState::Off;

    if (!AreThresholdsValid(thresholds))
    {
        return false;
    }

    Thresholds = thresholds;
    Initialized = true;
    return true;
}

CoolingState StateMachine::Update(double temperature,
                                  bool enable,
                                  bool shutdownRequested,
                                  bool sensorValid,
                                  bool pumpHealthy)
{
    if (!Initialized)
    {
        State = CoolingState::Fault;
        return State;
    }

    if (!sensorValid || !pumpHealthy || !std::isfinite(temperature))
    {
        State = CoolingState::Fault;
        return State;
    }

    switch (State)
    {
        case CoolingState::Off:
            if (enable)
            {
                State = CoolingState::Standby;
            }
            break;

        case CoolingState::Standby:
            if (!enable)
            {
                State = CoolingState::Off;
            }
            else if (temperature >= Thresholds.CoolOn)
            {
                State = CoolingState::Cooling;
            }
            break;

        case CoolingState::Cooling:
            if (shutdownRequested)
            {
                State = CoolingState::PostRun;
            }
            else if (temperature >= Thresholds.HighCoolOn)
            {
                State = CoolingState::HighCooling;
            }
            else if (temperature <= Thresholds.CoolOff)
            {
                State = CoolingState::Standby;
            }
            break;

        case CoolingState::HighCooling:
            if (shutdownRequested)
            {
                State = CoolingState::PostRun;
            }
            else if (temperature <= Thresholds.HighCoolOff)
            {
                State = CoolingState::Cooling;
            }
            break;

        case CoolingState::PostRun:
            if (temperature <= Thresholds.CoolOff)
            {
                State = CoolingState::Off;
            }
            break;

        case CoolingState::Fault:
            if (sensorValid && pumpHealthy && !enable)
            {
                State = CoolingState::Off;
            }
            break;
    }

    return State;
}

CoolingState StateMachine::GetState() const
{
    return State;
}

PIDController::PIDController(double kp,
                             double ki,
                             double kd,
                             double minOutput,
                             double maxOutput)
    : Kp(kp),
      Ki(ki),
      Kd(kd),
      MinOutput(minOutput),
      MaxOutput(maxOutput)
{
    if (MinOutput > MaxOutput)
    {
        std::swap(MinOutput, MaxOutput);
    }
}

double PIDController::Update(double setpoint,
                             double measured,
                             double dt)
{
    if (!std::isfinite(setpoint) ||
        !std::isfinite(measured) ||
        !std::isfinite(dt) ||
        dt <= 0.0)
    {
        return LastOutput;
    }

    const double error = measured - setpoint;
    Integral += error * dt;

    double derivative = 0.0;
    if (Initialized)
    {
        derivative = (error - PreviousError) / dt;
    }

    const double output = Kp * error + Ki * Integral + Kd * derivative;

    PreviousError = error;
    LastOutput = std::clamp(output, MinOutput, MaxOutput);
    Initialized = true;

    return LastOutput;
}

void PIDController::Reset()
{
    Integral = 0.0;
    PreviousError = 0.0;
    LastOutput = 0.0;
    Initialized = false;
}

CoolingController::CoolingController()
    : Pid(PID_KP,
          PID_KI,
          PID_KD,
          MIN_PID_OUTPUT,
          MAX_FAN_SPEED)
{
}

bool CoolingController::Init()
{
    Initialized = false;
    FanSpeed = OFF_FAN_SPEED;
    Pid.Reset();

    Thresholds = LoadCalibration();

    if (!StateMachineObj.Init(Thresholds))
    {
        return false;
    }

    Initialized = true;
    return true;
}

void CoolingController::Update(double temperature,
                               bool enable,
                               bool shutdownRequested,
                               bool sensorValid,
                               bool pumpHealthy,
                               double dt)
{
    if (!Initialized)
    {
        FanSpeed = OFF_FAN_SPEED;
        return;
    }

    const CoolingState previousState = StateMachineObj.GetState();
    const CoolingState state = StateMachineObj.Update(temperature,
                                                      enable,
                                                      shutdownRequested,
                                                      sensorValid,
                                                      pumpHealthy);

    if (state != previousState)
    {
        Pid.Reset();
    }

    switch (state)
    {
        case CoolingState::Off:
        case CoolingState::Standby:
            FanSpeed = 0.0;
            break;

        case CoolingState::Cooling:
            FanSpeed = Pid.Update(Thresholds.CoolOff, temperature, dt);
            break;

        case CoolingState::HighCooling:
            FanSpeed = MAX_FAN_SPEED;
            break;

        case CoolingState::PostRun:
            FanSpeed = Pid.Update(Thresholds.CoolOff, temperature, dt);
            break;

        case CoolingState::Fault:
            FanSpeed = FAULT_FAN_SPEED;
            break;
    }
}

CoolingState CoolingController::GetState() const
{
    return StateMachineObj.GetState();
}

double CoolingController::GetFanSpeed() const
{
    return FanSpeed;
}
