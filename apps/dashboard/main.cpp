#include <iostream>

#include "CanMessage.hpp"
#include "CommCan.hpp"
#include "CanProcessor.hpp"
#include "CanReader.hpp"
#include "CanWriter.hpp"



// ============================================================================
//                               HANDLER FUNCTIONS
// ============================================================================

/// @brief Handle SPEED PGN (FEF2).
/// @details Extracts 16‑bit vehicle speed from bytes [0..1].
/// @param msg Incoming CAN message.
void handle_speed(const CanMessage& msg)
{
    const auto* data = msg.data;
    int speed = data[0] | (data[1] << 8);
    std::cout << "Manager: SPEED=" << speed << "\n";
}

/// @brief Handle RPM PGN (F004).
/// @details Extracts 16‑bit engine RPM from bytes [0..1].
/// @param msg Incoming CAN message.
void handle_rpm(const CanMessage& msg)
{
    const auto* data = msg.data;
    int rpm = data[0] | (data[1] << 8);
    std::cout << "Manager: RPM=" << rpm << "\n";
}

/// @brief Handle FUEL PGN (FEFC).
/// @details Extracts 16‑bit fuel level from bytes [0..1].
/// @param msg Incoming CAN message.
void handle_fuel(const CanMessage& msg)
{
    const auto* data = msg.data;
    int fuel = data[0] | (data[1] << 8);
    std::cout << "Manager: FUEL=" << fuel << "\n";
}

/// @brief Handle TEMPERATURE PGN (FEEE).
/// @details Extracts coolant temperature from byte [0].
///          Demonstrates simple control logic for pump/fan speed.
/// @param msg Incoming CAN message.
void handle_temp(const CanMessage& msg)
{
    const auto* data = msg.data;
    int temp = data[0];

    std::cout << "Manager: TEMP=" << temp << "\n";

    CanCommand cmd{};
    cmd.pump = (temp > 80 ? 70 : 40);
    cmd.fan  = (temp > 80 ? 70 : 40);
}

/// @brief Handle LAMP PGN (FECA).
/// @details Simple indicator message.
/// @param msg Incoming CAN message (unused).
void handle_lamp(const CanMessage& /*msg*/)
{
    std::cout << "Manager: FAULT PGN FECA : lamp\n";
}

/// @brief Handle FAULT PGN (EF00).
/// @details Emergency fault message. Generates maximum cooling command.
/// @param msg Incoming CAN message (unused).
void handle_fault(const CanMessage& /*msg*/)
{
    std::cout << "Manager: FAULT PGN EF00 : emergency cooling\n";

    CanCommand cmd{};
    cmd.pgn  = CanMessage::PgnType::Fault;
    cmd.pump = 100;
    cmd.fan  = 100;

    CanProcessor::Instance().PushCommand(cmd);
}

/// @brief Handle unknown PGNs.
/// @details Called when no handler is registered for the PGN.
/// @param msg Incoming CAN message.
void handle_unknown(const CanMessage& msg)
{
    std::cout << "Manager: UNKNOWN PGN 0x"
              << std::hex << msg.pgn
              << std::dec << "\n";
}



// ============================================================================
//                                      MAIN
// ============================================================================

/// @brief Application entry point.
/// @details
/// Initializes the CAN communication stack:
///   - CommCan: hardware CAN interface
///   - CanReader: receives raw CAN frames → pushes CanMessage
///   - CanWriter: pops CanCommand → sends CAN frames
///   - CanProcessor: central message dispatcher + handler engine
///
/// Registers handlers for each PGN and starts all worker threads.
///
/// @return Exit status code.
int main()
{
    CommCan::Instance().Init();

    CanReader::Instance().Init(&CommCan::Instance());
    CanWriter::Instance().Init(&CommCan::Instance());

    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Speed, handle_speed);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Rpm,   handle_rpm);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Temp,  handle_temp);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Fuel,  handle_fuel);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Fault, handle_fault);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Lamp,  handle_lamp);

    CanReader::Instance().Connect(&CanProcessor::Instance());
    CanWriter::Instance().Connect(&CanProcessor::Instance());

    CanProcessor::Instance().Start();
    CanWriter::Instance().Start();
    CanReader::Instance().Start();

    CanWriter::Instance().Join();
    CanReader::Instance().Join();
    CanProcessor::Instance().Join();

    return 0;
}
