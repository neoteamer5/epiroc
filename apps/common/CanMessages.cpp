#include "CanMessages.h"

/**
 * @file CanMessages.cpp
 * @brief Implements PGN decoding utilities for CAN/J1939 messages.
 *
 * This file contains the DecodePgn() function which maps raw PGN values
 * to the strongly typed PgnType enum. It ensures consistent PGN handling
 * across all modules that process incoming CAN frames.
 *
 */

PgnType DecodePgn(unsigned int pgn)
{
    switch (pgn)
    {
        case 0xFEF2:
            return PgnType::Speed;

        case 0xF004:
            return PgnType::Rpm;

        case 0xFEFC:
            return PgnType::Fuel;

        case 0xFEEE:
            return PgnType::Temp;

        case 0xFECA:
            return PgnType::Lamp;

        case 0xEF00:
            return PgnType::Fault;

        default:
            return PgnType::Unknown;
    }
}
