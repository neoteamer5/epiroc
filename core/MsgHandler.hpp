#pragma once

#include "CanMessage.hpp"

/// @brief Interface for processing CAN/J1939 messages.
class MsgHandler
{
public:
    virtual ~MsgHandler() = default;

    /// @brief Initialize the message handler.
    /// @return true if initialization succeeds.
    virtual bool Init()
    {
        return true;
    }

    /// @brief Handle an incoming CAN/J1939 message.
    /// @param msg Incoming CAN message.
    virtual void handle(const CanMessage& msg) = 0;
};