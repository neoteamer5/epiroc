#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>

#include <linux/can/j1939.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

#include "CanMessage.hpp"
#include "CommCan.hpp"

namespace
{

// -----------------------------------------------------------------------------
// Integration-test configuration
// -----------------------------------------------------------------------------

/// @brief Maximum time to wait for the Dashboard to respond to one test input.
///
/// The Dashboard control task currently executes periodically, so the PLC must
/// not expect a response immediately after transmitting telemetry.
constexpr auto COMMAND_TIMEOUT = std::chrono::milliseconds(1500);

/// @brief Small delay between non-blocking socket read attempts.
constexpr useconds_t READ_RETRY_DELAY_US = 10000;

/// @brief Delay between completed integration-test steps.
///
/// This is not required for correctness, but makes test output easier to read
/// and prevents test vectors from being injected back-to-back.
constexpr useconds_t TEST_STEP_DELAY_US = 200000;

/// @brief J1939 payload size used by the current telemetry PGNs.
constexpr size_t TELEMETRY_PAYLOAD_SIZE = 8;

/// @brief Cooling command payload size.
///
/// Byte layout:
///
///   Byte 0 : pump low byte
///   Byte 1 : pump high byte
///   Byte 2 : fan low byte
///   Byte 3 : fan high byte
///
/// Both pump and fan are uint16_t values encoded little-endian.
constexpr size_t COMMAND_PAYLOAD_SIZE = 4;


// -----------------------------------------------------------------------------
// Integration-test data structures
// -----------------------------------------------------------------------------

/// @brief Sensor values transmitted by the simulated PLC.
struct SensorData
{
    int Speed;
    int Rpm;
    int Fuel;
    int Temp;
};

/// @brief One deterministic integration-test step.
///
/// Each step:
///
/// 1. Sends known PLC telemetry.
/// 2. Waits for the Dashboard to process the telemetry.
/// 3. Receives a cooling command from the Dashboard.
/// 4. Verifies the command against the expected pump/fan values.
/// 5. Reports PASS or FAIL.
///
/// ExpectedFan values must correspond to the currently configured
/// CoolingController calibration/PID behavior.
struct IntegrationTestStep
{
    SensorData Data;

