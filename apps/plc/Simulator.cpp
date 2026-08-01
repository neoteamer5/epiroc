#include <iostream>
#include <cmath>
#include <cstring>
#include <chrono>
#include <thread>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include <iomanip>

#include "CanMessage.hpp"
#include "CommCan.hpp"

enum PlcState {
    PLC_NORMAL,
    PLC_FAULT,
    PLC_TEST
};

PlcState plc_state = PLC_NORMAL;
int sock = -1;

/**
 * Print telemetry + outputs + Linux commands in fixed columns
 *
 */
void PrintDashboard(int speed,
                    int rpm,
                    int fuel,
                    int temp,
                    int warn,
                    int pump_out,
                    int fan_out,
                    int pump_cmd,
                    int fan_cmd)
{
    // Move cursor to top-left (keeps output in fixed area)
    std::cout << "\033[H";

    // Header
    std::cout << std::left
              << std::setw(10) << "SPEED"
              << std::setw(10) << "RPM"
              << std::setw(10) << "FUEL"
              << std::setw(10) << "TEMP"
              << std::setw(10) << "WARN"
              << std::setw(10) << "PUMP_OUT"
              << std::setw(10) << "FAN_OUT"
              << std::setw(10) << "CMD(P/F)"
              << "\n";

    // Values
    std::cout << std::left
              << std::setw(10) << speed
              << std::setw(10) << rpm
              << std::setw(10) << fuel
              << std::setw(10) << temp
              << std::setw(10) << warn
              << std::setw(10) << pump_out
              << std::setw(10) << fan_out
              << std::setw(10) << (std::to_string(pump_cmd) + "/" + std::to_string(fan_cmd))
              << "\n";
}

/**
 * Send a CAN PGN frame
 *
 */
void SendPgn(uint32_t pgn, const uint8_t *data, size_t len)
{
    struct can_frame frame;
    std::memset(&frame, 0, sizeof(frame));
    static int countFrame = 0;

    uint32_t arb_id = (0x18 << 24) | (pgn << 8) | 0x80;
    frame.can_id = arb_id | CAN_EFF_FLAG;
    frame.can_dlc = len;

    std::memcpy(frame.data, data, len);
    write(sock, &frame, sizeof(frame));
    std::cout << "msg count=" << ++countFrame << std::endl;
}

/**
 * Send a fault PGN with a fault code
 *
 */
