#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <cmath>
#include <chrono>
#ifdef HAVE_J1939
#include <linux/can/j1939.h>
#endif

#include <iostream>


#include "can_reader.hpp"

CANData data;
int sockfd = -1;

void print_fixed()
{
    // Move cursor to top-left and clear screen
    std::cout << "\033[H\033[J";

    std::cout << "=== CAN Monitor ===\n\n";
    std::cout << "Speed : " << data.spd  << " km/h\n";
    std::cout << "RPM   : " << data.rpm  << "\n";
    std::cout << "Fuel  : " << data.fuel << " %\n";
    std::cout << "Temp  : " << data.temp << " C\n";
    std::cout << "Warn  : " << (data.warn ? "YES" : "NO") << "\n";

    std::cout.flush();
}

void demo_loop()
{
    double t = 0.0;

    while (true) {
        data.spd  = int((std::sin(t) + 1) * 100);
        data.rpm  = int((std::sin(t+1) + 1) * 2000);
        data.fuel = int((std::sin(t+2) + 1) * 50);
        data.temp = int((std::sin(t+3) + 1) * 75);
        data.warn = (std::sin(t+4) > 0.7);

        print_fixed();

        t += 0.05;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void init_socketcan()
{
#ifdef HAVE_J1939
    sockfd = socket(PF_J1939, SOCK_DGRAM, CAN_J1939);
#else
    sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
#endif

    struct ifreq ifr {};
    std::strcpy(ifr.ifr_name, "vcan0");
    ioctl(sockfd, SIOCGIFINDEX, &ifr);

    struct sockaddr_can addr {};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
}

void read_loop()
{
    struct can_frame frame {};

    /**
     * I have already known J1939 is not support from my Linux kernal,
     * and I don't have time to prepare a yacto build to be the dev environment
     * so the code here is raw SocketCAN format.
     */
    while (true) {
        int nbytes = read(sockfd, &frame, sizeof(frame));
        if (nbytes < 0) continue;

        uint32_t pgn = (frame.can_id >> 8) & 0xFFFF;

        if (pgn == 0xFEF2) {
            data.spd = frame.data[0] | (frame.data[1] << 8);
        }
        else if (pgn == 0xF004) {
            data.rpm = frame.data[0] | (frame.data[1] << 8);
        }
        else if (pgn == 0xFEFC) {
            data.fuel = frame.data[0];
        }
        else if (pgn == 0xFEEE) {
            data.temp = frame.data[0];
        }
        else if (pgn == 0xFECA) {
            data.warn = (frame.data[0] & 0x10) != 0;
        }

        print_fixed();
    }
}

