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

// -------------------- Message Structures --------------------

struct Message {
    uint32_t pgn;
    uint8_t  data[8];
};

struct Command {
    uint8_t pump;
    uint8_t fan;
};

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

// -------------------- PGN Extraction --------------------

uint32_t extract_pgn(const struct can_frame& frame) {
    uint32_t id = frame.can_id & CAN_EFF_MASK;

    uint8_t dp = (id >> 24) & 0x01;
    uint8_t pf = (id >> 16) & 0xFF;
    uint8_t ps = (id >> 8)  & 0xFF;

    if (pf < 240) {
        return (dp << 16) | (pf << 8);
    } else {
        return (dp << 16) | (pf << 8) | ps;
    }
}

// -------------------- Enqueue Incoming --------------------

void enqueue_in(const CanMessage& m) {
    std::lock_guard<std::mutex> lock(inMutex);
    inQ.push(m);
}

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

    std::lock_guard<std::mutex> lock(outMutex);
    outQ.push(cmd);
}

void handle_lamp(const CanMessage& /*msg*/)
{
    std::cout << "Manager: FAULT PGN FECA : lamp\n";

    CanCommand cmd{};
    cmd.pump = 100;
    cmd.fan  = 100;

    std::lock_guard<std::mutex> lock(outMutex);
    outQ.push(cmd);
}

void handle_fault(const CanMessage& /*msg*/)
{
    std::cout << "Manager: FAULT PGN EF00 : emergency cooling\n";

    CanCommand cmd{};
    cmd.pump = 100;
    cmd.fan  = 100;

    std::lock_guard<std::mutex> lock(outMutex);
    outQ.push(cmd);
}

void handle_unknown(const CanMessage& msg)
{
    std::cout << "Manager: UNKNOWN PGN 0x"
              << std::hex << msg.pgn
              << std::dec << "\n";
}
// -------------------- Process Thread --------------------

void process_thread()
{
    while (true)
    {
        CanMessage m;
        bool hasMessage = false;

        // Pop one message under lock
        {
            std::lock_guard<std::mutex> lock(inMutex);
            if (!inQ.empty())
            {
                m = inQ.top();
                inQ.pop();
                hasMessage = true;
            }
        }

        // Process message outside the lock
        if (hasMessage)
        {
            auto hdl = CanProcessor::Instance().GetHandler(m.pgn);
            if (hdl)
            {
                hdl(m);
            }
        }

        usleep(10000); // 10 ms loop
    }
}


// -------------------- Reader Thread --------------------

void reader_thread() {
    struct can_frame frame{};

    while (true) {
        int nbytes = read(sock, &frame, sizeof(frame));
        if (nbytes < 0) continue;

        CanMessage m{};
        m.pgn = CanMessage::DecodePgn(extract_pgn(frame));
        std::memcpy(m.data, frame.data, 8);

        enqueue_in(m);
    }

}

// -------------------- Writer Thread --------------------

void writer_thread() {


    while (true) {

        CanCommand cmd{};
        bool has_cmd = false;

        {
            std::lock_guard<std::mutex> lock(outMutex);
            if (!outQ.empty()) {
                cmd = outQ.front();
                outQ.pop();
                has_cmd = true;
            }
        }

        if (has_cmd) {
            struct can_frame frame{};
            frame.can_id  = 0x18FF50E5 | CAN_EFF_FLAG;  // example PGN
            frame.can_dlc = 8;

            frame.data[0] = cmd.pump;
            frame.data[1] = cmd.fan;

            int nbytes = write(sock, &frame, sizeof(frame));
            if (nbytes == sizeof(frame)) {
                std::cout << "Writer: sent pump=" << int(cmd.pump)
                          << " fan=" << int(cmd.fan) << "\n";
            }
        }

        usleep(5000);
    }

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

    /*
    std::thread t_reader(reader_thread);
    std::thread t_process(process_thread);
    std::thread t_writer(writer_thread);

    t_reader.join();
    t_process.join();
    t_writer.join();

    close(sock);
    */

    return 0;
}
