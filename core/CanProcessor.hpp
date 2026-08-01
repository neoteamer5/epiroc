#pragma once

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include "CanMessage.hpp"
#include "CanCommand.hpp"

/// @brief Singleton that processes incoming CAN messages and generates outgoing commands.
///
/// CanProcessor owns two internal queues:
///   - Incoming CanMessage queue (filled by CanReader)
///   - Outgoing CanCommand queue (consumed by CanWriter)
///
/// Responsibilities:
///   - Provide PushMessage() for CanReader
///   - Provide PopCommand() for CanWriter
///   - Process incoming PGNs and generate CanCommand objects
///
/// Used by:
///   - CanReader (pushes CanMessage)
///   - CanWriter (pops CanCommand)
class CanProcessor
{
public:
    /// @brief Returns the global singleton instance.
    static CanProcessor & Instance();

    /// @brief Pushes an incoming CAN message into the processor's input queue.
    ///
    /// @param msg CanMessage object produced by CanReader.
    void PushMessage(const CanMessage & msg);

    /// @brief Pops a CanCommand from the output queue if available.
    ///
    /// @param cmd Reference to CanCommand object to fill.
    /// @return true if a command was retrieved, false if queue is empty.
    bool PopCommand(CanCommand & cmd);

    /// @brief Launches the processor thread.
    void Start();

    /// @brief Waits for the processor thread to finish.
    void Join();

private:
    CanProcessor();
    void Loop();

    std::queue<CanMessage> inQueue;
    std::queue<CanCommand> outQueue;

    std::mutex inMutex;
    std::mutex outMutex;

    std::thread th;
    std::atomic<bool> running { false };

    CanProcessor(const CanProcessor &) = delete;
    CanProcessor & operator=(const CanProcessor &) = delete;
};
