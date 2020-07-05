#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace Network
{
    class GeneralHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool HandleConnected(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
        static bool HandleRequestAddress(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
        static bool HandleSendAddress(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
        static bool HandleRequestServerInfo(std::shared_ptr<NetworkClient>, std::shared_ptr<NetworkPacket>&);
    };
}