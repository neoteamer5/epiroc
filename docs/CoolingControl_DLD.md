# Cooling Control System — Detailed Low-Level Design (DLD)

## 1. Purpose

This document defines the detailed low-level design for the cooling control software composed of:

- `CoolingController`
- `StateMachine`
- `PIDController`
- `CoolingThresholds`
- `LoadCalibration()`

The design separates operating-mode logic from continuous PID control so that state transitions, calibration, and actuator behavior remain clear and testable.

---

## 2. Design Goals

The implementation shall:

- Separate cooling state transitions from PID control.
- Load cooling thresholds through `LoadCalibration()`.
- Require explicit initialization through `Init()`.
- Prevent normal machine operation when initialization fails or is forgotten.
- Support temperature hysteresis to prevent rapid state toggling.
- Provide fail-safe behavior for invalid sensors or unhealthy cooling hardware.
- Keep calibration values outside the state-machine implementation.
- Keep public API documentation in header files using `///` Doxygen comments.
- Use PascalCase for class members, camelCase for local variables and parameters, and uppercase names for compile-time constants.

---

## 3. High-Level Architecture

```text
                  +----------------------+
                  |   LoadCalibration()  |
                  +----------+-----------+
                             |
                             v
                  +----------------------+
                  |  CoolingThresholds   |
                  +----------+-----------+
                             |
                             v
+--------------------------------------------------+
|                CoolingController                 |
|                                                  |
|  Init()                                          |
|    |                                             |
|    +------> StateMachine::Init()                 |
|                                                  |
|  Update()                                        |
|    |                                             |
|    +------> StateMachine::Update()               |
|    |                                             |
|    +------> PIDController::Update()              |
|                                                  |
|    +------> FanSpeed                             |
+--------------------------------------------------+
```

The `CoolingController` is the top-level coordinator and is responsible for blocking operation when initialization has not completed successfully.

---

## 4. Data Types

### 4.1 CoolingState

```cpp
enum class CoolingState
{
    Off,
    Standby,
    Cooling,
    HighCooling,
    PostRun,
    Fault
};
```

### State Meaning

| State | Description |
|---|---|
| `Off` | Cooling system disabled. |
| `Standby` | System enabled and monitoring temperature. |
| `Cooling` | Normal PID-controlled cooling is active. |
| `HighCooling` | Maximum cooling output is commanded. |
| `PostRun` | Cooling continues after shutdown until safe temperature is reached. |
| `Fault` | System fault or invalid operating condition detected. |

---

## 5. Calibration

### 5.1 CoolingThresholds

```cpp
struct CoolingThresholds
{
    double CoolOn = 70.0;
    double CoolOff = 65.0;
    double HighCoolOn = 85.0;
    double HighCoolOff = 78.0;
};
```

The threshold values define state-transition boundaries.

Hysteresis is created by using separate ON and OFF thresholds.

Example:

```text
CoolOff < CoolOn
HighCoolOff < HighCoolOn
```

This prevents repeated transitions when measured temperature is near a threshold.

### 5.2 LoadCalibration()

```cpp
CoolingThresholds LoadCalibration();
```

`LoadCalibration()` provides cooling calibration values to the control system.

In a production target, calibration may come from:

- NVM
- EEPROM
- configuration file
- product calibration storage
- CAN/J1939 configuration
- compiled default values

The state machine shall not directly own the calibration source.

---

## 6. StateMachine Class

### 6.1 Responsibility

`StateMachine` determines the cooling operating mode.

It shall not calculate actuator PWM, fan percentage, or PID output.

Its responsibility is limited to determining:

```text
What operating state should the system be in?
```

### 6.2 Class Interface

```cpp
class StateMachine
{
public:
    bool Init(const CoolingThresholds& thresholds);

    CoolingState Update(double temperature,
                        bool enable,
                        bool shutdownRequested,
                        bool sensorValid,
                        bool pumpHealthy);

    CoolingState GetState() const;

private:
    CoolingThresholds Thresholds{};
    CoolingState State = CoolingState::Off;
    bool Initialized = false;
};
```

### 6.3 Init()

`Init()` validates and stores the calibration values.

Validation includes:

```cpp
Thresholds.CoolOff < Thresholds.CoolOn
Thresholds.HighCoolOff < Thresholds.HighCoolOn
Thresholds.CoolOn < Thresholds.HighCoolOn
```

On successful initialization:

```text
State       = Off
Initialized = true
```

On failure:

```text
Initialized = false
```

and the caller shall prevent normal operation.

### 6.4 Initialization Protection

