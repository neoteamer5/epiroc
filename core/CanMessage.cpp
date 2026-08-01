/**
 * @file CanMessage.cpp
 * @brief Implements PGN decoding utilities for CAN/J1939 messages.
 *
 * This file contains the CanMessage::DecodePgn() function which maps raw PGN
 * values to the strongly typed PgnType enum. It ensures consistent PGN handling
 * across all modules that process incoming CAN frames.
 */

#include <linux/can.h>
#include <linux/can/raw.h>
#include "CanMessage.hpp"

CanMessage::PgnType CanMessage::DecodePgn(uint32_t p)
{
    switch (p)
    {
        case 0xFEF2: return PgnType::Speed;
        case 0xF004: return PgnType::Rpm;
        case 0xFEFC: return PgnType::Fuel;
        case 0xFEEE: return PgnType::Temp;
        case 0xFECA: return PgnType::Lamp;
        case 0xEF00: return PgnType::Fault;
        default:     return PgnType::Unknown;
    }
}


CanMessage::PgnType CanMessage::extract_pgn(const struct can_frame& frame) {
    uint32_t id = frame.can_id & CAN_EFF_MASK;

    uint8_t dp = (id >> 24) & 0x01;
    uint8_t pf = (id >> 16) & 0xFF;
    uint8_t ps = (id >> 8)  & 0xFF;

    uint32_t rawPgn;
    if (pf < 240) {
        rawPgn = (dp << 16) | (pf << 8);
    } else {
        rawPgn =  (dp << 16) | (pf << 8) | ps;
    }
    return CanMessage::DecodePgn(rawPgn);
}

/**
 * frame to message
 * 
 *      CanMessage m{};
        m.pgn = CanMessage::DecodePgn(extract_pgn(frame));
        std::memcpy(m.data, frame.data, 8);


    command to frame

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
 */