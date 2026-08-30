#pragma once
/***
 * CoolingInputs is the bridge between Dashboard CAN handlers and the Dashboard periodic control task:
 * 
 * apps/dashboard/

 TempHandler
      |
      | writes
      v
 CoolingInputs.hpp
      ^
      | reads
      |
 ControlTask
      |
      v
 CoolingController
      |
      v
apps/cooling-control/
 */

#include <mutex>

/// @brief Snapshot of inputs consumed by the cooling control task.
struct CoolingInputSnapshot
{
    double Temperature = 0.0;

    bool CoolingEnabled = true;
    bool ShutdownRequested = false;
    bool SensorValid = true;
    bool PumpHealthy = true;
};

/// @brief Thread-safe shared cooling input storage.
///
/// @details
/// CAN message handlers update this object from the CanProcessor thread.
/// ControlTask reads a coherent snapshot from its periodic control thread.
class CoolingInputs
{
public:
    /// @brief Updates the measured coolant temperature.
    void SetTemperature(double temperature)
    {
        std::lock_guard<std::mutex> lock(Mutex);

        Data.Temperature = temperature;
        Data.SensorValid = true;
    }

    /// @brief Updates cooling enable state.
    void SetCoolingEnabled(bool enabled)
    {
        std::lock_guard<std::mutex> lock(Mutex);

        Data.CoolingEnabled = enabled;
    }

    /// @brief Updates shutdown request state.
    void SetShutdownRequested(bool requested)
    {
        std::lock_guard<std::mutex> lock(Mutex);

        Data.ShutdownRequested = requested;
    }

    /// @brief Updates temperature sensor validity.
    void SetSensorValid(bool valid)
    {
        std::lock_guard<std::mutex> lock(Mutex);

        Data.SensorValid = valid;
    }

    /// @brief Updates pump health status.
    void SetPumpHealthy(bool healthy)
    {
        std::lock_guard<std::mutex> lock(Mutex);

        Data.PumpHealthy = healthy;
    }

    /// @brief Returns a coherent snapshot of all cooling inputs.
    CoolingInputSnapshot GetSnapshot() const
    {
        std::lock_guard<std::mutex> lock(Mutex);

        return Data;
    }

private:
    mutable std::mutex Mutex;

    CoolingInputSnapshot Data;
};