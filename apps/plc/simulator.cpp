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


int sockfd;

enum PlcState {
    PLC_NORMAL,
    PLC_FAULT,
    PLC_TEST
};

PlcState plc_state = PLC_NORMAL;

// ---------------------------------------------------------------------------
// CAN PGN Sender
// ---------------------------------------------------------------------------
void send_pgn(uint32_t pgn, const uint8_t *data, size_t len)
{
    struct can_frame frame {};
    uint32_t arb_id = (0x18 << 24) | (pgn << 8) | 0x80;
    frame.can_id  = arb_id | CAN_EFF_FLAG;
    frame.can_dlc = len;
    std::memcpy(frame.data, data, len);
    write(sockfd, &frame, sizeof(frame));
}

void send_fault_pgn(uint8_t fault_code)
{
    uint8_t bytes[8] = { fault_code,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    send_pgn(0xEF00, bytes, 8);
}

// ---------------------------------------------------------------------------
// Telemetry Sender
// ---------------------------------------------------------------------------
void send_telemetry(int speed, int rpm, int fuel, int temp, int warn)
{
    uint8_t spd[8] = { uint8_t(speed), uint8_t(speed >> 8), 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    send_pgn(0xFEF2, spd, 8);

    uint8_t rpm_bytes[8] = { uint8_t(rpm), uint8_t(rpm >> 8), 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    send_pgn(0xF004, rpm_bytes, 8);

    uint8_t fuel_bytes[8] = { uint8_t(fuel), 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    send_pgn(0xFEFC, fuel_bytes, 8);

    uint8_t temp_bytes[8] = { uint8_t(temp), 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    send_pgn(0xFEEE, temp_bytes, 8);

    uint8_t lamp_bytes[8] = { static_cast<uint8_t>(warn ? 0x10 : 0x00), 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    send_pgn(0xFECA, lamp_bytes, 8);
}

// ---------------------------------------------------------------------------
// Read Linux Manager Command
// ---------------------------------------------------------------------------
bool read_linux_command(uint8_t &pump_cmd, uint8_t &fan_cmd)
{
    struct can_frame rx {};
    int nbytes = read(sockfd, &rx, sizeof(rx));

    if (nbytes > 0) {

        // Extract PGN correctly
        uint32_t id = rx.can_id & 0x1FFFFFFF;
        uint32_t pgn = (id >> 8) & 0xFFFF;   // PGN = PF + PS

        if (pgn == 0xFF50) {                // Linux command PGN
            pump_cmd = rx.data[0];
            fan_cmd  = rx.data[1];

            std::cout << "PLC: Linux command pump=" << int(pump_cmd)
                      << " fan=" << int(fan_cmd) << "\n";

            return true;
        }
    }
    return false;
}


// ---------------------------------------------------------------------------
// Safety Logic (temp > 120 → warning ON)
// ---------------------------------------------------------------------------
bool apply_safety_logic(int temp)
{
    if (temp > 120) {
        uint8_t lamp_bytes[8] = { 0x10, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
        send_pgn(0xFECA, lamp_bytes, 8);

        send_fault_pgn(0x02);
        std::cout << "PLC SAFETY: Over-temp (>120). Warning ON.\n";

        if (plc_state == PLC_NORMAL)
            plc_state = PLC_FAULT;

        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Apply State Logic
// ---------------------------------------------------------------------------
void apply_state_logic(bool safety_overtemp,
                       bool received_cmd,
                       uint8_t pump_cmd,
                       uint8_t fan_cmd)
{
    uint8_t pump_out = 0;
    uint8_t fan_out  = 0;

    switch (plc_state) {

        case PLC_NORMAL:
            if (safety_overtemp) {
                pump_out = 100;
                fan_out  = 100;
            } else if (received_cmd) {
                pump_out = pump_cmd;
                fan_out  = fan_cmd;
            }
            break;

        case PLC_FAULT:
            send_fault_pgn(0x02);
            pump_out = 100;
            fan_out  = 100;
            break;

        case PLC_TEST:
            send_fault_pgn(0x02);
            if (received_cmd) {
                pump_out = pump_cmd;
                fan_out  = fan_cmd;
            }
            break;
    }

    std::cout << "PLC OUTPUT: pump=" << int(pump_out)
              << " fan=" << int(fan_out) << "\n";
}

/**
 * 
 * NORMAL  --(temp > 120)-->  FAULT
 * NORMAL  --(test trigger)--> TEST
 * FAULT   --(temp <= 120)--> NORMAL
 * TEST    --(test timeout or manual exit)--> NORMAL
 * 
 */
void update_state_machine(int temp,
                          std::chrono::steady_clock::time_point &last_test)
{
    auto now = std::chrono::steady_clock::now();

    switch (plc_state) {

        case PLC_NORMAL:
            // Allow entering TEST mode only from NORMAL
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_test).count() >= 10)
            {
                std::cout << "PLC: Entering TEST mode...\n";
                send_fault_pgn(0x02);
                plc_state = PLC_TEST;
                last_test = now;
            }
            break;

        case PLC_FAULT:
            // FAULT can only return to NORMAL when temp is safe
            if (temp <= 120) {
                std::cout << "PLC: Fault cleared, returning to NORMAL.\n";
                plc_state = PLC_NORMAL;
            }
            break;

        case PLC_TEST:
            // TEST ends after 5 seconds
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_test).count() >= 5)
            {
                std::cout << "PLC: Exiting TEST mode, returning to NORMAL.\n";
                plc_state = PLC_NORMAL;
            }
            break;
    }
}


// ---------------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------------
int main()
{
    sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    struct ifreq ifr {};
    std::strcpy(ifr.ifr_name, "vcan0");
    ioctl(sockfd, SIOCGIFINDEX, &ifr);

    struct sockaddr_can addr {};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    auto last_fault = std::chrono::steady_clock::now();

    while (true)
    {
        double t = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        int speed = int((std::sin(t) + 1) * 100);
        int rpm   = int((std::sin(t+1) + 1) * 2000);
        int fuel  = int((std::sin(t+2) + 1) * 50);
        int temp  = int((std::sin(t+3) + 1) * 150);   // allow >120

        int warn = temp > 120;

        send_telemetry(speed, rpm, fuel, temp, warn);

        bool safety_overtemp = apply_safety_logic(temp);

        uint8_t pump_cmd = 0;
        uint8_t fan_cmd  = 0;
        bool received_cmd = read_linux_command(pump_cmd, fan_cmd);

        apply_state_logic(safety_overtemp, received_cmd, pump_cmd, fan_cmd);

        update_state_machine(temp, last_fault);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    close(sockfd);
    return 0;
}
