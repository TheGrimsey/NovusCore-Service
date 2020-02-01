#pragma once

class MessageHandler;
struct Packet;
namespace Server
{
    class GeneralHandlers
    {
    public:
        static void Setup(MessageHandler*);
    };
}