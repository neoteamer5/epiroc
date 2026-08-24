// -----------------------------------------------------------------------------
// File: CanReader.cpp
// Description:
//     Implements the CanReader singleton. Reads CAN frames from the CommCan
//     socket, converts them into CanMessage objects, and pushes them into
//     CanProcessor for further handling.
// -----------------------------------------------------------------------------

#include "CanReader.hpp"
#include <linux/can/j1939.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

CanReader & CanReader::Instance()
{
    static CanReader instance;
    return instance;
}

CanReader::CanReader()
    : fd(-1)
    , processor(nullptr)
    , running(false)
{
}

void CanReader::Init(CommCan * comm)
{
    fd = comm->GetSocket();
}

void CanReader::Connect(CanProcessor * proc)
{
    processor = proc;
}

void CanReader::Start()
{
    running = true;
    th = std::thread(&CanReader::Loop, this);
}

void CanReader::Join()
{
    if (th.joinable())
    {
        th.join();
    }
}

void CanReader::Loop()
{
    uint8_t payload[8];
    static int countFrame = 0;
    while (running)
    {
        sockaddr_can src{};
        socklen_t src_len = sizeof(src);
        int nbytes = recvfrom(fd, payload, sizeof(payload),
            0, reinterpret_cast<sockaddr*>(&src), &src_len); 

        if (nbytes < 0)
        {
            continue;
        }
        std::cout << "msg count=" << ++countFrame << std::endl;

        CanMessage msg;
        msg.pgn = static_cast<CanMessage::PgnType>(src.can_addr.j1939.pgn);
        std::memcpy(msg.data, payload, 8);

        processor->PushMessage(msg);
    }
}
