#include "DashboardBackend.hpp"

#include "CanProcessor.hpp"
#include "CanReader.hpp"
#include "CanWriter.hpp"
#include "CommCan.hpp"
#include <iostream>

bool DashboardBackend::Init()
{
    if (Initialized)
    {
        return true;
    }

    CommCan::Instance().Init(CanMessage::SourceAddress::Dashboard);

    CanReader::Instance().Init(&CommCan::Instance());

    CanWriter::Instance().Init(&CommCan::Instance());

    CanReader::Instance().Connect(&CanProcessor::Instance());

    CanWriter::Instance().Connect(&CanProcessor::Instance());

    if (!Temp.Init())
    {
        return false;
    }

    if (!Control.Init())
    {
        return false;
    }

    CanProcessor::Instance().RegisterHandler(
        CanMessage::PgnType::Unknown,
        [](const CanMessage& msg)
        {
            std::cout << "Unknown PGN: 0x"
                    << std::hex
                    << static_cast<uint32_t>(msg.pgn)
                    << std::dec << std::endl;
        });

    CanProcessor::Instance().RegisterHandler(CanMessage::PgnType::Temp, Temp);

    Initialized = true;

    return true;
}

void DashboardBackend::Start()
{
    if (!Initialized)
    {
        return;
    }

    CanProcessor::Instance().Start();
    CanWriter::Instance().Start();
    CanReader::Instance().Start();

    Control.Start();
}

void DashboardBackend::Join()
{
    if (!Initialized)
    {
        return;
    }

    Control.Join();

    CanWriter::Instance().Join();
    CanReader::Instance().Join();
    CanProcessor::Instance().Join();
}