#pragma once

#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

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
///   - Allow external modules to register message handlers
class CanProcessor
{
public:
    /// @brief Handler function type.
    using Handler = std::function<void(const CanMessage &)>;

    /// @brief Returns the global singleton instance.
    static CanProcessor & Instance();

    /// @brief Pushes an incoming CAN message into the processor's input queue.
    void PushMessage(const CanMessage & msg);

    /// @brief Pops a CanCommand from the output queue if available.
    bool PopCommand(CanCommand & cmd);

    /// @brief Launches the processor thread.
    void Start();

    /// @brief Waits for the processor thread to finish.
    void Join();

    /// @brief Registers a handler for the specified PGN.
    /// @note This function is intended to be called only during initialization.
    ///       Do not register handlers at runtime. Once initialization is complete,
    ///       handler lookups can be performed without locking.
    void RegisterHandler(CanMessage::PgnType pgn, Handler hdlFnc);

    /// @brief Returns the handler registered for the specified PGN.
    /// @return The registered handler, or nullptr if no handler is registered.
    ///
    /// @note Safe to call without locking because the handler table is
    ///       populated during initialization and remains immutable afterward.
    Handler GetHandler(CanMessage::PgnType pgn);


private:
    CanProcessor();
    void Loop();

    std::queue<CanMessage> inQueue;
    std::queue<CanCommand> outQueue;

    std::mutex inMutex;
    std::mutex outMutex;

    std::unordered_map<CanMessage::PgnType, Handler> handlers;
    std::mutex handlerMutex;

    std::thread th;
    std::atomic<bool> running { false };

    CanProcessor(const CanProcessor &) = delete;
    CanProcessor & operator=(const CanProcessor &) = delete;
};
