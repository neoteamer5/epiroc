#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include "CommCan.hpp"

/// @brief Sends outgoing CAN commands produced by CanProcessor.
///
/// CanWriter runs a background thread that continuously checks the outgoing
/// command queue (owned by CanProcessor). When a command is available, it
/// formats the CAN frame and transmits it through the CAN socket.
class CanWriter
{
public:
    /// @brief Initializes the reader with a CommCan object.
    ///
    /// @param comm Pointer to CommCan providing the CAN socket.
    void Init(CommCan * comm);

    /// @brief Starts the writer thread.
    void Start();

    /// @brief Waits for the writer thread to finish.
    void Join();

private:
    void Loop();

    int canSock { -1 };
    std::thread th;
    std::atomic<bool> running { false };

    CanWriter(const CanWriter &) = delete;
    CanWriter & operator=(const CanWriter &) = delete;
};