`StateMachine::Update()` shall defensively verify initialization.

If `Init()` has not succeeded:

```cpp
if (!Initialized)
{
    State = CoolingState::Fault;
    return State;
}
```

This protects the object if it is used directly outside `CoolingController`.

---

## 7. State Transition Design

### 7.1 State Diagram

```text
                         enable
          +--------------------------------+
          |                                v
        [Off] ------------------------> [Standby]
                                          |
                                          | temperature >= CoolOn
                                          v
                                      [Cooling]
                                          |
                                          | temperature >= HighCoolOn
                                          v
                                    [HighCooling]
                                          |
                                          | temperature <= HighCoolOff
                                          v
                                      [Cooling]

[Cooling] ---- temperature <= CoolOff ----> [Standby]

[Cooling]
    |
    | shutdownRequested
    v
[PostRun]

[HighCooling]
    |
    | shutdownRequested
    v
[PostRun]

[PostRun]
    |
    | temperature <= CoolOff
    v
  [Off]

Any normal state
    |
    | !sensorValid || !pumpHealthy
    v
  [Fault]

[Fault]
    |
    | sensorValid && pumpHealthy && !enable
    v
   [Off]
```

---

## 8. PIDController Class

### 8.1 Responsibility

`PIDController` calculates continuous cooling output.

It shall not know about:

- cooling states
- shutdown logic
- sensor fault policy
- calibration source
- fan state transitions

Its responsibility is:

```text
setpoint + measured temperature + dt
                    |
                    v
                  PID
                    |
                    v
               control output
```

### 8.2 Interface

```cpp
class PIDController
{
public:
    PIDController(double kp,
                  double ki,
                  double kd,
                  double minOutput,
                  double maxOutput);

    double Update(double setpoint,
                  double measured,
                  double dt);

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
```

### 8.3 Cooling Error Direction

Cooling control uses:

```cpp
error = measured - setpoint;
```

Therefore:

```text
measured > setpoint
        |
        v
positive error
        |
        v
higher cooling command
```

### 8.4 Output Limiting

The PID output is limited to the configured actuator range.

Example:

```text
0% <= FanSpeed <= 100%
```

### 8.5 Reset Behavior

The PID shall be reset when changing operating modes to avoid carrying stale accumulated state across incompatible control modes.

Typical reset conditions include:

- entering `Cooling`
- leaving `Cooling`
- entering `HighCooling`
- entering `PostRun`
- entering `Fault`
- returning to `Off`

---

## 9. CoolingController Class

### 9.1 Responsibility

`CoolingController` is the top-level cooling control object.

It coordinates:

- calibration loading
- initialization
- state-machine updates
- PID execution
- final fan command
- initialization blocking

### 9.2 Interface

```cpp
class CoolingController
{
public:
    bool Init();

    void Update(double temperature,
                bool enable,
                bool shutdownRequested,
                bool sensorValid,
                bool pumpHealthy,
                double dt);

    CoolingState GetState() const;

    double GetFanSpeed() const;

private:
    StateMachine StateMachineObj;
    PIDController Pid;

    double FanSpeed = 0.0;
    bool Initialized = false;
};
```

---

## 10. CoolingController Initialization

### 10.1 Initialization Flow

```text
CoolingController::Init()
          |
          v
  LoadCalibration()
          |
          v
 StateMachine::Init()
          |
     +----+----+
     |         |
   success   failure
     |         |
     v         v
Initialized  Initialized
  = true      = false
     |         |
     v         v
FanSpeed=0   Safe output
```

Example implementation:

```cpp
bool CoolingController::Init()
{
    CoolingThresholds thresholds = LoadCalibration();

    if (!StateMachineObj.Init(thresholds))
    {
        Initialized = false;
        FanSpeed = 0.0;
        return false;
    }

    Pid.Reset();
    FanSpeed = 0.0;
    Initialized = true;

    return true;
}
```

---

## 11. Missing Init Protection

Forgetting to call `Init()` must not allow the machine to operate normally.

The primary protection shall be located in `CoolingController::Update()`.

```cpp
if (!Initialized)
{
    FanSpeed = 0.0;
    return;
}
```

This prevents:

- state-machine execution
- PID execution
- normal actuator commands

The `StateMachine` maintains a second defensive check.

```cpp
if (!Initialized)
{
    State = CoolingState::Fault;
    return State;
}
```

This provides two levels of protection:

```text
CoolingController
    |
    +---- blocks overall control operation
    |
StateMachine
    |
    +---- protects itself against direct misuse
```

The exact safe actuator value shall be defined by the system safety requirement.

