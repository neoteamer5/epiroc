#include <iostream>
#include <thread>
#include <chrono>
#include "can_reader_c_api.hpp"

int main(int argc, char** argv)
{
    bool use_demo = true;
    if (argc > 1 && std::string(argv[1]) == "can")
        use_demo = false;

    if (use_demo) {
        std::cout << "Starting demo CAN reader...\n";
        start_can_reader_demo();
    } else {
        std::cout << "Starting REAL CAN reader...\n";
        start_can_reader_real();
    }

    CANData_C data{};

    while (true) {
        if (get_can_data(&data) == 0) {
            std::cout << "SPD="  << data.spd
                      << " RPM=" << data.rpm
                      << " FUEL=" << data.fuel
                      << " TEMP=" << data.temp
                      << " WARN=" << data.warn
                      << "\n";
        } else {
            std::cout << "get_can_data failed\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
