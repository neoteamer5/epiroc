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
#include <iostream>

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
    // Single thread consumer can safely and correctly check if queue is empty without lock
    if (outQueue.empty())
    {
        return false;
    }
    static int countCmd = 0;
    std::cout << "outQueue size=" << outQueue.size() << " count=" << ++countCmd << std::endl;  
    std::lock_guard<std::mutex> lock(outMutex);
    cmd = outQueue.front();
    outQueue.pop();
    return true;
}

bool CanProcessor::PushCommand(CanCommand & cmd)
{
    std::lock_guard<std::mutex> lock(outMutex);
    outQueue.push(cmd);
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

void CanProcessor::RegisterHandler(CanMessage::PgnType pgn, Handler hdlFnc)
{
    handlers[pgn] = hdlFnc;
}

CanProcessor::Handler CanProcessor::GetHandler(CanMessage::PgnType pgn)
{
    auto it = handlers.find(pgn);
    if (it != handlers.end())
    {
        return it->second;
    }
    //std::cout << pgn << std::endl;
    return handlers[CanMessage::PgnType::Unknown];
}

void CanProcessor::Loop()
{
    while (running)
    {
        CanMessage msg;


        int inQueueSize = inQueue.size();
        if ( inQueueSize > 0 ) std::cout << "inQueue size = " << inQueueSize << std::endl;

        // Single thread consumer can safely and correctly check if queue is empty without lock
        if (inQueue.empty())
        {
            usleep(100000);
            continue;
        }
        else
        {
            std::lock_guard<std::mutex> lock(inMutex);
            msg = inQueue.top();
            inQueue.pop();
        }

        auto handle = GetHandler(msg.pgn);
        if (handle != nullptr)
        {
            handle(msg);   
        }
       
    }
}
