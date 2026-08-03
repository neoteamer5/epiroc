#include "CANReader.hpp"
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <iostream>

CANReader::CANReader(bool demo_mode)
    : demo(demo_mode)
{
    std::cerr << "CANReader constructor START\n";

    if (demo) {
        worker = std::thread(&CANReader::demo_loop, this);
        std::cerr << "CANReader constructor END (demo)\n";
        return;
    }

    // Open CAN socket once
    sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sockfd < 0) {
        std::cerr << "socket() failed → switching to demo\n";
        demo = true;
        worker = std::thread(&CANReader::demo_loop, this);
        std::cerr << "CANReader constructor END (socket fail → demo)\n";
        return;
    }

    struct sockaddr_can addr {};
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex("vcan0");

    if (addr.can_ifindex == 0) {
        std::cerr << "vcan0 not found → switching to demo\n";
        close(sockfd);
        sockfd = -1;
        demo = true;
        worker = std::thread(&CANReader::demo_loop, this);
        std::cerr << "CANReader constructor END (no vcan0 → demo)\n";
        return;
    }

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind() failed → switching to demo\n";
        close(sockfd);
        sockfd = -1;
        demo = true;
        worker = std::thread(&CANReader::demo_loop, this);
        std::cerr << "CANReader constructor END (bind fail → demo)\n";
        return;
    }

    // Non‑blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    std::cerr << "vcan0 index = " << addr.can_ifindex << "\n";
    std::cerr << "CANReader constructor END (real)\n";

    // Start worker thread
    worker = std::thread(&CANReader::real_loop, this);
}

CANReader::~CANReader() {
    running = false;
    if (worker.joinable())
        worker.join();
    if (sockfd >= 0)
        close(sockfd);
}

void CANReader::demo_loop() {
    std::cerr << ">>> demo_loop ENTERED\n";
    while (running) {
        data.spd  = std::rand() % 120;
        data.rpm  = std::rand() % 3500;
        data.fuel = std::rand() % 100;
        data.temp = std::rand() % 120;
        data.warn = (data.temp.load() > 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void CANReader::real_loop() {
    std::cerr << ">>> real_loop ENTERED\n";

    if (sockfd < 0) {
        std::cerr << "real_loop: invalid sockfd, exiting\n";
        return;
    }

    struct can_frame frame;

    while (running) {
        int nbytes = read(sockfd, &frame, sizeof(frame));

        if (nbytes < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (nbytes < (int)sizeof(struct can_frame)) {
            continue;
        }

        switch (frame.can_id & 0x1FFFFFFF) {
            case 0x0CF00400: { // engine speed
                int rpm_raw = (frame.data[3] << 8) | frame.data[2];
                data.rpm = rpm_raw / 8;
                break;
            }
            case 0x0CF00300: { // vehicle speed
                data.spd = frame.data[1];
                break;
            }
            case 0x18FEEE00: { // coolant temp
                data.temp = frame.data[0];
                break;
            }
            case 0x18FEF200: { // fuel level
                data.fuel = frame.data[0];
                break;
            }
        }

        data.warn = (data.temp.load() > 100);
    }

    std::cerr << "real_loop exiting\n";
}