    uint16_t ExpectedPump;
    uint16_t ExpectedFan;
};


/// @brief Deterministic integration-test sequence.
///
/// The temperature sequence deliberately crosses the cooling thresholds and
/// hysteresis boundaries.
///
/// Example intended state progression:
///
///   Temp 60 -> Standby
///   Temp 72 -> Cooling
///   Temp 88 -> HighCooling
///   Temp 80 -> remain HighCooling because of hysteresis
///   Temp 76 -> Cooling
///   Temp 63 -> Standby
///
/// IMPORTANT:
/// The ExpectedFan values below are examples and must match the actual
/// CoolingController PID/calibration configuration.
///
/// If ControlTask currently controls only fan and leaves pump at zero,
/// ExpectedPump should remain zero.
constexpr std::array<IntegrationTestStep, 6> TEST_SEQUENCE
{{
    // Speed   RPM   Fuel   Temp    Pump   Fan
    {{20,      800,  90,    60},      0,    0},
    {{30,     1200,  85,    72},      0,   40},
    {{40,     1600,  80,    88},      0,   70},
    {{35,     1400,  75,    80},      0,   70},
    {{25,     1000,  70,    76},      0,   40},
    {{20,      900,  65,    63},      0,    0}
}};


/// @brief CAN/J1939 socket owned by CommCan.
int Sock = -1;


// -----------------------------------------------------------------------------
// J1939 transmit helpers
// -----------------------------------------------------------------------------

/// @brief Sends one J1939 PGN.
///
/// The socket is a CAN_J1939 SOCK_DGRAM socket, therefore sendto() transmits
/// only the J1939 payload. The destination sockaddr_can identifies the PGN.
///
/// Telemetry messages are broadcast by using J1939_NO_ADDR.
void SendPgn(
    CanMessage::PgnType pgn,
    const uint8_t* data,
    size_t len)
{
    struct sockaddr_can dst{};

    dst.can_family = AF_CAN;
    dst.can_ifindex = if_nametoindex("vcan0");

    dst.can_addr.j1939.name = J1939_NO_NAME;
    dst.can_addr.j1939.addr = J1939_NO_ADDR;
    dst.can_addr.j1939.pgn = static_cast<uint32_t>(pgn);

    const auto ret = sendto(
        Sock,
        data,
        len,
        0,
        reinterpret_cast<struct sockaddr*>(&dst),
        sizeof(dst));

    if (ret < 0)
    {
        std::cout
            << "PLC ERROR: failed to send PGN 0x"
            << std::hex
            << static_cast<uint32_t>(pgn)
            << std::dec
            << "\n";
    }
}


/// @brief Sends deterministic telemetry to the Dashboard.
///
/// The telemetry encoding follows the current application protocol:
///
/// Speed:
///   byte 0 = low byte
///   byte 1 = high byte
///
/// RPM:
///   byte 0 = low byte
///   byte 1 = high byte
///
/// Fuel:
///   byte 0 = low byte
///   byte 1 = high byte
///
/// Temperature:
///   byte 0 = temperature
///
/// Lamp:
///   bit 4 indicates warning.
///
/// Remaining unused bytes are set to 0xFF.
void SendTelemetry(
    int speed,
    int rpm,
    int fuel,
    int temp,
    int warn)
{
    uint8_t speedBytes[TELEMETRY_PAYLOAD_SIZE] =
    {
        static_cast<uint8_t>(speed),
        static_cast<uint8_t>(speed >> 8),
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    SendPgn(
        CanMessage::PgnType::Speed,
        speedBytes,
        sizeof(speedBytes));


    uint8_t rpmBytes[TELEMETRY_PAYLOAD_SIZE] =
    {
        static_cast<uint8_t>(rpm),
        static_cast<uint8_t>(rpm >> 8),
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    SendPgn(
        CanMessage::PgnType::Rpm,
        rpmBytes,
        sizeof(rpmBytes));


    uint8_t fuelBytes[TELEMETRY_PAYLOAD_SIZE] =
    {
        static_cast<uint8_t>(fuel),
        static_cast<uint8_t>(fuel >> 8),
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    SendPgn(
        CanMessage::PgnType::Fuel,
        fuelBytes,
        sizeof(fuelBytes));


    uint8_t tempBytes[TELEMETRY_PAYLOAD_SIZE] =
    {
        static_cast<uint8_t>(temp),
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    SendPgn(
        CanMessage::PgnType::Temp,
        tempBytes,
        sizeof(tempBytes));


    uint8_t lampBytes[TELEMETRY_PAYLOAD_SIZE] =
    {
        static_cast<uint8_t>(warn ? 0x10 : 0x00),
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };

    SendPgn(
        CanMessage::PgnType::Lamp,
        lampBytes,
        sizeof(lampBytes));


    std::cout
        << "PLC TX: "
        << "SPD=" << speed
        << " RPM=" << rpm
        << " FUEL=" << fuel
        << " TEMP=" << temp
        << " WARN=" << warn
        << "\n";
}


// -----------------------------------------------------------------------------
// Dashboard command receive/decode
// -----------------------------------------------------------------------------

/// @brief Attempts to read one cooling command sent by the Dashboard.
///
/// The socket is CAN_J1939 SOCK_DGRAM, therefore recvfrom() returns the J1939
/// payload directly rather than a struct can_frame.
///
/// The PGN is obtained from sockaddr_can returned by recvfrom().
///
/// Expected command payload:
///
///   byte 0-1 : pump uint16_t, little-endian
///   byte 2-3 : fan  uint16_t, little-endian
///
/// @param pumpCmd Decoded pump command.
/// @param fanCmd Decoded fan command.
/// @return true when a valid Cmd PGN was received and decoded.
bool ReadLinuxCommand(
    uint16_t& pumpCmd,
    uint16_t& fanCmd)
{
    uint8_t payload[TELEMETRY_PAYLOAD_SIZE]{};

    struct sockaddr_can src{};
    socklen_t srcLen = sizeof(src);

    const int nbytes = recvfrom(
        Sock,
        payload,
        sizeof(payload),
        0,
        reinterpret_cast<struct sockaddr*>(&src),
        &srcLen);

    if (nbytes <= 0)
    {
        return false;
    }

    const auto pgn =
        static_cast<CanMessage::PgnType>(
            src.can_addr.j1939.pgn);

    if (pgn != CanMessage::PgnType::Cmd)
    {
        // The PLC may receive unrelated J1939 traffic.
        // Ignore it and continue waiting for the command PGN.
        return false;
    }

    if (nbytes < static_cast<int>(COMMAND_PAYLOAD_SIZE))
    {
        std::cout
            << "PLC ERROR: Cmd payload too short. bytes="
            << nbytes
            << "\n";

        return false;
    }

    pumpCmd =
        static_cast<uint16_t>(payload[0]) |
        (static_cast<uint16_t>(payload[1]) << 8);

    fanCmd =
        static_cast<uint16_t>(payload[2]) |
        (static_cast<uint16_t>(payload[3]) << 8);

    std::cout
        << "PLC RX: "
        << "PUMP=" << pumpCmd
        << " FAN=" << fanCmd
        << "\n";

    return true;
}


// -----------------------------------------------------------------------------
// Integration-test verification
// -----------------------------------------------------------------------------

/// @brief Verifies a received Dashboard cooling command.
///
/// @return true when both pump and fan match the expected values.
bool VerifyCommand(
    const IntegrationTestStep& step,
    uint16_t pumpCmd,
    uint16_t fanCmd)
{
    const bool pumpOk =
        pumpCmd == step.ExpectedPump;

    const bool fanOk =
        fanCmd == step.ExpectedFan;

    if (pumpOk && fanOk)
    {
        std::cout
            << "TEST PASS: "
            << "TEMP=" << step.Data.Temp
            << " PUMP=" << pumpCmd
            << " FAN=" << fanCmd
            << "\n";

        return true;
    }

    std::cout
        << "PLC RX: command does not match current test step. "
        << "Expected PUMP=" << step.ExpectedPump
        << " FAN=" << step.ExpectedFan
        << ", received PUMP=" << pumpCmd
        << " FAN=" << fanCmd
        << "\n";

    return false;
}


/// @brief Waits until the expected Dashboard command is received or timeout.
///
/// The Dashboard sends a command:
///
///   - immediately when the output changes, OR
///   - periodically as a heartbeat.
///
/// Because a heartbeat generated for the previous test step may already be
/// queued in the socket, an unexpected command does not immediately fail the
/// test.
///
/// Instead, the PLC continues reading until:
///
///   expected command received -> PASS
///   timeout                  -> FAIL
///
/// This avoids incorrectly failing a test because of an old heartbeat command.
bool WaitForExpectedCommand(
    const IntegrationTestStep& step,
    std::chrono::milliseconds timeout)
{
    const auto deadline =
        std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
        uint16_t pumpCmd = 0;
        uint16_t fanCmd = 0;

        if (ReadLinuxCommand(pumpCmd, fanCmd))
        {
            if (VerifyCommand(
                    step,
                    pumpCmd,
                    fanCmd))
            {
                return true;
            }
        }

        usleep(READ_RETRY_DELAY_US);
    }

    std::cout
        << "TEST FAIL: timeout waiting for expected command. "
        << "TEMP=" << step.Data.Temp
        << " Expected PUMP=" << step.ExpectedPump
        << " FAN=" << step.ExpectedFan
        << "\n";

    return false;
}


// -----------------------------------------------------------------------------
// Integration-test execution
// -----------------------------------------------------------------------------

/// @brief Runs the complete PLC-to-Dashboard integration test.
///
/// Test sequence for every step:
///
///        PLC                         Dashboard
///         |                              |
///         | ---- telemetry ------------>|
///         |                              |
///         |                         TempHandler
///         |                              |
///         |                        CoolingInputs
///         |                              |
///         |                         ControlTask
///         |                              |
///         |                     CoolingController
///         |                              |
///         |<---- CanCommand -------------|
///         |
///         +---- verify command
///
/// The next telemetry step is not sent until the current step passes or times
/// out. This makes the test deterministic:
///
///   SEND -> WAIT -> VERIFY -> SEND -> WAIT -> VERIFY
///
/// @return true when every integration-test step passes.
bool RunIntegrationTest()
{
    size_t passed = 0;
    size_t failed = 0;

    std::cout
        << "\n"
        << "========================================\n"
        << "PLC / Dashboard Integration Test\n"
        << "========================================\n";

    for (size_t i = 0; i < TEST_SEQUENCE.size(); ++i)
    {
        const auto& step = TEST_SEQUENCE[i];

        std::cout
            << "\n"
            << "----------------------------------------\n"
            << "TEST STEP " << i + 1
            << " / " << TEST_SEQUENCE.size()
            << "\n"
            << "Input TEMP=" << step.Data.Temp
            << "\n"
            << "Expected PUMP=" << step.ExpectedPump
            << " FAN=" << step.ExpectedFan
            << "\n"
            << "----------------------------------------\n";

        const int warn =
            step.Data.Temp > 120;

        // Step 1:
        // Inject deterministic sensor input into the Dashboard.
        SendTelemetry(
            step.Data.Speed,
            step.Data.Rpm,
            step.Data.Fuel,
            step.Data.Temp,
            warn);

        // Step 2:
        // Wait for the Dashboard control loop to process the new input and
        // return the expected cooling command.
        const bool stepPassed =
            WaitForExpectedCommand(
                step,
                COMMAND_TIMEOUT);

        if (stepPassed)
        {
            ++passed;
        }
        else
        {
            ++failed;
        }

        usleep(TEST_STEP_DELAY_US);
    }


    std::cout
        << "\n"
        << "========================================\n"
        << "Integration Test Result\n"
        << "========================================\n"
        << "Total : " << TEST_SEQUENCE.size() << "\n"
        << "Passed: " << passed << "\n"
        << "Failed: " << failed << "\n"
        << "========================================\n";

    if (failed == 0)
    {
        std::cout << "RESULT: PASS\n";
        return true;
    }

    std::cout << "RESULT: FAIL\n";
    return false;
}

} // namespace


// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
/***
 * main()
  |
  +--> Initialize CommCan
  |
  +--> Set socket non-blocking
  |
  +--> Get J1939 socket
  |
  +--> RunIntegrationTest()
  |       |
  |       +--> send test step 1
  |       +--> verify
  |       +--> send test step 2
  |       +--> verify
  |       +--> ...
  |
  +--> close socket
  |
  +--> return 0  PASS
       return 1  FAIL
 */
int main()
{
    CommCan::Instance().Init(CanMessage::SourceAddress::Simulator);
    CommCan::Instance().SetNonBlock();

    Sock = CommCan::Instance().GetSocket();

    if (Sock < 0)
    {
        std::cout << "PLC ERROR: CAN/J1939 socket is not initialized.\n";
        return 1;
    }

    const bool passed = RunIntegrationTest();

    close(Sock);

    return passed ? 0 : 1;
}