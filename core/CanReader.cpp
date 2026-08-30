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
    /***
     * Why max size 1785?

        Each TP data packet (TP.DT) is an 8-byte CAN frame:

        Byte 0       Sequence number
        Byte 1–7     Payload (7 bytes)

        The sequence number is one byte, allowing up to 255 packets:

        255 packets × 7 payload bytes
        = 1785 bytes
     */
    static const uint32_t J1939_MAX_PAYLOAD_SIZE= 1785;
    static uint8_t payload[J1939_MAX_PAYLOAD_SIZE];
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
        if (countFrame % 100 == 0) std::cout << "msg count=" << ++countFrame << std::endl;

        CanMessage msg;
        msg.pgn = static_cast<CanMessage::PgnType>(src.can_addr.j1939.pgn);

        // Demo only: assume an 8-byte payload. Production code should determine
        // the payload length from the PGN definition and the actual received length.
        std::memcpy(msg.data, payload, 8);

        processor->PushMessage(msg);
    }
}
