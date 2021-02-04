#include "ConnectionSystems.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkServer.h>
#include <Networking/PacketUtils.h>
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

    MessageHandler* messageHandler = ServiceLocator::GetNetworkMessageHandler();
    auto view = registry.view<ConnectionComponent>();
    view.each([&registry, &messageHandler, deltaTime](const auto, ConnectionComponent& connection)
    {
                std::shared_ptr<NetworkPacket> packet;
            while (connection.packetQueue.try_dequeue(packet))
            {
#ifdef NC_Debug
                DebugHandler::PrintSuccess("[Network/Socket]: CMD: %u, Size: %u", packet->header.opcode, packet->header.size);
#endif // NC_Debug

                if (!messageHandler->CallHandler(connection.connection, packet))
                {
                    connection.connection->Close(asio::error::shut_down);
                    return;
                }
            }

            if (connection.lowPriorityBuffer->writtenData)
            {
                connection.lowPriorityTimer += deltaTime;
                if (connection.lowPriorityTimer >= LOW_PRIORITY_TIME)
                {
                    connection.lowPriorityTimer = 0;
                    connection.connection->Send(connection.lowPriorityBuffer);
                    connection.lowPriorityBuffer->Reset();
                }
            }
            if (connection.mediumPriorityBuffer->writtenData)
            {
                connection.mediumPriorityTimer += deltaTime;
                if (connection.mediumPriorityTimer >= MEDIUM_PRIORITY_TIME)
                {
                    connection.mediumPriorityTimer = 0;
                    connection.connection->Send(connection.mediumPriorityBuffer);
                    connection.mediumPriorityBuffer->Reset();
                }
            }
            if (connection.highPriorityBuffer->writtenData)
            {
                connection.connection->Send(connection.highPriorityBuffer);
                connection.highPriorityBuffer->Reset();
            }
    });
}

void ConnectionUpdateSystem::HandleConnection(NetworkServer* internalServer, asio::ip::tcp::socket* socket, const asio::error_code& error)
{
    if (!error)
    {
#ifdef NC_Debug
        DebugHandler::PrintSuccess("[Network/Socket]: Client connected from (%s)", socket->remote_endpoint().address().to_string().c_str());
#endif // NC_Debug

        socket->non_blocking(true);
        socket->set_option(asio::socket_base::send_buffer_size(NETWORK_BUFFER_SIZE));
        socket->set_option(asio::socket_base::receive_buffer_size(NETWORK_BUFFER_SIZE));
        socket->set_option(asio::ip::tcp::no_delay(true));

        entt::registry* registry = ServiceLocator::GetRegistry();
        auto& connectionDeferredSingleton = registry->ctx<ConnectionDeferredSingleton>();
        connectionDeferredSingleton.newConnectionQueue.enqueue(socket);
    }
}

void ConnectionUpdateSystem::HandleRead(BaseSocket* socket)
{
    NetworkClient* client = static_cast<NetworkClient*>(socket);

    std::shared_ptr<Bytebuffer> buffer = client->GetReceiveBuffer();
    entt::registry* registry = ServiceLocator::GetRegistry();

    entt::entity entity = static_cast<entt::entity>(client->GetEntityId());
    ConnectionComponent& connectionComponent = registry->get<ConnectionComponent>(entity);

    while (buffer->GetActiveSize())
    {
        Opcode opcode = Opcode::INVALID;
        u16 size = 0;

        buffer->Get(opcode);
        buffer->GetU16(size);

        if (size > NETWORK_BUFFER_SIZE)
        {
            client->Close(asio::error::shut_down);
            return;
        }

        std::shared_ptr<NetworkPacket> packet = NetworkPacket::Borrow();
        {
            // Header
            {
                packet->header.opcode = opcode;
                packet->header.size = size;
            }

            // Payload
            {
                if (size)
                {
                    packet->payload = Bytebuffer::Borrow<NETWORK_BUFFER_SIZE>();
                    packet->payload->size = size;
                    packet->payload->writtenData = size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), size);
                }
            }

            connectionComponent.packetQueue.enqueue(packet);
        }

        buffer->readData += size;
    }

    client->Listen();
}

void ConnectionUpdateSystem::HandleDisconnect(BaseSocket* socket)
{
#ifdef NC_Debug
    DebugHandler::PrintWarning("[Network/Socket]: Client disconnected from (%s)", socket->socket()->remote_endpoint().address().to_string().c_str());
#endif // NC_Debug

    entt::registry* registry = ServiceLocator::GetRegistry();
    auto& connectionDeferredSingleton = registry->ctx<ConnectionDeferredSingleton>();

    NetworkClient* client = static_cast<NetworkClient*>(socket);
    entt::entity entity = static_cast<entt::entity>(client->GetEntityId());

    connectionDeferredSingleton.droppedConnectionQueue.enqueue(entity);
}

void ConnectionDeferredSystem::Update(entt::registry& registry)
{
    ConnectionDeferredSingleton& connectionDeferredSingleton = registry.ctx<ConnectionDeferredSingleton>();

    if (connectionDeferredSingleton.newConnectionQueue.size_approx() > 0)
    {
        asio::ip::tcp::socket* socket;
        while (connectionDeferredSingleton.newConnectionQueue.try_dequeue(socket))
        {
            entt::entity entity = registry.create();

            ConnectionComponent& connectionComponent = registry.emplace<ConnectionComponent>(entity);
            connectionComponent.connection = std::make_shared<NetworkClient>(socket, entt::to_integral(entity));

            Authentication& authentication = registry.emplace<Authentication>(entity);

            connectionComponent.connection->SetStatus(ConnectionStatus::AUTH_CHALLENGE);
            connectionComponent.connection->SetReadHandler(std::bind(&ConnectionUpdateSystem::HandleRead, std::placeholders::_1));
            connectionComponent.connection->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::HandleDisconnect, std::placeholders::_1));
            connectionComponent.connection->Listen();

            connectionDeferredSingleton.networkServer->AddConnection(connectionComponent.connection);
        }
    }

    if (connectionDeferredSingleton.droppedConnectionQueue.size_approx() > 0)
    {
        entt::entity entity;
        while (connectionDeferredSingleton.droppedConnectionQueue.try_dequeue(entity))
        {
            if (registry.has<HasServerInformation>(entity))
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