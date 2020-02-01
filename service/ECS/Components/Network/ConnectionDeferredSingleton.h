#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <asio.hpp>
#include "../../../Network/NetworkServer.h"

struct ConnectionDeferredSingleton
{
    ConnectionDeferredSingleton() : newConnectionQueue(256), droppedConnectionQueue(256) { }

    std::shared_ptr<NetworkServer> networkServer;
    moodycamel::ConcurrentQueue<asio::ip::tcp::socket*> newConnectionQueue;
    moodycamel::ConcurrentQueue<u64> droppedConnectionQueue;
};