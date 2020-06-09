#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>

struct ConnectionComponent
{
    ConnectionComponent() : packetQueue(256) { }

    std::shared_ptr<NetworkClient> connection;
    moodycamel::ConcurrentQueue<NetworkPacket*> packetQueue;
};