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

CommCan & CommCan::Instance()
{
    static CommCan instance;
    return instance;
}

CommCan::CommCan()
{
}

void CommCan::Init()
{
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    int recv_own = 0;
    setsockopt(sock, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
               &recv_own, sizeof(recv_own));

    struct ifreq ifr{};
    std::strcpy(ifr.ifr_name, "vcan0");
    ioctl(sock, SIOCGIFINDEX, &ifr);

    struct sockaddr_can addr{};
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
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
