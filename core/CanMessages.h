#pragma once

/**
 * @file CanMessages.h
 * @brief Defines PGN types and decoding utilities for CAN/J1939 messages.
 *
 * This module centralizes all PGN identifiers used by the PLC and dashboard.
 * It provides a strongly typed enum for PGNs and a helper function to decode
 * raw PGN values extracted from CAN extended identifiers.
 *
 */

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

/**
 * Convert a raw PGN integer into a PgnType enum.
 *
 */
PgnType DecodePgn(unsigned int pgn);
