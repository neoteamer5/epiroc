# Dashboard Backend — High-Level Design (HLD)

## 1. Purpose

This document defines the high-level software architecture for the Dashboard backend.

The backend is responsible for:

- receiving CAN/J1939 messages,
- dispatching messages to PGN-specific handlers,
- maintaining application input state,
- executing periodic control tasks,
- invoking reusable control algorithms,
- generating actuator commands, and
- transmitting commands through the CAN/J1939 communication layer.

The design separates generic CAN infrastructure from application-specific message handling and reusable cooling-control logic.

---

## 2. Design Goals

The Dashboard backend shall:

- keep generic CAN/J1939 infrastructure independent of Dashboard application logic,
- keep message decoding separate from periodic control execution,
- keep reusable cooling algorithms independent of CAN transport and application scheduling,
- support object-based message handlers through a common `MsgHandler` interface,
- avoid requiring individual handler definitions in `main.cpp`,
- maintain explicit ownership and lifetime rules for registered handlers,
- execute cooling control from a deterministic periodic task,
- avoid coupling PID execution frequency to CAN message arrival frequency,
- provide a clear path for Linux and QNX execution environments,
- keep dependencies flowing from application code toward reusable lower-level modules.

---

## 3. High-Level Module Architecture

```text
+---------------------------------------------------------+
|                    Dashboard Application                |
|                                                         |
|  +------------------+       +------------------------+   |
|  |   Msg Handlers   |       |      ControlTask       |   |
|  |                  |       |                        |   |
|  | TempHandler      |------>| periodic execution     |   |
|  | SpeedHandler     |       | 10/50/100 ms           |   |
|  | RpmHandler       |       +-----------+------------+   |
|  | FuelHandler      |                   |                |
|  | FaultHandler     |                   v                |
|  | LampHandler      |       +------------------------+   |
|  +--------+---------+       |   CoolingController    |   |
|           ^                 +-----------+------------+   |
+-----------|-----------------------------|----------------+
            |                             |
            |                             v
+-----------+---------------------------------------------+
|                         Core                            |
|                                                         |
|  CanReader -> CanProcessor -> CanWriter                 |
|                   |                                     |
|                   +---- MsgHandler interface            |
|                                                         |
|  CanMessage / CanCommand / CommCan / PriorityQueue      |
+---------------------------------------------------------+

                    depends on

+---------------------------------------------------------+
|                  cooling-control                        |
|                                                         |
|  CoolingController                                      |
|      |                                                  |
|      +--> StateMachine                                  |
|      +--> PIDController                                 |
|      +--> CoolingThresholds / LoadCalibration()         |
+---------------------------------------------------------+
```

---

## 4. Module Responsibilities

### 4.1 `core`

Location:

```text
core/
```

The `core` module contains generic CAN/J1939 communication and message-processing infrastructure.

Typical classes include:

```text
CanMessage
CanCommand
CommCan
CanReader
CanWriter
CanProcessor
MsgHandler
PriorityQueue
```

Responsibilities:

- receive messages from CAN/J1939 transport,
- queue incoming messages,
- dispatch messages according to PGN,
- queue outgoing commands,
- send commands through the communication transport,
- define generic message-handler interfaces.

The `core` module shall not contain:

- `TempHandler`,
- Dashboard-specific handlers,
- `ControlTask`,
- `CoolingController`,
- Dashboard-specific control policies.

This prevents generic infrastructure from depending on application code.

---

### 4.2 `apps/cooling-control`

Location:

```text
apps/cooling-control/
```

This module contains reusable cooling-control algorithms.

Typical classes include:

```text
CoolingController
StateMachine
PIDController
CoolingThresholds
LoadCalibration()
```

Responsibilities:

- determine cooling operating state,
- execute PID control,
- apply cooling thresholds and hysteresis,
- produce cooling actuator demand,
- perform initialization and fail-safe protection.

This module shall not know about:

- CAN PGNs,
- `CanProcessor`,
- `CanReader`,
- `CanWriter`,
- `MsgHandler`,
- Dashboard scheduling threads.

The cooling-control module is an algorithm library and shall remain transport-independent.

---

### 4.3 `apps/dashboard`

Location:

