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

    NC_LOG_MESSAGE("Received CMSG_LOGON_CHALLENGE");
    NC_LOG_MESSAGE("Version (%u, %u, %u)", logonChallenge.majorVersion, logonChallenge.patchVersion, logonChallenge.minorVersion);
    NC_LOG_MESSAGE("Game Build (%s, %u)", logonChallenge.BuildTypeString().c_str(), logonChallenge.gameBuild);
    NC_LOG_MESSAGE("Game Name (%s)", logonChallenge.gameName.c_str());
    NC_LOG_MESSAGE("Username (%s)", logonChallenge.username.c_str());

    client->username = logonChallenge.username;

    std::shared_ptr<ByteBuffer> sBuffer = ByteBuffer::Borrow<4>();
    std::shared_ptr<ByteBuffer> vBuffer = ByteBuffer::Borrow<256>();
    SRPUtils::CreateAccount(client->username.c_str(), "test", sBuffer.get(), vBuffer.get());

    client->srp.saltBuffer = sBuffer;
    client->srp.verifierBuffer = vBuffer;

    // If "StartVerification" fails, we have either hit a bad memory allocation or a SRP-6a safety check, thus we should close the connection
    if (!client->srp.StartVerification(client->username, logonChallenge.A, sizeof(logonChallenge.A)))
    {
        client->Close(asio::error::no_data);
        return true;
    }

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<512>();
    ServerLogonChallenge serverChallenge;
    serverChallenge.status = 0;

    std::memcpy(serverChallenge.s, client->srp.saltBuffer->GetDataPointer(), client->srp.saltBuffer->Size);
    std::memcpy(serverChallenge.B, client->srp.bBuffer->GetDataPointer(), client->srp.bBuffer->Size);

    buffer->PutU16(Opcode::SMSG_LOGON_CHALLENGE);
    buffer->PutU16(0);

    u16 payloadSize = serverChallenge.Serialize(buffer);
    buffer->Put<u16>(payloadSize, 2);
    client->Send(buffer.get());
    return true;
}
bool Server::AuthHandlers::ClientHandshakeResponseHandler(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    NC_LOG_MESSAGE("Received CMSG_LOGON_RESPONSE");
    ClientLogonResponse logonResponse;
    logonResponse.Deserialize(packet->payload);

    if (!client->srp.VerifySession(logonResponse.M1))
    {
        NC_LOG_WARNING("Unsuccessful Login");
        client->Close(asio::error::no_permission);
        return true;
    }

    NC_LOG_SUCCESS("Successful Login for: %s", client->username.c_str());

    ServerLogonResponse serverChallenge;
    std::memcpy(serverChallenge.HAMK, client->srp.HAMK, sizeof(client->srp.HAMK));

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<128>();
    buffer->PutU16(Opcode::SMSG_LOGON_RESPONSE);
    buffer->PutU16(0);

    u16 payloadSize = serverChallenge.Serialize(buffer);
    buffer->Put<u16>(payloadSize, 2);
    client->Send(buffer.get());
    return true;
}