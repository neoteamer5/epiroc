#pragma once

#include "CoolingInputs.hpp"
#include "MsgHandler.hpp"

/// @brief Handles coolant temperature CAN/J1939 messages.
class TempHandler : public MsgHandler
{
public:
    explicit TempHandler(CoolingInputs& inputs);

    /// @brief Initializes the temperature handler.
    /// @return true if initialization succeeds.
    bool Init() override;

    /// @brief Processes a coolant temperature message.
    /// @param msg Incoming CAN/J1939 message.
    void handle(const CanMessage& msg) override;

private:
    CoolingInputs& Inputs;
};