void SendFaultPgn(uint8_t fault_code)
{
    uint8_t bytes[8] = { fault_code, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    SendPgn(0xEF00, bytes, 8);
}

/**
 * Send telemetry PGNs (speed, rpm, fuel, temp, warning)
 *
 */
void SendTelemetry(int speed, int rpm, int fuel, int temp, int warn)
{
    uint8_t spd[8] = { uint8_t(speed), uint8_t(speed >> 8), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    SendPgn(0xFEF2, spd, 8);

    uint8_t rpm_bytes[8] = { uint8_t(rpm), uint8_t(rpm >> 8), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    SendPgn(0xF004, rpm_bytes, 8);

    uint8_t fuel_bytes[8] = { uint8_t(fuel), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    SendPgn(0xFEFC, fuel_bytes, 8);

    uint8_t temp_bytes[8] = { uint8_t(temp), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    SendPgn(0xFEEE, temp_bytes, 8);

    uint8_t lamp_bytes[8] = { uint8_t(warn ? 0x10 : 0x00), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    SendPgn(0xFECA, lamp_bytes, 8);

    std::cout << "TELEMETRY : "
              << "SPD="  << speed << " "
              << "RPM="  << rpm   << " "
              << "FUEL=" << fuel  << " "
              << "TEMP=" << temp  << " "
              << "WARN=" << warn
              << "\n";
}

/**
 * Read Linux manager command PGN
 *
 */
bool ReadLinuxCommand(uint8_t &pump_cmd, uint8_t &fan_cmd)
{
    struct can_frame rx;
    std::memset(&rx, 0, sizeof(rx));
    
    //std::cout << "start reading sock=" << sock << "..." << std::endl;
    int nbytes = read(sock, &rx, sizeof(rx));
    if (nbytes <= 0)
        return false;

    uint32_t id = rx.can_id & 0x1FFFFFFF;
    uint32_t pgn = (id >> 8) & 0xFFFF;

    if (pgn == CanMessage::PgnType::Fault) 
    {
        pump_cmd = rx.data[0];
        fan_cmd = rx.data[1];

        std::cout << "PLC: Linux command pump=" << int(pump_cmd)
                  << " fan=" << int(fan_cmd) << "\n";

        return true;
    }
    else
    {
        std::cout << "unknown pgn=" << pgn << std::endl;
    }

    return false;
}

/**
 * Apply safety logic (temp > 120 → warning ON)
 *
 */
bool ApplySafetyLogic(int temp)
{
    if (temp > 120)
    {
        //uint8_t lamp_bytes[8] = { 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
        //SendPgn(0xFECA, lamp_bytes, 8);

        SendFaultPgn(0x02);
        std::cout << "PLC SAFETY: Over-temp (>120). Warning ON.\n";

        if (plc_state == PLC_NORMAL)
            plc_state = PLC_FAULT;

        return true;
    }

    return false;
}

/**
 * Apply PLC state logic (NORMAL, FAULT, TEST)
 *
 */
void ApplyStateLogic(bool safety_overtemp,
                     bool received_cmd,
                     uint8_t pump_cmd,
                     uint8_t fan_cmd)
{
    uint8_t pump_out = 0;
    uint8_t fan_out = 0;

    switch (plc_state)
    {
        case PLC_NORMAL:
            if (safety_overtemp)
            {
                pump_out = 100;
                fan_out = 100;
            }
            else if (received_cmd)
            {
                pump_out = pump_cmd;
                fan_out = fan_cmd;
            }
            break;

        case PLC_FAULT:
            pump_out = 100;
            fan_out = 100;
            break;

        case PLC_TEST:
            SendFaultPgn(0x02);
            if (received_cmd)
            {
                pump_out = pump_cmd;
                fan_out = fan_cmd;
            }
            break;
    }

    std::cout << "PLC OUTPUT: pump=" << int(pump_out)
              << " fan=" << int(fan_out) << "\n";
}

/**
 * PLC state machine transitions
 *
 */
void UpdateStateMachine(int temp,
                        std::chrono::steady_clock::time_point &last_test)
{
    auto now = std::chrono::steady_clock::now();

    switch (plc_state)
    {
        case PLC_NORMAL:
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_test).count() >= 10)
            {
                std::cout << "PLC: Entering TEST mode...\n";
                SendFaultPgn(0x02);
                plc_state = PLC_TEST;
                last_test = now;
            }
            break;

        case PLC_FAULT:
            if (temp <= 120)
            {
                std::cout << "PLC: Fault cleared, returning to NORMAL.\n";
                plc_state = PLC_NORMAL;
            }
            break;

        case PLC_TEST:
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_test).count() >= 5)
            {
                std::cout << "PLC: Exiting TEST mode, returning to NORMAL.\n";
                plc_state = PLC_NORMAL;
            }
            break;
    }
}

/**
 * Enable non-blocking keyboard input
 *
 */
void EnableKeyboardNonBlocking()
{
    termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~ICANON;   // disable canonical mode
    t.c_lflag &= ~ECHO;     // disable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &t);

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}



/**
 * Main loop
 *
 */
int main()
{
    CommCan::Instance().Init();
    CommCan::Instance().SetNonBlock();
    sock =  CommCan::Instance().GetSocket();

    auto last_test = std::chrono::steady_clock::now();

    std::cout << "\033[2J";  // Clear screen
    EnableKeyboardNonBlocking();
    bool paused = false;

    while (true)
    {
        // Toggle pause on any key
        char ch;
        if (read(STDIN_FILENO, &ch, 1) > 0)
        {
            paused = !paused;

            if (paused)
                std::cout << "\n--- PAUSED --- Press any key to continue ---\n";
            else
                std::cout << "\n--- RESUMED ---\n";
        }

        if (paused)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        double t = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        int speed = static_cast<int>((std::sin(t) + 1) * 100);
        int rpm =   static_cast<int>((std::sin(t) + 1) * 2000);
        int fuel =  static_cast<int>((std::sin(t) + 1) * 50);
        int temp =  static_cast<int>((std::sin(t) + 1) * 80);

        int warn = temp > 120;

        SendTelemetry(speed, rpm, fuel, temp, warn);

        bool safety_overtemp = ApplySafetyLogic(temp);

        uint8_t pump_cmd = 0;
        uint8_t fan_cmd = 0;
        bool received_cmd = ReadLinuxCommand(pump_cmd, fan_cmd);

        ApplyStateLogic(safety_overtemp, received_cmd, pump_cmd, fan_cmd);

        UpdateStateMachine(temp, last_test);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    close(sock);
    return 0;
}
