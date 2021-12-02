#pragma once
#include <memory>

class NetPacketHandler;
class NetClient;
struct NetPacket;
namespace Network
{
    class GeneralHandlers
    {
    public:
        static void Setup(NetPacketHandler*);
        static bool HandleConnected(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
        static bool HandleRequestAddress(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
        static bool HandleSendAddress(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
        static bool HandleRequestServerInfo(std::shared_ptr<NetClient>, std::shared_ptr<NetPacket>);
    };
}