For some products:

```text
FanSpeed = 0%
```

may be safest.

For other thermal systems:

```text
FanSpeed = 100%
```

may be the required fail-safe behavior.

The software shall not assume the safe output without a product requirement.

---

## 12. CoolingController Update Flow

```text
Update()
   |
   v
Initialized?
   |
   +---- No ----> Safe output ----> Return
   |
  Yes
   |
   v
StateMachine::Update()
   |
   v
State changed?
   |
   +---- Yes ----> PID Reset()
   |
   v
Switch(State)
   |
   +---- Off ----------> FanSpeed = 0
   |
   +---- Standby ------> FanSpeed = 0
   |
   +---- Cooling ------> PID output
   |
   +---- HighCooling --> FanSpeed = 100
   |
   +---- PostRun ------> PID output
   |
   +---- Fault --------> Fail-safe output
```

---

## 13. State-to-Actuator Behavior

| State | Control Method | Fan Command |
|---|---|---|
| `Off` | Fixed | 0% |
| `Standby` | Fixed | 0% |
| `Cooling` | PID | Configured minimum to 100% |
| `HighCooling` | Fixed | 100% |
| `PostRun` | PID | Configured minimum to 100% |
| `Fault` | Safety policy | Product-defined |

---

## 14. Example Control Sequence

Assume:

```text
CoolOn       = 70 C
CoolOff      = 65 C
HighCoolOn   = 85 C
HighCoolOff  = 78 C
```

Example:

```text
Temperature     State

60 C            Standby
72 C            Cooling
78 C            Cooling
86 C            HighCooling
82 C            HighCooling
77 C            Cooling
64 C            Standby
```

This demonstrates hysteresis around both cooling thresholds.

---

## 15. Error and Fault Handling

The following conditions shall force the state machine to `Fault`:

```text
sensorValid == false
pumpHealthy == false
StateMachine used before successful Init()
```

Additional production fault conditions may include:

- fan feedback mismatch
- pump RPM mismatch
- overtemperature
- coolant level low
- CAN timeout
- invalid calibration
- sensor plausibility failure
- actuator communication failure

---

## 16. Threading and Execution Model

The current design assumes a deterministic periodic control loop.

Example:

```text
10 ms / 50 ms / 100 ms task
          |
          v
CoolingController::Update()
```

`dt` shall represent elapsed time in seconds.

Example:

```cpp
dt = 0.1;
```

for a 100 ms control cycle.

The classes are not currently designed for concurrent calls from multiple threads.

If multiple execution contexts access the controller, synchronization shall be added externally or the ownership model shall be restricted to one control task.

---

## 17. Testing Strategy

### StateMachine Tests

Test:

- initialization success
- invalid calibration rejection
- update before initialization
- Off to Standby
- Standby to Cooling
- Cooling to HighCooling
- HighCooling to Cooling
- Cooling to Standby
- shutdown to PostRun
- PostRun to Off
- sensor fault
- pump fault
- fault recovery
- threshold hysteresis

### PIDController Tests

Test:

- zero error
- positive cooling error
- output clamping
- reset behavior
- first update behavior
- invalid or zero `dt`
- proportional response
- integral accumulation
- derivative response

### CoolingController Tests

Test:

- Update before Init
- Init failure
- state-to-output mapping
- PID reset on state transition
- fail-safe output
- calibration loading
- normal cooling sequence

---

## 18. Future Improvements

Recommended future enhancements include:

- PID integral anti-windup
- configurable PID gains through calibration
- post-run timeout
- minimum fan-on time
- minimum state dwell time
- sensor filtering
- derivative filtering
- actuator feedback diagnostics
- fault-latching policy
- degraded cooling mode
- overtemperature state
- configurable fail-safe output
- calibration versioning and CRC validation

---

## 19. Naming Convention

The implementation uses:

```text
Class names       PIDController
Methods           Update()
Class members     FanSpeed
Local variables   fanSpeed
Parameters        temperature
Constants         COOL_ON
```

Class members shall begin with a capital letter and shall not use a trailing underscore.

---

## 20. Summary

The cooling control design separates responsibilities into three levels:

```text
StateMachine
    |
    +---- decides WHAT mode the system is in

PIDController
    |
    +---- decides HOW MUCH cooling is required

CoolingController
    |
    +---- coordinates both and controls machine operation
```

Explicit `Init()` is required before normal operation.

`CoolingController` provides the primary initialization guard, while `StateMachine` provides a secondary defensive guard.

Calibration is isolated through `LoadCalibration()` so the control logic remains independent of the physical calibration source.
