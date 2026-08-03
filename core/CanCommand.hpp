#pragma once

#include "CanMessage.hpp"

/// @brief Represents an outgoing command derived from a CAN/J1939 message.
///
/// CanCommand inherits the PGN and DecodePgn() logic from CanMessage but
/// restricts the payload to 4 bytes, which is sufficient for pump/fan control.
class CanCommand : public CanMessage
{
public:
    CanCommand()
    {
        pump = 0;
        fan  = 0;
    }

    CanCommand& operator=(const CanCommand& other)
    {
        if (this != &other)
        {
            pgn  = other.pgn;
            pump = other.pump;
            fan  = other.fan;
        }
        return *this;
    }

    union
    {
        uint8_t payload[4];

        struct
        {
            uint16_t pump;
            uint16_t fan;
        };
    };
};
