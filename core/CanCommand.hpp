#pragma once

#include "CanMessage.hpp"

/// @brief Represents an outgoing command derived from a CAN/J1939 message.
///
/// CanCommand inherits the PGN and DecodePgn() logic from CanMessage but
/// restricts the payload to 2 bytes, which is sufficient for pump/fan control.
class CanCommand : public CanMessage
{
public:
    /// @brief Copy assignment operator.
    CanCommand & operator=(const CanCommand & other)
    {
        if (this != &other)
        {
            // Copy base class fields
            this->pgn = other.pgn;

            // Copy command fields
            this->pump = other.pump;
            this->fan  = other.fan;
        }
        return *this;
    }

    /// @brief Two‑byte command payload.
    ///
    /// Byte 0 = pump control (0–255)
    /// Byte 1 = fan control  (0–255)
    uint8_t data[2] { 0, 0 };

    /// @brief Convenience accessors for pump/fan values.
    uint8_t & pump { data[0] };
    uint8_t & fan  { data[1] };
};
