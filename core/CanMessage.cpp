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
    return static_cast<CanMessage::PgnType>(rawPgn);
}

uint32_t CanMessage::MakeJ1939CanId(PgnType pgn, SourceAddress sa)
{
    constexpr uint32_t priority = 6;

    return (priority << 26)
         | (static_cast<uint32_t>(pgn) << 8)
         | static_cast<uint8_t>(sa);
}
