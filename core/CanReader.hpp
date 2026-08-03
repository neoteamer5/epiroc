#pragma once

#include <thread>
#include <atomic>
#include "CommCan.hpp"
#include "CanMessage.hpp"
#include "CanProcessor.hpp"

/// @brief Reads CAN frames from the CommCan socket and pushes CanMessage objects
///        into CanProcessor.
///
/// CanReader runs a background thread that continuously reads CAN frames from
/// the CAN device. Each frame is converted into a CanMessage structure and pushed
/// into CanProcessor for further handling.
class CanReader
{
public:
    /// @brief Returns the global singleton instance.
    static CanReader & Instance();

    /// @brief Initializes the reader with a CommCan object.
    ///
    /// @param comm Pointer to CommCan providing the CAN socket.
    void Init(CommCan * comm);

    /// @brief Connects the reader to the CanProcessor instance.
    ///
    /// @param proc Pointer to CanProcessor.
    void Connect(CanProcessor * proc);

    /// @brief Starts the reader thread.
    void Start();

    /// @brief Waits for the reader thread to finish.
    void Join();

private:
    CanReader();
    void Loop();

    int fd { -1 };
    CanProcessor * processor { nullptr };

    std::thread th;
    std::atomic<bool> running { false };

    CanReader(const CanReader &) = delete;
    CanReader & operator=(const CanReader &) = delete;
};
