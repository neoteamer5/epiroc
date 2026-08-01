// -----------------------------------------------------------------------------
// File: CanReader.cpp
// Description:
//     Implements the CanReader singleton. Reads CAN frames from the CommCan
//     socket, converts them into CanMessage objects, and pushes them into
//     CanProcessor for further handling.
// -----------------------------------------------------------------------------

#include "CanReader.hpp"
#include <linux/can.h>
#include <linux/can/raw.h>
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
    struct can_frame frame;
    static int countFrame = 0;
    while (running)
    {
        int nbytes = read(fd, &frame, sizeof(frame));

        if (nbytes < 0)
        {
            continue;
        }
        std::cout << "msg count=" << ++countFrame << std::endl;

        CanMessage msg;
        msg.pgn = CanMessage::extract_pgn(frame);
        std::memcpy(msg.data, frame.data, 8);

        processor->PushMessage(msg);
    }
}
