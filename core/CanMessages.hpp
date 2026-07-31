#pragma once

/// @brief Provides PGN type definitions and decoding utilities for CAN/J1939.
///
/// The CanMessages class centralizes all PGN identifiers used by the PLC and
/// dashboard. It exposes a strongly typed enum (PgnType) and a static helper
/// function to convert raw PGN integers into PgnType values.
class CanMessages
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
    /// @param pgn Raw PGN extracted from a CAN extended identifier.
    /// @return Corresponding PgnType value, or PgnType::Unknown if not recognized.
    static PgnType DecodePgn(unsigned int pgn);

private:
    CanMessages() = delete;
};
