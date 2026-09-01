// -----------------------------------------------------------------------------
// Class: CommCan
// Description:
//     Singleton responsible for initializing and owning the SocketCAN interface.
//     Provides access to the CAN socket file descriptor and configuration
//     utilities such as enabling non-blocking mode.
//
// Responsibilities:
//     - Initialize CAN socket (Init)
//     - Configure socket flags (SetNonBlock)
//     - Provide socket FD to CANReader and CANWriter (GetSocket)
//
// Used by:
//     - Dashboard (initialization)
//     - CANReader (read FD)
//     - CANWriter (write FD)
// -----------------------------------------------------------------------------
#pragma once
#include "CanMessage.hpp"

class CommCan
{
public:
    // -------------------------------------------------------------------------
    // Function: Instance
    // Description:
    //     Returns the global singleton instance of CommCan.
    // -------------------------------------------------------------------------
    static CommCan & Instance();

    // -------------------------------------------------------------------------
    // Function: Init
    // Description:
    //     Initializes and binds the J1939 CAN socket using the specified local
    //     source address. Must be called before any CAN read/write operations.
    // -------------------------------------------------------------------------
    void Init(CanMessage::SourceAddress source);

    // -------------------------------------------------------------------------
    // Function: SetNonBlock
    // Description:
    //     Sets the CAN socket file descriptor to non-blocking mode. Prevents
    //     read() and write() from blocking worker threads.
    // -------------------------------------------------------------------------
    void SetNonBlock();

    /// @brief Sends a J1939 PGN with the specified payload.
    /// @param pgn PGN to transmit.
    /// @param data Pointer to the payload data.
    /// @param len Payload length in bytes.
    /// @return true if the complete payload was transmitted successfully.
    bool SendPgn(
        CanMessage::PgnType pgn,
        const uint8_t* data);

    // -------------------------------------------------------------------------
    // Function: GetSocket
    // Description:
    //     Returns the CAN socket file descriptor.
    // -------------------------------------------------------------------------
    int GetSocket() const;

private:
    CommCan();
    CommCan(const CommCan &) = delete;
    CommCan & operator=(const CommCan &) = delete;

    int sock;
};
