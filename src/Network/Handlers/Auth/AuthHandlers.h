#pragma once
#include <memory>

class NetPacketHandler;
class NetClient;
struct NetPacket;
namespace Network
{
    class AuthHandlers
    {
    public:
        static void Setup(NetPacketHandler*);
        static bool ClientChallengeHandler(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
        static bool ClientHandshakeHandler(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
    };
}