#include "CANReader.hpp"
#include <chrono>
#include <iostream>
#include <net/if.h>

CANReader::CANReader(bool demo_mode)
    : demo(demo_mode)
{
    if (demo) {
        worker = std::thread(&CANReader::demo_loop, this);
    } else {
        // open socketcan
        sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

        sockaddr_can addr{};
        addr.can_family = AF_CAN;
        addr.can_ifindex = if_nametoindex("can0");

        bind(sockfd, (sockaddr*)&addr, sizeof(addr));

        worker = std::thread(&CANReader::real_loop, this);
    }
}

CANReader::~CANReader() {
    running = false;
    if (worker.joinable())
        worker.join();
    if (sockfd >= 0)
        close(sockfd);
}

void CANReader::demo_loop() {
    while (running) {
        data.spd  = (std::rand() % 120);
        data.rpm  = (std::rand() % 3500);
        data.fuel = (std::rand() % 100);
        data.temp = (std::rand() % 120);
        data.warn = (data.temp > 100);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void CANReader::real_loop() {
    struct can_frame frame;

    while (running) {
        int nbytes = read(sockfd, &frame, sizeof(frame));
        if (nbytes < 0) continue;

        // Example PGN decoding (replace with your real logic)
        if (frame.can_id == 0x0CF00400) { // engine speed
            int rpm_raw = frame.data[3] << 8 | frame.data[2];
            data.rpm = rpm_raw / 8;
        }

        if (frame.can_id == 0x0CF00300) { // vehicle speed
            int spd_raw = frame.data[1];
            data.spd = spd_raw;
        }
    }
}
