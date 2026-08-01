#include <iostream>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <signal.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "SocketCAN.h"
#include "CanMessage.hpp"
#include "CommCan.hpp"
#include "CanProcessor.hpp"
#include "CanReader.hpp"
#include "CanWriter.hpp"


// -------------------- Priority Compare (EF00 always first) --------------------

struct Compare {
    bool operator()(const CanMessage& a, const CanMessage& b) const {
        bool a_fault = (a.pgn == 0xEF00);
        bool b_fault = (b.pgn == 0xEF00);

        if (a_fault && !b_fault) return false;
        if (b_fault && !a_fault) return true;

        return false;
    }
};

// -------------------- Queues --------------------

using InQueue  = std::priority_queue<CanMessage, std::vector<CanMessage>, Compare>;
using OutQueue = std::queue<CanCommand>;

InQueue  inQ;
OutQueue outQ;

std::mutex inMutex;
std::mutex outMutex;


// -------------------- Handlers --------------------
void handle_speed(const CanMessage& msg)
{
    const auto* data = msg.data;

    int speed = data[0] | (data[1] << 8);
    std::cout << "Manager: SPEED=" << speed << "\n";
}

void handle_rpm(const CanMessage& msg)
{
    const auto* data = msg.data;

    int rpm = data[0] | (data[1] << 8);
    std::cout << "Manager: RPM=" << rpm << "\n";
}

void handle_fuel(const CanMessage& msg)
{
    const auto* data = msg.data;

    int fuel = data[0] | (data[1] << 8);
    std::cout << "Manager: FUEL=" << fuel << "\n";
}

void handle_temp(const CanMessage& msg)
{
    const auto* data = msg.data;

    int temp = data[0];
    std::cout << "Manager: TEMP=" << temp << "\n";

    CanCommand cmd{};
    cmd.pump = (temp > 80 ? 70 : 40);
    cmd.fan  = (temp > 80 ? 70 : 40);

}

void handle_lamp(const CanMessage& /*msg*/)
{
    std::cout << "Manager: FAULT PGN FECA : lamp\n";
}

void handle_fault(const CanMessage& /*msg*/)
{
    std::cout << "Manager: FAULT PGN EF00 : emergency cooling\n";

    CanCommand cmd{};
    cmd.pgn = CanMessage::PgnType::Fault;
    cmd.pump = 100;
    cmd.fan  = 100;

    CanProcessor::Instance().PushCommand(cmd);
}

void handle_unknown(const CanMessage& msg)
{
    std::cout << "Manager: UNKNOWN PGN 0x"
              << std::hex << msg.pgn
              << std::dec << "\n";
}


// -------------------- Main --------------------

int main() {
    //init_socket();
    CommCan::Instance().Init();

    CanReader::Instance().Init(&CommCan::Instance());
    CanWriter::Instance().Init(&CommCan::Instance());


    //CanProcessor::Instance().Init();
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Speed, handle_speed);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Rpm, handle_rpm);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Temp, handle_temp);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Fuel, handle_fuel);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Fault, handle_fault);
    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Lamp, handle_lamp);
    
    CanReader::Instance().Connect(&CanProcessor::Instance());
    CanWriter::Instance().Connect(&CanProcessor::Instance());

    CanProcessor::Instance().Start();
    CanWriter::Instance().Start();
    CanReader::Instance().Start();

    CanWriter::Instance().Join();
    CanReader::Instance().Join();


    return 0;
}
