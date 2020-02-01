#pragma once

class MessageHandler;
struct NetworkPacket;
namespace Server
{
    class AuthHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool HandshakeHandler(NetworkPacket*);
        static bool HandshakeResponseHandler(NetworkPacket*);
    };
}