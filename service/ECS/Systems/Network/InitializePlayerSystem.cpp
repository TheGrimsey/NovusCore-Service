#include "InitializePlayerSystem.h"
#include <memory>
#include <entt.hpp>
#include <tracy/Tracy.hpp>
#include <Utils/ByteBuffer.h>
#include <Networking/Opcode.h>
#include "../../../Utils/EntityUtils.h"
#include "../../Components/Transform.h"
#include "../../Components/GameEntityInfo.h"
#include "../../Components/Network/ConnectionComponent.h"
#include "../../Components/Network/InitializedConnection.h"
#include "../../Components/InitializeWorldState.h"
#include "../../Components/JustSpawned.h"

void InitializePlayerSystem::Update(entt::registry& registry)
{
    // This is a very naive approach, primarily due to the fact that we are currently limited to just sending 8192 bytes at a time.
    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<8192>();
    std::shared_ptr<ByteBuffer> initialConnectionBuffer = ByteBuffer::Borrow<8192>();

    auto initialConnectionView = registry.view<ConnectionComponent, InitializeWorldState, JustSpawned>();
    auto connectionView = registry.view<ConnectionComponent, InitializedConnection>();
    if (initialConnectionView.empty() && connectionView.empty())
        return;

    auto newEntityView = registry.view<GameEntityInfo, Transform, JustSpawned>();
    newEntityView.each([&registry, &buffer](const entt::entity& entity, GameEntityInfo& gameEntityInfo, Transform& transform, JustSpawned& justSpawned)
        {
            buffer->Put(Opcode::SMSG_CREATE_ENTITY);

            size_t sizeOffset = buffer->WrittenData;
            buffer->PutU16(0);

            u16 payloadSize = EntityUtils::Serialize(entity, gameEntityInfo, transform, buffer);
            buffer->Put(payloadSize, sizeOffset);

            transform.isDirty = false;
        });

    auto entitiesView = registry.view<GameEntityInfo, Transform>();
    entitiesView.each([&registry, &initialConnectionView, &connectionView, &buffer, &initialConnectionBuffer](const entt::entity& entity, GameEntityInfo& gameEntityInfo, Transform& transform)
        {
            if (!initialConnectionView.empty())
            {
                initialConnectionBuffer->Put(Opcode::SMSG_CREATE_ENTITY);

                size_t sizeOffset = initialConnectionBuffer->WrittenData;
                initialConnectionBuffer->PutU16(0);

                u16 payloadSize = EntityUtils::Serialize(entity, gameEntityInfo, transform, initialConnectionBuffer);
                initialConnectionBuffer->Put(payloadSize, sizeOffset);
            }

            if (!connectionView.empty())
            {
                if (transform.isDirty)
                {
                    buffer->Put(Opcode::SMSG_UPDATE_ENTITY);
                    buffer->PutU16(40);

                    buffer->Put(entity);
                    buffer->Put(transform.position);
                    buffer->Put(transform.rotation);
                    buffer->Put(transform.scale);

                    transform.isDirty = false;
                }
            }
        });

    if (!connectionView.empty() && buffer->WrittenData)
    {
        connectionView.each([&registry, &buffer](const entt::entity& entity, ConnectionComponent& connectionComponent, InitializedConnection&)
            {
                connectionComponent.connection->Send(buffer.get());
            });
    }

    if (!initialConnectionView.empty() && initialConnectionBuffer->WrittenData)
    {
        initialConnectionView.each([&registry, &initialConnectionBuffer](const entt::entity& entity, ConnectionComponent& connectionComponent, InitializeWorldState& initializeWorldState, JustSpawned& justSpawned)
            {
                connectionComponent.connection->Send(initialConnectionBuffer.get());
            });

        registry.assign<InitializedConnection>(initialConnectionView.begin(), initialConnectionView.end());
        registry.remove<InitializeWorldState, JustSpawned>(initialConnectionView.begin(), initialConnectionView.end());
    }

    if (!newEntityView.empty())
    {
        registry.remove<JustSpawned>(newEntityView.begin(), newEntityView.end());
    }
}