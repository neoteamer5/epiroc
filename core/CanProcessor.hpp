#pragma once

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include "Message.hpp"
#include "Command.hpp"

/// @brief Singleton that processes incoming CAN messages and generates outgoing commands.
///
/// CanProcessor owns two internal queues:
///   - Incoming Message queue (filled by CanReader)
///   - Outgoing Command queue (consumed by CanWriter)
///
/// Responsibilities:
///   - Provide PushMessage() for CanReader
///   - Provide PopCommand() for CanWriter
///   - Process incoming PGNs and generate Commands
///
/// Used by:
///   - CanReader (pushes Message)
///   - CanWriter (pops Command)
class CanProcessor
{
public:
    /// @brief Returns the global singleton instance.
    static CanProcessor & Instance();

    /// @brief Pushes an incoming Message into the processor's input queue.
    ///
    /// @param msg Message object produced by CanReader.
    void PushMessage(const Message & msg);

    /// @brief Pops a Command from the output queue if available.
    ///
    /// @param cmd Reference to Command object to fill.
    /// @return true if a command was retrieved, false if queue is empty.
    bool PopCommand(Command & cmd);

    /// @brief Launches the processor thread.
    void Start();

    /// @brief Waits for the processor thread to finish.
    void Join();

private:
    CanProcessor();
    void Loop();

    std::queue<Message> inQueue;
    std::queue<Command> outQueue;

    std::mutex inMutex;
    std::mutex outMutex;

    std::thread th;
    std::atomic<bool> running { false };

    CanProcessor(const CanProcessor &) = delete;
    CanProcessor & operator=(const CanProcessor &) = delete;
};