```text
apps/dashboard/
```

The Dashboard module contains application-specific integration logic.

Recommended structure:

```text
apps/dashboard/
├── CMakeLists.txt
├── main.cpp
├── ControlTask.hpp
├── ControlTask.cpp
├── handlers/
│   ├── SpeedHandler.hpp
│   ├── SpeedHandler.cpp
│   ├── RpmHandler.hpp
│   ├── RpmHandler.cpp
│   ├── TempHandler.hpp
│   ├── TempHandler.cpp
│   ├── FuelHandler.hpp
│   ├── FuelHandler.cpp
│   ├── FaultHandler.hpp
│   ├── FaultHandler.cpp
│   ├── LampHandler.hpp
│   └── LampHandler.cpp
├── api/
└── qt-app/
```

Responsibilities:

- instantiate and register Dashboard-specific handlers,
- decode Dashboard-relevant PGNs,
- maintain current application input state,
- execute application control tasks,
- connect the reusable cooling-control library to CAN/J1939 inputs and outputs,
- produce `CanCommand` objects for transmission.

---

## 5. Message Handler Architecture

### 5.1 `MsgHandler`

`MsgHandler` belongs in `core` because it is a generic message-processing interface.

Example interface:

```cpp
class MsgHandler
{
public:
    virtual ~MsgHandler() = default;

    virtual bool Init()
    {
        return true;
    }

    virtual void handle(const CanMessage& msg) = 0;
};
```

The interface represents:

```text
CanMessage
    |
    v
MsgHandler::handle()
```

A concrete handler is responsible for decoding and storing application data associated with a particular PGN.

---

### 5.2 Concrete Handlers

Concrete handlers belong in:

```text
apps/dashboard/handlers/
```

Examples:

```text
SpeedHandler
RpmHandler
TempHandler
FuelHandler
FaultHandler
LampHandler
```

Example:

```cpp
class TempHandler : public MsgHandler
{
public:
    bool Init() override;
    void handle(const CanMessage& msg) override;
};
```

`TempHandler::handle()` shall primarily decode and update the latest temperature input.

Example:

```cpp
void TempHandler::handle(const CanMessage& msg)
{
    Temperature = static_cast<double>(msg.data[0]);
    SensorValid = true;
}
```

The handler shall not use CAN message arrival as the timing source for PID execution.

---

## 6. `CanProcessor` Handler Registration

`CanProcessor` continues to support function-based handlers:

```cpp
using Handler = std::function<void(const CanMessage&)>;

void RegisterHandler(
    CanMessage::PgnType pgn,
    Handler hdlFnc);
```

An object-based overload may also be provided:

```cpp
void RegisterHandler(
    CanMessage::PgnType pgn,
    MsgHandler& msgHandler);
```

Example implementation:

```cpp
void CanProcessor::RegisterHandler(
    CanMessage::PgnType pgn,
    MsgHandler& msgHandler)
{
    RegisterHandler(
        pgn,
        [&msgHandler](const CanMessage& msg)
        {
            msgHandler.handle(msg);
        });
}
```

This overload is non-owning.

Therefore:

```text
caller owns MsgHandler
        |
        v
CanProcessor stores callable reference
```

The handler owner must guarantee that the handler remains alive for the entire period during which `CanProcessor` may dispatch messages.

For stronger ownership semantics, the architecture may later migrate to:

```cpp
std::unique_ptr<MsgHandler>
```

owned by an application-side registry or handler container.

---

## 7. Handler Ownership

Handler classes shall not normally be implemented as singletons.

Singleton handlers would introduce:

- global mutable state,
- tighter coupling,
- more difficult unit testing,
- hidden dependencies.

Preferred ownership is explicit application ownership.

Example conceptual ownership:

```text
DashboardBackend
    |
    +--> TempHandler
    +--> SpeedHandler
    +--> RpmHandler
    +--> FuelHandler
    +--> FaultHandler
    +--> LampHandler
    |
    +--> ControlTask
```

The application-level owner guarantees handler lifetime while `CanProcessor` is running.

---

## 8. Periodic Control Execution

### 8.1 Why Control Is Not Executed in `handle()`

CAN message handlers are event-driven.

