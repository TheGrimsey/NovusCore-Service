#pragma once
#include "Opcodes.h"

struct NetworkPacket;
class MessageHandler
{
typedef bool (*MessageHandlerFn)(NetworkPacket*);

public:
    MessageHandler();

    void SetMessageHandler(Opcode opcode, MessageHandlerFn func);
    bool CallHandler(NetworkPacket* packet);

private:
    MessageHandlerFn handlers[Opcode::OPCODE_MAX_COUNT];
};