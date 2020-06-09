#include "AuthHandlers.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>
#include <Utils/ByteBuffer.h>
#include <Utils/StringUtils.h>
#include <Database/DBConnection.h>
#include "../../../../Utils/ServiceLocator.h"
#include "../../../../ECS/Components/Network/Authentication.h"

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

    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    Authentication& authentication = gameRegistry->get<Authentication>(static_cast<entt::entity>(client->GetIdentity()));
    authentication.username = logonChallenge.username;

    std::shared_ptr<ByteBuffer> sBuffer = ByteBuffer::Borrow<4>();
    std::shared_ptr<ByteBuffer> vBuffer = ByteBuffer::Borrow<256>();

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
        client->Close(asio::error::no_data);
        return true;
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
    if (!authentication.srp.StartVerification(authentication.username, logonChallenge.A))
    {
        NC_LOG_WARNING("Unsuccessful Login for: %s", authentication.username.c_str());
        client->Close(asio::error::no_data);
        return true;
    }

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<512>();
    ServerLogonChallenge serverChallenge;
    serverChallenge.status = 0;

    std::memcpy(serverChallenge.s, authentication.srp.saltBuffer->GetDataPointer(), authentication.srp.saltBuffer->Size);
    std::memcpy(serverChallenge.B, authentication.srp.bBuffer->GetDataPointer(), authentication.srp.bBuffer->Size);

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

    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    Authentication& authentication = gameRegistry->get<Authentication>(static_cast<entt::entity>(client->GetIdentity()));

    if (!authentication.srp.VerifySession(logonResponse.M1))
    {
        NC_LOG_WARNING("Unsuccessful Login for: %s", authentication.username.c_str());
        client->Close(asio::error::no_permission);
        return true;
    }

    NC_LOG_SUCCESS("Successful Login for: %s", authentication.username.c_str());

    ServerLogonResponse serverChallenge;
    std::memcpy(serverChallenge.HAMK, authentication.srp.HAMK, sizeof(authentication.srp.HAMK));

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<128>();
    buffer->PutU16(Opcode::SMSG_LOGON_RESPONSE);
    buffer->PutU16(0);

    u16 payloadSize = serverChallenge.Serialize(buffer);
    buffer->Put<u16>(payloadSize, 2);
    client->Send(buffer.get());

    return true;
}