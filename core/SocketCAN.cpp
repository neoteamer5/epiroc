// -----------------------------------------------------------------------------
// File: SocketCAN.cpp
// Description:
//     Common SocketCAN initialization and low‑level CAN socket utilities.
//
//     This module provides a shared init_socket() function used by all
//     applications that interact with the CAN bus (PLC simulator, dashboard,
//     can_reader library). It centralizes raw Linux CAN socket setup so that
//     each module does not duplicate the same PF_CAN / SOCK_RAW boilerplate.
//
//     If init_socket() fails, it usually indicates that the Linux CAN setup
//     (SetupLinux) has not completed successfully. In that case, the PLC
//     simulator or other modules may not build or may fail to start because
//     the vcan0 interface is missing.
//
// Responsibilities:
//     - Open PF_CAN / SOCK_RAW CAN_RAW socket
//     - Disable receiving self‑transmitted frames
//     - Resolve interface index for "vcan0"
//     - Bind the socket to the CAN interface
//
// Used by:
//     - apps/plc (PLC simulator)
//     - apps/dashboard (UI visualizer)
//     - can_reader (shared CAN reader library)
//
// -----------------------------------------------------------------------------

#include "SocketCAN.h"
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cstring>
#include <fcntl.h>
int sock = -1;

void init_socket(bool noBlock)
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

    if (noBlock)
    {
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }
}