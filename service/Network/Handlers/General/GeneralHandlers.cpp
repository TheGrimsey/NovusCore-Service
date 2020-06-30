#include "GeneralHandlers.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>
#include <Networking/AddressType.h>
#include <Networking/PacketUtils.h>
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Network/ConnectionComponent.h"
#include "../../../ECS/Components/Network/LoadBalanceSingleton.h"
#include "../../../ECS/Components/Network/HasServerInformation.h"

void Network::GeneralHandlers::Setup(MessageHandler* messageHandler)
{
    messageHandler->SetMessageHandler(Opcode::CMSG_CONNECTED, Network::GeneralHandlers::CMSG_CONNECTED);
    messageHandler->SetMessageHandler(Opcode::MSG_REQUEST_ADDRESS, Network::GeneralHandlers::MSG_REQUEST_ADDRESS);
    messageHandler->SetMessageHandler(Opcode::SMSG_SEND_ADDRESS, Network::GeneralHandlers::SMSG_SEND_ADDRESS);
    messageHandler->SetMessageHandler(Opcode::MSG_REQUEST_INTERNAL_SERVER_INFO, Network::GeneralHandlers::MSG_REQUEST_INTERNAL_SERVER_INFO);
}

bool Network::GeneralHandlers::CMSG_CONNECTED(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    if (client->GetStatus() != ConnectionStatus::AUTH_SUCCESS)
        return false;

    // (AddressType type) is used to identify what kind of server just connected to us
    AddressType type;
    u32 address;
    u16 port;
    if (!packet->payload->Get(type) ||
        (type < AddressType::AUTH || type >= AddressType::COUNT))
    {
        return false;
    }

    if (!packet->payload->GetU32(address))
        return false;

    if (!packet->payload->GetU16(port))
        return false;

    entt::registry* registry = ServiceLocator::GetRegistry();
    auto& loadBalanceSingleton = registry->ctx<LoadBalanceSingleton>();

    entt::entity entity = static_cast<entt::entity>(client->GetEntityId());
    HasServerInformation& hasServerInformation = registry->assign<HasServerInformation>(entity);

    hasServerInformation.entity = entity;
    hasServerInformation.type = type;

    ServerInformation serverInformation;
    serverInformation.entity = entity;
    serverInformation.type = type;
    serverInformation.address = address;
    serverInformation.port = port;

    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<8192>();
    buffer->Put(Opcode::SMSG_CONNECTED);
    buffer->PutU16(0);

    if (type == AddressType::AUTH)
    {
        loadBalanceSingleton.Add<AddressType::AUTH>(serverInformation);
    }
    else if (type == AddressType::REALM)
    {
        loadBalanceSingleton.Add<AddressType::REALM>(serverInformation);
    }
    else if (type == AddressType::WORLD)
    {
        loadBalanceSingleton.Add<AddressType::WORLD>(serverInformation);
    }
    else if (type == AddressType::INSTANCE)
    {
        loadBalanceSingleton.Add<AddressType::INSTANCE>(serverInformation);
    }
    else if (type == AddressType::CHAT)
    {
        loadBalanceSingleton.Add<AddressType::CHAT>(serverInformation);
    }
    else if (type == AddressType::LOADBALANCE)
    {
        loadBalanceSingleton.Add<AddressType::LOADBALANCE>(serverInformation);

        const std::vector<ServerInformation> serverInformations = loadBalanceSingleton.GetServerInformations();

        if (!PacketUtils::Write_SMSG_SEND_FULL_INTERNAL_SERVER_INFO(buffer, reinterpret_cast<const u8*>(serverInformations.data()), serverInformations.size()))
            return false;
    }
    else if (type == AddressType::REGION)
    {
        loadBalanceSingleton.Add<AddressType::REGION>(serverInformation);
    }

    if (type != AddressType::LOADBALANCE)
    {
        auto& loadBalancers = loadBalanceSingleton.GetLoadBalancers();
        if (loadBalancers.size() > 0)
        {
            std::shared_ptr<Bytebuffer> bufferAddServer = Bytebuffer::Borrow<128>();
            if (!PacketUtils::Write_SMSG_SEND_ADD_INTERNAL_SERVER_INFO(bufferAddServer, serverInformation.entity, serverInformation.type, serverInformation.address, serverInformation.port))
                return false;

            for (const ServerInformation& serverInformation : loadBalancers)
            {
                ConnectionComponent& serverConnection = registry->get<ConnectionComponent>(serverInformation.entity);
                serverConnection.AddPacket(bufferAddServer);
            }
        }
    }

    client->Send(buffer.get());
    client->SetStatus(ConnectionStatus::CONNECTED);
    return true;
}

