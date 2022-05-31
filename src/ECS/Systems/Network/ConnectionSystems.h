#pragma once
#include <entity/fwd.hpp>

class NetClient;

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