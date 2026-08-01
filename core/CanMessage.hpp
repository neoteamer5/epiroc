#pragma once

#include <cstdint>

/// @brief Represents a decoded CAN/J1939 message.
///
/// CanMessage stores the PGN and raw 8‑byte payload of a CAN frame. It also
/// provides a strongly typed PGN enumeration and a helper function to decode
/// raw PGN integers extracted from CAN extended identifiers.
class CanMessage
{
public:
    /// @brief Strongly typed PGN identifiers for CAN/J1939 messages.
    enum class PgnType
    {
        Speed      = 0xFEF2,
        Rpm        = 0xF004,
        Fuel       = 0xFEFC,
        Temp       = 0xFEEE,
        Lamp       = 0xFECA,
        Fault      = 0xEF00,
        Unknown    = 0x0000
    };

    /// @brief Converts a raw PGN integer into a PgnType enum value.
    ///
    /// @param p Raw PGN extracted from a CAN extended identifier.
    /// @return Corresponding PgnType value, or PgnType::Unknown if not recognized.
    static PgnType DecodePgn(uint32_t p);

    static uint32_t extract_pgn(const struct can_frame& frame);

public:
    /// @brief Raw PGN value extracted from the CAN identifier.
    uint32_t pgn { 0 };

    /// @brief Raw 8‑byte CAN payload.
    uint8_t data[8] { 0 };
};
