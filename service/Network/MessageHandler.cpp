#include "MessageHandler.h"
#include <NovusTypes.h>
#include "NetworkPacket.h"

MessageHandler::MessageHandler()
{
    for (i32 i = 0; i < Opcode::OPCODE_MAX_COUNT; i++)
    {
        handlers[i] = nullptr;
    }
}

void MessageHandler::SetMessageHandler(Opcode opcode, MessageHandlerFn func)
{
    handlers[opcode] = func;
}

bool MessageHandler::CallHandler(NetworkPacket* packet)
{
    return handlers[packet->header.opcode] ? handlers[packet->header.opcode](packet) : true;
}