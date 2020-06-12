#include "ConnectionSystems.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkServer.h>
#include "../../Components/Network/Authentication.h"
#include "../../Components/Network/ConnectionComponent.h"
#include "../../Components/Network/ConnectionDeferredSingleton.h"
#include "../../Components/Network/InitializedConnection.h"
#include "../../../Utils/ServiceLocator.h"
#include <tracy/Tracy.hpp>

void ConnectionUpdateSystem::Update(entt::registry& registry)
{
    ZoneScopedNC("ConnectionUpdateSystem::Update", tracy::Color::Blue)

    MessageHandler* messageHandler = ServiceLocator::GetNetworkMessageHandler();
    auto view = registry.view<ConnectionComponent>();
    view.each([&registry, &messageHandler](const auto, ConnectionComponent& connectionComponent)
    {

                NetworkPacket* packet;
            while (connectionComponent.packetQueue.try_dequeue(packet))
            {
                if (!messageHandler->CallHandler(connectionComponent.connection, packet))
                {
                    connectionComponent.packetQueue.enqueue(packet);
                    continue;
                }

                delete packet;

                // We might close the socket during a handler
                if (connectionComponent.connection->IsClosed())
                    break;
            }
    });
}

void ConnectionUpdateSystem::HandleConnection(NetworkServer* server, asio::ip::tcp::socket* socket, const asio::error_code& error)
{
    if (!error)
    {
        socket->non_blocking(true);
        socket->set_option(asio::socket_base::send_buffer_size(NETWORK_BUFFER_SIZE));
        socket->set_option(asio::socket_base::receive_buffer_size(NETWORK_BUFFER_SIZE));
        socket->set_option(asio::ip::tcp::no_delay(true));

        entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
        gameRegistry->ctx<ConnectionDeferredSingleton>().newConnectionQueue.enqueue(socket);
    }
}

void ConnectionUpdateSystem::HandleRead(BaseSocket* socket)
{
    NetworkClient* client = static_cast<NetworkClient*>(socket);

    std::shared_ptr<ByteBuffer> buffer = client->GetReceiveBuffer();
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();

    entt::entity entity = static_cast<entt::entity>(client->GetIdentity());
    ConnectionComponent& connectionComponent = gameRegistry->get<ConnectionComponent>(entity);

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

        NetworkPacket* packet = new NetworkPacket();
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
                    packet->payload = ByteBuffer::Borrow<NETWORK_BUFFER_SIZE>();
                    packet->payload->Size = size;
                    packet->payload->WrittenData = size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), size);
                }
            }

            connectionComponent.packetQueue.enqueue(packet);
        }

        buffer->ReadData += size;
    }

    client->Listen();
}

void ConnectionUpdateSystem::HandleDisconnect(BaseSocket* socket)
{
    NetworkClient* client = static_cast<NetworkClient*>(socket);

    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    gameRegistry->ctx<ConnectionDeferredSingleton>().droppedConnectionQueue.enqueue(client->GetIdentity());
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

            ConnectionComponent* connectionComponent = &registry.assign<ConnectionComponent>(entity);
            connectionComponent->connection = std::make_shared<NetworkClient>(socket, entt::to_integral(entity));

            Authentication* authentication = &registry.assign<Authentication>(entity);

            connectionComponent->connection->SetReadHandler(std::bind(&ConnectionUpdateSystem::HandleRead, std::placeholders::_1));
            connectionComponent->connection->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::HandleDisconnect, std::placeholders::_1));
            connectionComponent->connection->Listen();

            connectionDeferredSingleton.networkServer->AddConnection(connectionComponent->connection);
        }
    }

    if (connectionDeferredSingleton.droppedConnectionQueue.size_approx() > 0)
    {
        std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<8192>();
        u64 entityid;
        while (connectionDeferredSingleton.droppedConnectionQueue.try_dequeue(entityid))
        {
            entt::entity entity = static_cast<entt::entity>(entityid);

            buffer->Put(Opcode::SMSG_DELETE_ENTITY);
            buffer->PutU16(sizeof(ENTT_ID_TYPE));
            buffer->Put(entity);

            registry.destroy(entity);
        }

        auto connectionView = registry.view<ConnectionComponent, InitializedConnection>();
        if (connectionView.size() > 0)
        {
            if (buffer->WrittenData)
            {
                connectionView.each([&registry, &buffer](const entt::entity& entity, ConnectionComponent& connectionComponent, InitializedConnection&)
                    {
                        connectionComponent.connection->Send(buffer.get());
                    });
            }
        }
    }
}