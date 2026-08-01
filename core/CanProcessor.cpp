/**
 * @brief Implements CAN message processing and command generation.
 *
 * CanProcessor receives incoming CanMessage objects from CanReader, interprets
 * their PGNs using CanMessage::DecodePgn(), and produces CanCommand objects
 * for CanWriter. This module owns both the incoming and outgoing queues and
 * runs a background thread to continuously process messages.
 */

#include "CanProcessor.hpp"
#include "CanMessage.hpp"
#include <unistd.h>

CanProcessor & CanProcessor::Instance()
{
    static CanProcessor instance;
    return instance;
}

CanProcessor::CanProcessor()
    : running(false)
{
}

void CanProcessor::PushMessage(const CanMessage & msg)
{
    std::lock_guard<std::mutex> lock(inMutex);
    inQueue.push(msg);
}

bool CanProcessor::PopCommand(CanCommand & cmd)
{
    std::lock_guard<std::mutex> lock(outMutex);

    if (outQueue.empty())
    {
        return false;
    }

    cmd = outQueue.front();
    outQueue.pop();
    return true;
}

void CanProcessor::Start()
{
    running = true;
    th = std::thread(&CanProcessor::Loop, this);
}

void CanProcessor::Join()
{
    if (th.joinable())
    {
        th.join();
    }
}

void CanProcessor::Loop()
{
    while (running)
    {
        CanMessage msg;

        // Try to get a message
        {
            std::lock_guard<std::mutex> lock(inMutex);

            if (inQueue.empty())
            {
                usleep(10000);
                continue;
            }

            msg = inQueue.front();
            inQueue.pop();
        }

        // Decode PGN using CanMessage class
        auto pgn = CanMessage::DecodePgn(msg.pgn);

        CanCommand cmd {};
        bool push = false;

        // ---------------------------------------------------------------------
        // PGN-based processing logic
        // ---------------------------------------------------------------------

        switch (pgn)
        {
            case CanMessage::PgnType::Temp:
                cmd.pump = (msg.data[0] > 80 ? 70 : 40);
                cmd.fan  = (msg.data[0] > 80 ? 70 : 40);
                push = true;
                break;

            case CanMessage::PgnType::Lamp:
            case CanMessage::PgnType::Fault:
                cmd.pump = 100;
                cmd.fan  = 100;
                push = true;
                break;

            default:
                break;
        }

        // Push command if generated
        if (push)
        {
            std::lock_guard<std::mutex> lock(outMutex);
            outQueue.push(cmd);
        }
    }
}
