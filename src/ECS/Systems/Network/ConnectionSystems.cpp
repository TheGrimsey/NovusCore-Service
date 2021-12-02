#include "ConnectionSystems.h"
#include <entt.hpp>
#include <Networking/NetClient.h>
#include <Networking/NetServer.h>
#include <Networking/PacketUtils.h>
#include <Networking/NetPacketHandler.h>
#include <Utils/DebugHandler.h>
#include "../../Components/Singletons/TimeSingleton.h"
#include "../../Components/Network/ConnectionDeferredSingleton.h"
#include "../../Components/Network/LoadBalanceSingleton.h"
#include "../../Components/Network/Authentication.h"
#include "../../Components/Network/ConnectionComponent.h"
#include "../../Components/Network/InitializedConnection.h"
#include "../../Components/Network/HasServerInformation.h"
#include "../../../Utils/ServiceLocator.h"
#include <tracy/Tracy.hpp>

void ConnectionUpdateSystem::Update(entt::registry& registry)
{
    ZoneScopedNC("ConnectionUpdateSystem::Update", tracy::Color::Blue)

    TimeSingleton& timeSingleton = registry.ctx<TimeSingleton>();
    f32 deltaTime = timeSingleton.deltaTime;

    NetPacketHandler* netPacketHandler = ServiceLocator::GetNetPacketHandler();
    auto view = registry.view<ConnectionComponent>();
    view.each([&registry, &netPacketHandler, deltaTime](const auto, ConnectionComponent& connection)
    {
        if (connection.netClient->Read())
        {
            HandleRead(connection.netClient);
        }

        if (!connection.netClient->IsConnected())
        {
            HandleDisconnect(connection.netClient);
            return;
        }

        std::shared_ptr<NetPacket> packet = nullptr;
        while (connection.packetQueue.try_dequeue(packet))
        {
#ifdef NC_Debug
             DebugHandler::PrintSuccess("[Network/Socket]: CMD: %u, Size: %u", packet->header.opcode, packet->header.size);
#endif // NC_Debug

            if (!netPacketHandler->CallHandler(connection.netClient, packet))
            {
                connection.netClient->Close();
                return;
            }
        }

        if (connection.lowPriorityBuffer->writtenData)
        {
            connection.lowPriorityTimer += deltaTime;
            if (connection.lowPriorityTimer >= LOW_PRIORITY_TIME)
            {
                connection.lowPriorityTimer = 0;
                connection.netClient->Send(connection.lowPriorityBuffer);
                connection.lowPriorityBuffer->Reset();
            }
        }
        if (connection.mediumPriorityBuffer->writtenData)
        {
            connection.mediumPriorityTimer += deltaTime;
            if (connection.mediumPriorityTimer >= MEDIUM_PRIORITY_TIME)
            {
                connection.mediumPriorityTimer = 0;
                connection.netClient->Send(connection.mediumPriorityBuffer);
                connection.mediumPriorityBuffer->Reset();
            }
        }
        if (connection.highPriorityBuffer->writtenData)
        {
            connection.netClient->Send(connection.highPriorityBuffer);
            connection.highPriorityBuffer->Reset();
        }
    });
}

bool ConnectionUpdateSystem::HandleConnection(std::shared_ptr<NetClient> netClient)
{
#ifdef NC_Debug
    const NetSocket::ConnectionInfo& connectionInfo = netClient->GetSocket()->GetConnectionInfo();
    DebugHandler::PrintSuccess("[Network/Socket]: Client connected from (%s, %u)", connectionInfo.ipAddrStr.c_str(), connectionInfo.port);
#endif // NC_Debug

    std::shared_ptr<NetSocket> netSocket = netClient->GetSocket();
    netSocket->SetBlockingState(false);
    netSocket->SetNoDelayState(true);
    netSocket->SetReceiveBufferSize(8192);
    netSocket->SetSendBufferSize(8192);

    entt::registry* registry = ServiceLocator::GetRegistry();
    auto& connectionDeferredSingleton = registry->ctx<ConnectionDeferredSingleton>();
    connectionDeferredSingleton.newConnectionQueue.enqueue(netClient);

    return true;
}

