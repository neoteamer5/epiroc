/**
 * @brief Implements CAN command transmission for outgoing J1939 messages.
 *
 * CanWriter retrieves Command objects from CanProcessor and sends them as CAN
 * frames. This module isolates all CAN write operations to ensure clean
 * separation between processing and I/O.
 */

#include "CanWriter.hpp"
#include "CanProcessor.hpp"
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>

void CanWriter::Init(int sock)
{
    canSock = sock;
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
        Command cmd;
        bool ok = CanProcessor::Instance().PopCommand(cmd);

        if (!ok)
        {
            usleep(10000);
            continue;
        }

        // ---------------------------------------------------------------------
        // Build CAN frame (example — adapt to your PGN/command format)
        // ---------------------------------------------------------------------
        struct can_frame frame {};
        frame.can_id  = 0x18FF0000;   // Example PGN
        frame.can_dlc = 8;

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
