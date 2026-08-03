#include <atomic>

typedef struct CANData {
    std::atomic<int> spd;
    std::atomic<int> rpm;
    std::atomic<int> fuel;
    std::atomic<int> temp;
    std::atomic<bool> warn;
}CANData;


void print_fixed();

void demo_loop();

void init_socketcan();
void read_loop();

