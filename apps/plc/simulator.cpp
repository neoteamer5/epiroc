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


int sockfd;

void send_pgn(uint32_t pgn, const uint8_t *data, size_t len)
{
    struct can_frame frame {};

    // Build J1939-style 29-bit ID:
    // priority=6 (0x18), PGN in middle, SA=0x80
    uint32_t arb_id = (0x18 << 24) | (pgn << 8) | 0x80;

    frame.can_id  = arb_id | CAN_EFF_FLAG;   // extended frame
    frame.can_dlc = len;
    std::memcpy(frame.data, data, len);

    write(sockfd, &frame, sizeof(frame));
}

int main()
{
    // Open SocketCAN
    sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    struct ifreq ifr {};
    std::strcpy(ifr.ifr_name, "vcan0");
    ioctl(sockfd, SIOCGIFINDEX, &ifr);

    struct sockaddr_can addr {};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    while (true)
    {
        double t = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        int speed = int((std::sin(t) + 1) * 100);      // 0–200
        int rpm   = int((std::sin(t+1) + 1) * 2000);   // 0–4000
        int fuel  = int((std::sin(t+2) + 1) * 50);     // 0–100
        int temp  = int((std::sin(t+3) + 1) * 75);     // 0–150

        bool warn = temp > 120;

        // PGN 65266 – Speed
        uint8_t spd[8] = {
            uint8_t(speed & 0xFF),
            uint8_t((speed >> 8) & 0xFF),
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
        };
        send_pgn(0xFEF2, spd, 8);

        // PGN 65265 – RPM
        uint8_t rpm_bytes[8] = {
            uint8_t(rpm & 0xFF),
            uint8_t((rpm >> 8) & 0xFF),
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
        };
        send_pgn(0xF004, rpm_bytes, 8);

        // PGN 65257 – Fuel
        uint8_t fuel_bytes[8] = {
            uint8_t(fuel),
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
        };
        send_pgn(0xFEFC, fuel_bytes, 8);

        // PGN 65262 – Coolant Temp
        uint8_t temp_bytes[8] = {
            uint8_t(temp),
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
        };
        send_pgn(0xFEEE, temp_bytes, 8);

        // PGN 65226 – Warning Lamp
        uint8_t lamp = warn ? 0x10 : 0x00;
        uint8_t lamp_bytes[8] = {
            lamp,
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
        };
        send_pgn(0xFECA, lamp_bytes, 8);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    close(sockfd);
    return 0;
}
