#include "AuthHandlers.h"
#include "../../../MessageHandler.h"

// @TODO: Remove Temporary Includes when they're no longer needed
#include <Utils\DebugHandler.h>

void Server::AuthHandlers::Setup(MessageHandler* messageHandler)
{
    messageHandler->SetMessageHandler(Opcode::IMSG_HANDSHAKE, Server::AuthHandlers::HandshakeHandler);
    messageHandler->SetMessageHandler(Opcode::IMSG_HANDSHAKE_RESPONSE, Server::AuthHandlers::HandshakeHandler);
}
bool Server::AuthHandlers::HandshakeHandler(NetworkPacket*)
{
    // Handle initial handshake
    NC_LOG_MESSAGE("Received Handshake");
    return true;
}
bool Server::AuthHandlers::HandshakeResponseHandler(NetworkPacket*)
{
    // Handle handshake response
    NC_LOG_MESSAGE("Received Handshake Response");
    return true;
}