#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace Server
{
    class AuthHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool ClientHandshakeHandler(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool ClientHandshakeResponseHandler(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}