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
        static bool CMSG_CONNECTED(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool MSG_REQUEST_ADDRESS(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool SMSG_SEND_ADDRESS(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool MSG_REQUEST_INTERNAL_SERVER_INFO(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}