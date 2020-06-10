#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace Server
{
    class GeneralHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool CMSG_CONNECTED(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool MSG_MOVE_ENTITY(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool MSG_MOVE_HEARTBEAT_ENTITY(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool MSG_MOVE_STOP_ENTITY(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}