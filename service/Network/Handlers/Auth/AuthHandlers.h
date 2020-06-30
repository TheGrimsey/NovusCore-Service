#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace Network
{
    class AuthHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool ClientChallengeHandler(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool ClientHandshakeHandler(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}