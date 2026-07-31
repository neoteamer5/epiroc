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
    bool operator()(const Message& a, const Message& b) const {
        bool a_fault = (a.pgn == 0xEF00);
        bool b_fault = (b.pgn == 0xEF00);

        if (a_fault && !b_fault) return false;
        if (b_fault && !a_fault) return true;

        return false;
    }
};

// -------------------- Queues --------------------

using InQueue  = std::priority_queue<Message, std::vector<Message>, Compare>;
using OutQueue = std::queue<Command>;

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

void enqueue_in(const Message& m) {
    std::lock_guard<std::mutex> lock(inMutex);
    inQ.push(m);
}

// -------------------- Handlers --------------------

void handle_speed(const uint8_t* data) {
    int speed = data[0] | (data[1] << 8);
    std::cout << "Manager: SPEED=" << speed << "\n";
}

void handle_rpm(const uint8_t* data) {
    int rpm = data[0] | (data[1] << 8);
    std::cout << "Manager: RPM=" << rpm << "\n";
}

void handle_fuel(const uint8_t* data) {
    int fuel = data[0] | (data[1] << 8);
    std::cout << "Manager: FUEL=" << fuel << "\n";
}

void handle_temp(const uint8_t* data) {
    int temp = data[0];
    std::cout << "Manager: TEMP=" << temp << "\n";

    Command cmd{};
    cmd.pump = (temp > 80 ? 70 : 40);
    cmd.fan  = (temp > 80 ? 70 : 40);

    std::lock_guard<std::mutex> lock(outMutex);
    outQ.push(cmd);
}

void handle_lamp(const uint8_t* /*data*/) {
    std::cout << "Manager: FAULT PGN FECA : lamp\n";

    Command cmd{};
    cmd.pump = 100;
    cmd.fan  = 100;

    std::lock_guard<std::mutex> lock(outMutex);
    outQ.push(cmd);
}

void handle_fault(const uint8_t* /*data*/) {
    std::cout << "Manager: FAULT PGN EF00 : emergency cooling\n";

    Command cmd{};
    cmd.pump = 100;
    cmd.fan  = 100;

    std::lock_guard<std::mutex> lock(outMutex);
    outQ.push(cmd);
}

void handle_unknown(uint32_t pgn) {
    std::cout << "Manager: UNKNOWN PGN 0x" << std::hex << pgn << std::dec << "\n";
}

// -------------------- Process Thread --------------------

void process_thread() {
    while (true) {

        // Drain incoming queue
        {
            std::lock_guard<std::mutex> lock(inMutex);

            while (!inQ.empty()) {
                Message m = inQ.top();
                inQ.pop();

                switch (m.pgn) {
                    case 0xFEF2: handle_speed(m.data); break;   // Speed
                    case 0xF004: handle_rpm(m.data); break;     // RPM
                    case 0xFEFC: handle_fuel(m.data); break;    // Fuel
                    case 0xFEEE: handle_temp(m.data); break;    // Temperature
                    case 0xFECA: handle_lamp(m.data); break;    // Lamp / Warning
                    case 0xEF00: handle_fault(m.data); break;   // Fault
                    default:     handle_unknown(m.pgn); break;
                }

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

        Message m{};
        m.pgn = extract_pgn(frame);
        std::memcpy(m.data, frame.data, 8);

        enqueue_in(m);
    }

}

// -------------------- Writer Thread --------------------

void writer_thread() {


    while (true) {

        Command cmd{};
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
    init_socket();
    
    std::thread t_reader(reader_thread);
    std::thread t_process(process_thread);
    std::thread t_writer(writer_thread);

    t_reader.join();
    t_process.join();
    t_writer.join();

    close(sock);

    return 0;
}