```text
CAN message arrives
       |
       v
MsgHandler::handle()
```

The timing of `handle()` depends on:

- PGN transmission rate,
- CAN bus scheduling,
- network delay,
- message loss,
- simulator timing,
- bus loading.

PID control requires a predictable `dt`.

Therefore:

```cpp
Cooling.Update(...)
```

shall not normally be called directly from:

```cpp
TempHandler::handle(...)
```

because that would make PID execution frequency dependent on temperature-message arrival frequency.

---

## 9. `ControlTask`

`ControlTask` belongs to:

```text
apps/dashboard/
```

It does not belong to `core` because it implements Dashboard application behavior.

It does not belong to `cooling-control` because the reusable algorithm library shall not own operating-system scheduling or application execution threads.

Responsibilities:

- execute at a configured periodic interval,
- read the most recent cooling inputs,
- invoke `CoolingController::Update()`,
- create the resulting `CanCommand`,
- submit the command for transmission.

Example conceptual interface:

```cpp
class ControlTask
{
public:
    bool Init();

    void Start();
    void Join();

private:
    void Run();
};
```

Example execution:

```cpp
void ControlTask::Run()
{
    constexpr double DT = 0.1;

    while (Running)
    {
        Cooling.Update(
            Temperature,
            CoolingEnabled,
            ShutdownRequested,
            SensorValid,
            PumpHealthy,
            DT);

        CanCommand cmd{};
        cmd.fan =
            static_cast<std::uint8_t>(
                Cooling.GetFanSpeed());

        CanProcessor::Instance().PushCommand(cmd);

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100));
    }
}
```

The exact implementation may use a more deterministic platform-specific timer mechanism.

---

## 10. Preferred Cooling Data Flow

The preferred architecture separates CAN decoding from control execution.

```text
                  CAN/J1939
                      |
                      v
                 CanReader
                      |
                      v
                 CanProcessor
                      |
                      v
                 TempHandler
                      |
                      | updates
                      v
               CoolingInputs
                      |
                      | reads latest snapshot
                      v
                 ControlTask
                 every 100 ms
                      |
                      v
              CoolingController
                  /       \
                 v         v
          StateMachine   PIDController
                 \         /
                  \       /
                   v     v
                  FanSpeed
                      |
                      v
                  CanCommand
                      |
                      v
                 CanProcessor
                      |
                      v
                  CanWriter
                      |
                      v
                   CAN/J1939
```

---

## 11. Cooling Input Model

A shared application data structure is preferred over making `ControlTask` depend directly on `TempHandler`.

Example:

```cpp
struct CoolingInputs
{
    double Temperature = 0.0;

    bool CoolingEnabled = true;
    bool ShutdownRequested = false;
    bool SensorValid = false;
    bool PumpHealthy = true;
};
```

Then:

```text
TempHandler
    |
    +--> updates CoolingInputs

FaultHandler
    |
    +--> may update PumpHealthy / fault state

ControlTask
    |
    +--> reads CoolingInputs
    |
    +--> invokes CoolingController
```

This keeps each handler focused on message decoding.

---

## 12. Recommended Ownership Model

A Dashboard-specific application object can own the integration components.

Example:

```cpp
class DashboardBackend
{
public:
    bool Init();
    void Start();
    void Join();

private:
    CoolingInputs Inputs;

    TempHandler Temp;
    SpeedHandler Speed;
    RpmHandler Rpm;
    FuelHandler Fuel;
    FaultHandler Fault;
    LampHandler Lamp;

    ControlTask Control;
};
```

Conceptually:

```text
DashboardBackend
      |
      +------ owns ------> CoolingInputs
      |
      +------ owns ------> MsgHandlers
      |
      +------ owns ------> ControlTask
```

This provides clear lifetime management and keeps `main.cpp` small.

---

## 13. `main.cpp` Responsibility

`main.cpp` shall perform top-level application composition and lifecycle management only.

It should not contain:

- PGN decoding logic,
- cooling-control algorithms,
- individual handler implementations.

Preferred form:

```cpp
int main()
{
    DashboardBackend backend;

    if (!backend.Init())
    {
        return 1;
    }

    backend.Start();
    backend.Join();

    return 0;
}
```

