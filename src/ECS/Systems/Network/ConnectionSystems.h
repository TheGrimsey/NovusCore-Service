#pragma once
#include <entity/fwd.hpp>
#include <Utils/ConcurrentQueue.h>

class NetworkServer;
class NetClient;
namespace moddycamel
{
    class ConcurrentQueue;
}
class ConnectionUpdateSystem
{
public:
    static void Update(entt::registry& registry);

    // Handlers for Network Server
    static bool HandleConnection(std::shared_ptr<NetClient> netClient);

    // Handlers for Network Client
    static void HandleRead(std::shared_ptr<NetClient> netClient);
    static void HandleDisconnect(std::shared_ptr<NetClient> netClient);
};

class ConnectionDeferredSystem
{
public:
    static void Update(entt::registry& registry);
};