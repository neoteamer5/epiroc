/**
 * @brief Implements CAN command transmission for outgoing J1939 messages.
 *
 * CanWriter retrieves CanCommand objects from CanProcessor and sends them as CAN
 * frames. This module isolates all CAN write operations to ensure clean
 * separation between processing and I/O.
 */

#include "CanWriter.hpp"
#include "CanProcessor.hpp"
#include "CanCommand.hpp"
#include "CommCan.hpp"
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>

CanWriter & CanWriter::Instance()
{
    static CanWriter instance;
    return instance;
}

CanWriter::CanWriter()
    : canSock(-1)
    , processor(nullptr)
    , running(false)
{
}

void CanWriter::Init(CommCan * comm)
{
    canSock = comm->GetSocket();
}

void CanWriter::Connect(CanProcessor * proc)
{
    processor = proc;
}

void CanWriter::Start()
{
    running = true;
    th = std::thread(&CanWriter::Loop, this);
}

void CanWriter::Join()
{
    if (th.joinable())
    {
        th.join();
    }
}

void CanWriter::Loop()
{
    while (running)
    {
        CanCommand cmd;
        bool ok = CanProcessor::Instance().PopCommand(cmd);

        if (!ok)
        {
            usleep(10000);
            continue;
        }

        // ---------------------------------------------------------------------
        // Build CAN frame
        // ---------------------------------------------------------------------
        struct can_frame frame {};
        frame.can_id  = 0x18EF00E5 | CAN_EFF_FLAG;  // EF00 is fault
        frame.can_dlc = sizeof(cmd.data);

        frame.data[0] = cmd.pump;
        frame.data[1] = cmd.fan;

        // ---------------------------------------------------------------------
        // Transmit CAN frame
        // ---------------------------------------------------------------------
        if (canSock >= 0)
        {
            write(canSock, &frame, sizeof(frame));
        }
    }
}
