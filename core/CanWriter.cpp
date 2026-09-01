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

        if (!CanProcessor::Instance().PopCommand(cmd))
        {
            usleep(10000);
            continue;
        }

        uint8_t payload[4]{};

        payload[0] = static_cast<uint8_t>(cmd.pump & 0xFF);
        payload[1] = static_cast<uint8_t>((cmd.pump >> 8) & 0xFF);
        payload[2] = static_cast<uint8_t>(cmd.fan & 0xFF);
        payload[3] = static_cast<uint8_t>((cmd.fan >> 8) & 0xFF);

        CommCan::Instance().SendPgn(
            cmd.pgn,
            payload);
    }
}
