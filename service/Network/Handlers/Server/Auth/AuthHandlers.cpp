#include "AuthHandlers.h"
#include "../../../MessageHandler.h"
#include "../../../NetworkClient.h"
#include "../../../NetworkPacket.h"
#include <Utils/ByteBuffer.h>

// @TODO: Remove Temporary Includes when they're no longer needed
#include <Utils/DebugHandler.h>

void Server::AuthHandlers::Setup(MessageHandler* messageHandler)
{
    messageHandler->SetMessageHandler(Opcode::CMSG_LOGON_CHALLENGE, Server::AuthHandlers::ClientHandshakeHandler);
    messageHandler->SetMessageHandler(Opcode::CMSG_LOGON_RESPONSE, Server::AuthHandlers::ClientHandshakeResponseHandler);
}
bool Server::AuthHandlers::ClientHandshakeHandler(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    ClientLogonChallenge logonChallenge;
    logonChallenge.Deserialize(packet->payload);

    /*NC_LOG_MESSAGE("Received CMSG_LOGON_CHALLENGE");
    NC_LOG_MESSAGE("Version (%u, %u, %u)", logonChallenge.majorVersion, logonChallenge.patchVersion, logonChallenge.minorVersion);
    NC_LOG_MESSAGE("Game Build (%s, %u)", logonChallenge.BuildTypeString().c_str(), logonChallenge.gameBuild);
    NC_LOG_MESSAGE("Game Name (%s)", logonChallenge.gameName.c_str());
    NC_LOG_MESSAGE("Username (%s)", logonChallenge.username.c_str());

    client->username = logonChallenge.username;
   
    u8* s = reinterpret_cast<u8*>(client->salt.data());
    u8* v = reinterpret_cast<u8*>(client->verifier.data());

    client->srp.CreateAccount(client->username, "test");
    client->srp_verifier = client->srp.InitializeSRP(logonChallenge.username, logonChallenge.A);

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<256>();
    ServerLogonChallenge serverChallenge;
    serverChallenge.status = 0;
    if (client->srp.B)
    {
        std::memcpy(serverChallenge.s, s, 8);
        std::memcpy(serverChallenge.B, client->srp.B, 128);
    }
    else
    {
        serverChallenge.status = 1;
    }

    buffer->PutU16(Opcode::SMSG_LOGON_CHALLENGE);
    buffer->PutU16(0);

    u16 payloadSize = serverChallenge.Serialize(buffer);
    buffer->Put<u16>(payloadSize, 2);
    client->Send(buffer.get());*/
    return true;
}
bool Server::AuthHandlers::ClientHandshakeResponseHandler(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    /*ClientLogonResponse logonResponse;
    logonResponse.Deserialize(packet->payload);

    if (client->srp.VerifySession(client->srp_verifier, logonResponse.M1))
    {
        NC_LOG_SUCCESS("Successful Login");
    }
    else
    {
        std::stringstream ss1;
        for (i32 i = 0; i < 32; i++)
        {
            ss1 << std::hex << static_cast<i32>(logonResponse.M1[i]);
        }
        NC_LOG_WARNING("Unsuccessful Login");
        NC_LOG_MESSAGE("m: %s", ss1.str().c_str());
    }

    // Handle initial handshake
    NC_LOG_MESSAGE("Received Client Handshake Response");
    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<128>();
    buffer->PutU16(Opcode::SMSG_LOGON_RESPONSE);
    buffer->PutU16(0);
    client->Send(buffer.get());*/
    return true;
}