#pragma once
#include <asio.hpp>
#include <entity/fwd.hpp>
#include <Utils/ConcurrentQueue.h>

class NetworkServer;
class BaseSocket;
namespace moddycamel
{
    class ConcurrentQueue;
}
class ConnectionUpdateSystem
{
public:
    static void Update(entt::registry& registry);

    // Handlers for Network Server
    static void HandleConnection(NetworkServer* server, asio::ip::tcp::socket* socket, const asio::error_code& error);

    // Handlers for Network Client
    static void HandleRead(BaseSocket* socket);
    static void HandleDisconnect(BaseSocket* socket);
};

class ConnectionDeferredSystem
{
public:
    static void Update(entt::registry& registry);
};