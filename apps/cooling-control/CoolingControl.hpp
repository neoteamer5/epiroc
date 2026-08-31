#pragma once

#include <cstdint>

/// Operating states for the cooling control system.
enum class CoolingState : std::uint8_t
{
    Off,
    Standby,
    Cooling,
    HighCooling,
    PostRun,
    Fault
};

/// Calibration thresholds used by the cooling state machine.
struct CoolingThresholds
{
    double CoolOn = 70.0;
    double CoolOff = 65.0;
    double HighCoolOn = 85.0;
    double HighCoolOff = 78.0;
};

/// Loads cooling calibration values from the product calibration source.
///
/// The current implementation returns compiled defaults. A production target
/// can replace the implementation with NVM, EEPROM, configuration-file, or
/// CAN/J1939-backed calibration loading without changing the control classes.
///
/// @return Cooling threshold calibration values.
CoolingThresholds LoadCalibration();

/// Determines the current cooling operating mode.
class StateMachine
{
public:
    /// Initializes the state machine with validated cooling thresholds.
    ///
    /// @param thresholds Cooling transition thresholds.
    /// @return true when the thresholds are valid; otherwise false.
    bool Init(const CoolingThresholds& thresholds);

    /// Updates the cooling operating state.
    ///
    /// @param temperature Current measured temperature.
    /// @param enable true when cooling-system operation is enabled.
    /// @param shutdownRequested true when controlled shutdown/post-run is requested.
    /// @param sensorValid true when the temperature sensor is valid.
    /// @param pumpHealthy true when the cooling pump is healthy.
    /// @return The resulting cooling state.
    CoolingState Update(double temperature,
                        bool enable,
                        bool shutdownRequested,
                        bool sensorValid,
                        bool pumpHealthy);

    /// Returns the current cooling state.
    ///
    /// @return Current cooling state.
    CoolingState GetState() const;

private:
    CoolingThresholds Thresholds{};
    CoolingState State = CoolingState::Off;
    bool Initialized = false;
};

/// Calculates continuous PID cooling demand.
class PIDController
{
public:
    /// Constructs a PID controller.
    ///
    /// @param kp Proportional gain.
    /// @param ki Integral gain.
    /// @param kd Derivative gain.
    /// @param minOutput Minimum allowed controller output.
    /// @param maxOutput Maximum allowed controller output.
    PIDController(double kp,
                  double ki,
                  double kd,
                  double minOutput,
                  double maxOutput);

    /// Calculates a new cooling output.
    ///
    /// Cooling uses error = measured - setpoint, so temperatures above the
    /// setpoint produce a positive cooling demand.
    ///
    /// @param setpoint Desired temperature.
    /// @param measured Current measured temperature.
    /// @param dt Elapsed time in seconds.
    /// @return Clamped PID output. For invalid dt, the previous output is returned.
    double Update(double setpoint,
                  double measured,
                  double dt);

    /// Clears accumulated PID state and previous-output history.
    void Reset();

private:
    double Kp;
    double Ki;
    double Kd;
    double MinOutput;
    double MaxOutput;
    double Integral = 0.0;
    double PreviousError = 0.0;
    double LastOutput = 0.0;
    bool Initialized = false;
};

/// Top-level cooling controller coordinating calibration, state logic, and PID control.
class CoolingController
{
public:
    /// Constructs the controller with default PID tuning.
    CoolingController();

    /// Loads calibration and initializes all cooling-control components.
    ///
    /// @return true when initialization succeeds; otherwise false.
    bool Init();

    /// Executes one periodic cooling-control cycle.
    ///
    /// If Init() has not succeeded, normal state-machine and PID execution is
    /// blocked and the configured safe output is commanded.
    ///
    /// @param temperature Current measured temperature.
    /// @param enable true when cooling-system operation is enabled.
    /// @param shutdownRequested true when controlled shutdown/post-run is requested.
    /// @param sensorValid true when the temperature sensor is valid.
    /// @param pumpHealthy true when the cooling pump is healthy.
    /// @param dt Elapsed control-loop time in seconds.
    void Update(double temperature,
                bool enable,
                bool shutdownRequested,
                bool sensorValid,
                bool pumpHealthy,
                double dt);

    /// Returns the current operating state.
    ///
    /// @return Current cooling state.
    CoolingState GetState() const;

    /// Returns the current fan-speed command.
    ///
    /// @return Fan command in percent from 0.0 to 100.0.
    double GetFanSpeed() const;
    static constexpr double MIN_PID_OUTPUT = 20.0;
    static constexpr double MAX_FAN_SPEED = 100.0;
    static constexpr double SAFE_FAN_SPEED = 20.0;
    static constexpr double OFF_FAN_SPEED = 0.0;
    static constexpr double FAULT_FAN_SPEED = 100.0;
private:
    static constexpr double PID_KP = 2.0;
    static constexpr double PID_KI = 0.25;
    static constexpr double PID_KD = 0.10;


    StateMachine StateMachineObj;
    PIDController Pid;
    CoolingThresholds Thresholds{};
    double FanSpeed = OFF_FAN_SPEED;
    bool Initialized = false;
};
