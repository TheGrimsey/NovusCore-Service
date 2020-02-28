#include "ConnectionSystems.h"
#include "../../Components/Network/ConnectionComponent.h"
#include "../../Components/Network/ConnectionDeferredSingleton.h"
#include "../../../Network/MessageHandler.h"
#include "../../../Network/NetworkServer.h"
#include "../../../Utils/ServiceLocator.h"
#include <tracy/Tracy.hpp>

void ConnectionUpdateSystem::Update(entt::registry& registry)
{
    MessageHandler* messageHandler = ServiceLocator::GetNetworkMessageHandler();
    auto view = registry.view<ConnectionComponent>();
    view.each([&registry, &messageHandler](const auto, ConnectionComponent& connectionComponent)
    {
            ZoneScopedNC("InternalPacketHandlerSystem::Update", tracy::Color::Blue)

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

void ConnectionDeferredSystem::Update(entt::registry& registry)
{
    ConnectionDeferredSingleton& connectionDeferredSingleton = registry.ctx<ConnectionDeferredSingleton>();

    asio::ip::tcp::socket* socket;
    while (connectionDeferredSingleton.newConnectionQueue.try_dequeue(socket))
    {
        entt::entity entity = registry.create();

        ConnectionComponent* connectionComponent = &registry.assign<ConnectionComponent>(entity);
        connectionComponent->connection = std::make_shared<NetworkClient>(socket, entt::to_integer(entity));

        connectionDeferredSingleton.networkServer->AddConnection(connectionComponent->connection);
    }

    u64 entityid;
    while (connectionDeferredSingleton.droppedConnectionQueue.try_dequeue(entityid))
    {
        entt::entity entity = static_cast<entt::entity>(entityid);
        registry.destroy(entity);
    }
}