#pragma once
#include <atomic>
#include <thread>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

struct CANData {
    std::atomic<int> spd{0};
    std::atomic<int> rpm{0};
    std::atomic<int> fuel{0};
    std::atomic<int> temp{0};
    std::atomic<bool> warn{false};
};

class CANReader {
public:
    CANData data;

    CANReader(bool demo_mode);
    ~CANReader();

private:
    bool demo;
    int sockfd = -1;
    std::thread worker;
    bool running = true;

    void demo_loop();
    void real_loop();
};