Alternatively, while the architecture is being migrated, `main.cpp` may continue to initialize the existing singleton infrastructure, but individual handler implementation details should remain outside `main.cpp`.

---

## 14. Threading Model

The backend contains two different execution models.

### CAN Event Processing

```text
CanReader thread
      |
      v
incoming queue
      |
      v
CanProcessor thread
      |
      v
MsgHandler::handle()
```

### Periodic Control Processing

```text
ControlTask thread
      |
      | fixed interval
      v
CoolingController::Update()
      |
      v
CanCommand
      |
      v
outgoing queue
      |
      v
CanWriter thread
```

These execution models shall remain independent.

---

## 15. Thread Safety

If `TempHandler::handle()` and `ControlTask::Run()` execute on different threads, shared cooling inputs require synchronization.

The preferred pattern is to publish/read a coherent snapshot.

Conceptually:

```text
CanProcessor thread
      |
      v
update CoolingInputs
      |
      | synchronization
      v
snapshot
      |
      v
ControlTask thread
```

Possible implementations include:

- mutex-protected input structure,
- atomics for independent scalar inputs,
- single-producer/single-consumer snapshot mechanism.

A mutex-protected snapshot is preferred initially because several cooling values together represent one logical control input set.

The control algorithm itself should remain owned and executed only by `ControlTask`.

---

## 16. Dependency Direction

Dependencies shall flow downward toward reusable infrastructure and algorithms.

Allowed:

```text
dashboard
    |
    +----> core
    |
    +----> cooling-control
```

Avoid:

```text
core
    |
    +----X----> dashboard

cooling-control
    |
    +----X----> dashboard

cooling-control
    |
    +----X----> core CAN infrastructure
```

This ensures that:

- `core` remains reusable,
- `cooling-control` remains transport-independent,
- Dashboard-specific behavior remains in the Dashboard application.

---

## 17. Build Dependencies

Conceptually:

```cmake
target_link_libraries(
    dashboard_backend
    PRIVATE
        core
        cooling_control
        Threads::Threads
)
```

The `cooling_control` target shall not link to Dashboard or CAN-specific targets.

The `core` target shall not link to Dashboard handlers.

---

## 18. Startup Sequence

Recommended startup sequence:

```text
main()
  |
  v
DashboardBackend::Init()
  |
  +--> initialize CommCan
  |
  +--> initialize CanReader
  |
  +--> initialize CanWriter
  |
  +--> initialize handlers
  |
  +--> register handlers with CanProcessor
  |
  +--> initialize CoolingController / ControlTask
  |
  v
DashboardBackend::Start()
  |
  +--> CanProcessor::Start()
  |
  +--> CanWriter::Start()
  |
  +--> CanReader::Start()
  |
  +--> ControlTask::Start()
```

Initialization shall complete before worker threads begin normal processing.

---

## 19. Shutdown Sequence

Recommended shutdown order should ensure no component uses another component after its lifetime ends.

Conceptually:

```text
request shutdown
      |
      v
stop ControlTask
      |
      v
stop CAN receive/processing
      |
      v
finish pending command handling
      |
      v
Join()
      |
      v
destroy DashboardBackend
```

All registered `MsgHandler` objects shall remain alive until `CanProcessor` has completely stopped.

---

## 20. Design Summary

The Dashboard backend is separated into three principal layers:

```text
Core
 |
 +--> communication, queues, dispatching, MsgHandler interface

Dashboard
 |
 +--> PGN-specific handlers
 +--> application state
 +--> periodic ControlTask
 +--> system composition

Cooling Control
 |
 +--> StateMachine
 +--> PIDController
 +--> CoolingController
```

The principal runtime flow is:

```text
CAN/J1939
    |
    v
CanReader
    |
    v
CanProcessor
    |
    v
MsgHandler
    |
    v
latest application inputs
    |
    v
periodic ControlTask
    |
    v
CoolingController
    |
    v
CanCommand
    |
    v
CanWriter
    |
    v
CAN/J1939
```

The key architectural rule is:

> Message arrival updates inputs; the periodic control task executes control logic.

This keeps CAN event timing separate from PID timing and maintains clean ownership, testability, and module boundaries.
