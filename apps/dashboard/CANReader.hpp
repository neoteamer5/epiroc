/**
 * CANReader.hpp
 * 
 * When the worker thread start, the real_loop will poll the vcan0 to read the CANData and save into a buffer, which is 'data' (CANReader class' public member).
 * So, the consumer shall creat a static CANReader and start the worker thread so that CANData become live.
 * So, the consumer need a wrapper api to start the worker thread and get the CANData buffer pointer.
 * I have drafted the wrapper api, can_reader_c_api.hpp, which is a pure c api that can support various types of consumer such as python ctypes.CDLL
 * can_reader is depending on Linux but can_reader_c_api might not be aware of the existance of Linux as long as it get the memory address of CANData buffer.
 * So, can_reader_c_api could be an abstraction layer to consume CANData.
 * 
 * Currently the python consumer, test_c_api.py, tests the can_reader_c_api with demo_loop successfully;  for the real_loop, it has also been successfully tested by
 * test_c_api.cpp.
 * 
 */ 

#pragma once
#include <atomic>
#include <thread>

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
    std::thread worker;
    bool running = true;
    int sockfd = -1;
    void demo_loop();
    void real_loop();
};
