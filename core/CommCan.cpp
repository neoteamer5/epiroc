// -----------------------------------------------------------------------------
// File: CommCan.cpp
// Description:
//     Implements the CommCan singleton. Handles initialization of the SocketCAN
//     interface, configuration of socket flags (non-blocking mode), and provides
//     access to the CAN socket file descriptor for all CAN pipeline components.
// -----------------------------------------------------------------------------

#include "CommCan.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <linux/can/j1939.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cstring>
#include <iostream>

CommCan & CommCan::Instance()
{
    static CommCan instance;
    return instance;
}

CommCan::CommCan() : sock(-1)
{
}

void CommCan::Init(CanMessage::SourceAddress source)
{
    //By default, a J1939 socket does not receive messages that the same socket itself transmitted.
    sock = socket(PF_CAN, SOCK_DGRAM, CAN_J1939);
    if (sock < 0)
    {
        return;
    }

    // Linux's J1939 broadcast packets cannot be sent or received by default
    int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));

    struct ifreq ifr{};
    std::strcpy(ifr.ifr_name, "vcan0");
    ioctl(sock, SIOCGIFINDEX, &ifr);

    struct sockaddr_can addr{};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    addr.can_addr.j1939.name = J1939_NO_NAME;
    // Linux's J1939 socket must have an usable local source address unless NAME-based address claiming
    addr.can_addr.j1939.addr = static_cast<uint8_t>(source);
    addr.can_addr.j1939.pgn  = J1939_NO_PGN;

    auto ret = bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    std::cout << "bind ret=" << ret << " ifindex=" << ifr.ifr_ifindex << " source=0x" << std::hex << static_cast<int>(source) << std::dec << "\n";
}

void CommCan::SetNonBlock()
{
    int flags = fcntl(sock, F_GETFL, 0);

    if (flags < 0)
    {
        return;
    }

    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

int CommCan::GetSocket() const
{
    return sock;
}
