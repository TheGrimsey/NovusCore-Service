#include "AuthHandlers.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>
#include <Utils/ByteBuffer.h>
#include <Utils/StringUtils.h>
#include <Database/DBConnection.h>
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Network/Authentication.h"

// @TODO: Remove Temporary Includes when they're no longer needed
#include <Utils/DebugHandler.h>

void Network::AuthHandlers::Setup(MessageHandler* messageHandler)
{
    messageHandler->SetMessageHandler(Opcode::CMSG_LOGON_CHALLENGE, Network::AuthHandlers::ClientChallengeHandler);
    messageHandler->SetMessageHandler(Opcode::CMSG_LOGON_HANDSHAKE, Network::AuthHandlers::ClientHandshakeHandler);
}
bool Network::AuthHandlers::ClientChallengeHandler(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    if (client->GetStatus() != ConnectionStatus::AUTH_CHALLENGE)
        return false;

    std::string inUsername;
    u8 aBuffer[256];

    packet->payload->GetString(inUsername);
    packet->payload->GetBytes(aBuffer, 256);

    entt::registry* registry = ServiceLocator::GetRegistry();
    Authentication& authentication = registry->get<Authentication>(static_cast<entt::entity>(client->GetEntityId()));
    authentication.username = inUsername;

    std::shared_ptr<Bytebuffer> sBuffer = Bytebuffer::Borrow<4>();
    std::shared_ptr<Bytebuffer> vBuffer = Bytebuffer::Borrow<256>();

    DBConnection conn("localhost", 3306, "root", "ascent", "novuscore", 0, 2);

    std::stringstream ss;
    ss << "SELECT salt, verifier FROM accounts WHERE username='" << authentication.username << "';";

    std::shared_ptr<QueryResult> result = conn.Query(ss.str());
    conn.Close();

    // If we found no account with the provided username we "temporarily" close the connection
    // TODO: Generate Random Salt & Verifier (Shorter length?) and "fake" logon challenge to not give away if an account exists or not
    if (result->GetAffectedRows() == 0)
    {
        NC_LOG_WARNING("Unsuccessful Login for: %s", authentication.username.c_str());
        return false;
    }

    result->GetNextRow();
    {
        const Field& saltField = result->GetField(0);
        const Field& verifierField = result->GetField(1);

        std::string salt = saltField.GetString();
        StringUtils::HexStrToBytes(salt.c_str(), sBuffer->GetDataPointer());

        std::string verifier = verifierField.GetString();
        StringUtils::HexStrToBytes(verifier.c_str(), vBuffer->GetDataPointer());
    }

    authentication.srp.saltBuffer = sBuffer;
    authentication.srp.verifierBuffer = vBuffer;

    // If "StartVerification" fails, we have either hit a bad memory allocation or a SRP-6a safety check, thus we should close the connection
    if (!authentication.srp.StartVerification(authentication.username, aBuffer))
    {
        NC_LOG_WARNING("Unsuccessful Login for: %s", authentication.username.c_str());
        return false;
    }

    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<512>();
    ServerLogonChallenge serverChallenge;
    serverChallenge.status = 0;

    std::memcpy(serverChallenge.s, authentication.srp.saltBuffer->GetDataPointer(), authentication.srp.saltBuffer->size);
    std::memcpy(serverChallenge.B, authentication.srp.bBuffer->GetDataPointer(), authentication.srp.bBuffer->size);

    buffer->Put(Opcode::SMSG_LOGON_CHALLENGE);
    buffer->SkipWrite(sizeof(u16));

    u16 payloadSize = serverChallenge.Serialize(buffer);
    buffer->Put<u16>(payloadSize, 2);
    client->Send(buffer.get());

    client->SetStatus(ConnectionStatus::AUTH_HANDSHAKE);
    return true;
}
bool Network::AuthHandlers::ClientHandshakeHandler(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    if (client->GetStatus() != ConnectionStatus::AUTH_HANDSHAKE)
        return false;

    ClientLogonHandshake logonResponse;
    logonResponse.Deserialize(packet->payload);

    entt::registry* registry = ServiceLocator::GetRegistry();
    Authentication& authentication = registry->get<Authentication>(static_cast<entt::entity>(client->GetEntityId()));

    if (!authentication.srp.VerifySession(logonResponse.M1))
    {
        NC_LOG_WARNING("Unsuccessful Login for: %s", authentication.username.c_str());
        return false;
    }
    else
    {
        NC_LOG_SUCCESS("Successful Login for: %s", authentication.username.c_str());
    }

    // Update Key field in accounts table in the database here

    ServerLogonHandshake serverChallenge;
    std::memcpy(serverChallenge.HAMK, authentication.srp.HAMK, sizeof(authentication.srp.HAMK));

    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
    buffer->Put(Opcode::SMSG_LOGON_HANDSHAKE);
    buffer->SkipWrite(sizeof(u16));

    u16 payloadSize = serverChallenge.Serialize(buffer);
    buffer->Put<u16>(payloadSize, 2);
    client->Send(buffer.get());

    client->SetStatus(ConnectionStatus::AUTH_SUCCESS);
    return true;
}