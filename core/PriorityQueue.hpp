
#pragma once

#include <queue>
#include <vector>
#include "CanMessage.hpp"
#include "CanCommand.hpp"

/// @brief Comparator for prioritizing incoming CAN messages.
///        Fault PGN (0xEF00) gets highest priority.
struct Compare
{
    bool operator()(const CanMessage & a, const CanMessage & b) const
    {
        bool a_fault = (a.pgn == CanMessage::PgnType::Fault);
        bool b_fault = (b.pgn == CanMessage::PgnType::Fault);

        // Fault messages should appear at the *top* of the priority queue.
        if (a_fault && !b_fault) return false;  // a has higher priority
        if (b_fault && !a_fault) return true;   // b has higher priority

        return false; // otherwise equal priority
    }
};

/// @brief Incoming message queue (priority queue).
using InQueue  = std::priority_queue<
    CanMessage,
    std::vector<CanMessage>,
    Compare
>;

/// @brief Outgoing command queue (FIFO).
using OutQueue = std::queue<CanCommand>;