void ConnectionUpdateSystem::HandleRead(std::shared_ptr<NetClient> netClient)
{
    std::shared_ptr<Bytebuffer> buffer = netClient->GetReadBuffer();
    entt::registry* registry = ServiceLocator::GetRegistry();

    const entt::entity& entity = netClient->GetEntity();
    ConnectionComponent& connectionComponent = registry->get<ConnectionComponent>(entity);

    while (size_t activeSize = buffer->GetActiveSize())
    {
        // We have received a partial header and need to read more
        if (activeSize < sizeof(PacketHeader))
        {
            buffer->Normalize();
            break;
        }

        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer->GetReadPointer());

        if (header->opcode == Opcode::INVALID || header->opcode > Opcode::MAX_COUNT)
        {
#ifdef NC_Debug
            DebugHandler::PrintError("Received Invalid Opcode (%u) from network stream", static_cast<u16>(header->opcode));
#endif // NC_Debug
            break;
        }

        if (header->size > 8192)
        {
#ifdef NC_Debug
            DebugHandler::PrintError("Received Invalid Opcode Size (%u) from network stream", header->size);
#endif // NC_Debug
            break;
        }
        size_t sizeWithoutHeader = activeSize - sizeof(PacketHeader);

        // We have received a valid header, but we have yet to receive the entire payload
        if (sizeWithoutHeader < header->size)
        {
            buffer->Normalize();
            break;
        }
        
        // Skip Header
        buffer->SkipRead(sizeof(PacketHeader));

        std::shared_ptr<NetPacket> packet = NetPacket::Borrow();
        {
            // Header
            {
                packet->header = *header;
            }

            // Payload
            {
                if (packet->header.size)
                {
                    packet->payload = Bytebuffer::Borrow<8192/*NETWORK_BUFFER_SIZE*/ >();
                    packet->payload->size = packet->header.size;
                    packet->payload->writtenData = packet->header.size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), packet->header.size);

                    // Skip Payload
                    buffer->SkipRead(header->size);
                }
            }

            connectionComponent.packetQueue.enqueue(packet);
        }
    }

    // Only reset if we read everything that was written
    if (buffer->GetActiveSize() == 0)
    {
        buffer->Reset();
    }
}

void ConnectionUpdateSystem::HandleDisconnect(std::shared_ptr<NetClient> netClient)
{
#ifdef NC_Debug
    const NetSocket::ConnectionInfo& connectionInfo = netClient->GetSocket()->GetConnectionInfo();
    DebugHandler::PrintWarning("[Network/Socket]: Client disconnected from (%s, %u)", connectionInfo.ipAddrStr.c_str(), connectionInfo.port);
#endif // NC_Debug

    entt::registry* registry = ServiceLocator::GetRegistry();
    auto& connectionDeferredSingleton = registry->ctx<ConnectionDeferredSingleton>();

    entt::entity entity = netClient->GetEntity();
    connectionDeferredSingleton.droppedConnectionQueue.enqueue(entity);
}

void ConnectionDeferredSystem::Update(entt::registry& registry)
{
    ConnectionDeferredSingleton& connectionDeferredSingleton = registry.ctx<ConnectionDeferredSingleton>();

    if (connectionDeferredSingleton.newConnectionQueue.size_approx() > 0)
    {
        std::shared_ptr<NetClient> netClient;
        while (connectionDeferredSingleton.newConnectionQueue.try_dequeue(netClient))
        {
            entt::entity entity = registry.create();

            ConnectionComponent& connectionComponent = registry.emplace<ConnectionComponent>(entity);
            connectionComponent.netClient = netClient;

            Authentication& authentication = registry.emplace<Authentication>(entity);

            connectionComponent.netClient->SetEntity(entity);
            connectionComponent.netClient->SetConnectionStatus(ConnectionStatus::AUTH_CHALLENGE);
            //connectionComponent.connection->SetReadHandler(std::bind(&ConnectionUpdateSystem::HandleRead, std::placeholders::_1));
            //connectionComponent.connection->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::HandleDisconnect, std::placeholders::_1));

            //connectionDeferredSingleton.networkServer->AddConnection(connectionComponent.connection);
        }
    }

    if (connectionDeferredSingleton.droppedConnectionQueue.size_approx() > 0)
    {
        entt::entity entity;
        while (connectionDeferredSingleton.droppedConnectionQueue.try_dequeue(entity))
        {
            if (registry.all_of<HasServerInformation>(entity))
            {
                LoadBalanceSingleton& loadBalanceSingleton = registry.ctx<LoadBalanceSingleton>();
                HasServerInformation& hasServerInformation = registry.get<HasServerInformation>(entity);

                loadBalanceSingleton.Remove(hasServerInformation.type, hasServerInformation.entity);

                auto& loadBalancers = loadBalanceSingleton.GetLoadBalancers();
                if (loadBalancers.size() > 0)
                {
                    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
                    if (PacketUtils::Write_SMSG_SEND_REMOVE_INTERNAL_SERVER_INFO(buffer, hasServerInformation.entity, hasServerInformation.type, hasServerInformation.realmId))
                    {
                        for (const ServerInformation& serverInformation : loadBalancers)
                        {
                            if (entity == serverInformation.entity)
                                continue;

                            ConnectionComponent& serverConnection = registry.get<ConnectionComponent>(serverInformation.entity);
                            serverConnection.AddPacket(buffer);
                        }
                    }
                    else
                    {
                        assert(false);
                    }
                }
            }

            registry.destroy(entity);
        }
    }
}