bool Network::GeneralHandlers::MSG_REQUEST_ADDRESS(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    if (client->GetStatus() != ConnectionStatus::CONNECTED)
        return false;

    AddressType type;
    if (!packet->payload->Get(type) ||
        (type < AddressType::AUTH || type >= AddressType::COUNT))
    {
        return false;
    }

    entt::registry* registry = ServiceLocator::GetRegistry();
    auto& loadBalanceSingleton = registry->ctx<LoadBalanceSingleton>();

    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
    ServerInformation serverInformation;

    u16 packetSize = 0;
    if (loadBalanceSingleton.Get<AddressType::LOADBALANCE>(serverInformation))
    {
        if (!PacketUtils::Write_MSG_REQUEST_ADDRESS(buffer, type, static_cast<entt::entity>(client->GetEntityId()), packet->payload->GetReadPointer(), packet->payload->GetReadSpace()))
            return false;
    }
    else
    {
        if (!PacketUtils::Write_SMSG_SEND_ADDRESS(buffer, 0, 0, 0, packet->payload->GetReadPointer(), packet->payload->GetReadSpace()))
            return false;
    }
    
    ConnectionComponent& loadBalancerConnection = registry->get<ConnectionComponent>(serverInformation.entity);
    loadBalancerConnection.AddPacket(buffer);

    return true;
}

bool Network::GeneralHandlers::SMSG_SEND_ADDRESS(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    if (client->GetStatus() != ConnectionStatus::CONNECTED)
        return false;

    u8 status = 0;
    u32 address = 0;
    u16 port = 0;
    entt::entity entity = entt::null;

    if (!packet->payload->GetU8(status))
        return false;

    if (status == 1)
    {
        if (!packet->payload->GetU32(address))
            return false;

        if (!packet->payload->GetU16(port))
            return false;
    }

    if (!packet->payload->Get(entity))
        return false;

    // Forward Packet
    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
    if (!PacketUtils::Write_SMSG_SEND_ADDRESS(buffer, status, address, port, packet->payload->GetReadPointer(), packet->payload->GetReadSpace()))
        return false;

    entt::registry* registry = ServiceLocator::GetRegistry();
    ConnectionComponent& connection = registry->get<ConnectionComponent>(entity);
    connection.AddPacket(buffer);
    return true;
}

bool Network::GeneralHandlers::MSG_REQUEST_INTERNAL_SERVER_INFO(std::shared_ptr<NetworkClient> client, NetworkPacket* packet)
{
    if (client->GetStatus() != ConnectionStatus::CONNECTED)
        return false;
    
    entt::registry* registry = ServiceLocator::GetRegistry();
    LoadBalanceSingleton& loadBalanceSingleton = registry->ctx<LoadBalanceSingleton>();
    const std::vector<ServerInformation> serverInformations = loadBalanceSingleton.GetServerInformations();

    // Forward Packet
    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<8192>();

    buffer->Put(Opcode::SMSG_SEND_FULL_INTERNAL_SERVER_INFO);
    buffer->SkipWrite(sizeof(u16));
    size_t startWrittenData = buffer->writtenData;

    for (const ServerInformation& serverInformation : serverInformations)
    {
        buffer->PutEnttId(serverInformation.entity);
        buffer->Put(serverInformation.type);
        buffer->PutU32(serverInformation.address);
        buffer->PutU16(serverInformation.port);
    }

    u16 packetSize = static_cast<u16>(buffer->writtenData - startWrittenData);
    buffer->Put(packetSize, 2);
    client->Send(buffer.get());

    return true;
}