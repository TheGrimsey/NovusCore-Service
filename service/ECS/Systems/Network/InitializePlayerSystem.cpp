#include "InitializePlayerSystem.h"
#include <memory>
#include <entt.hpp>
#include <tracy/Tracy.hpp>
#include <Utils/ByteBuffer.h>
#include <Networking/Opcode.h>
#include "../../../Utils/EntityUtils.h"
#include "../../Components/Singletons/TimeSingleton.h"
#include "../../Components/Transform.h"
#include "../../Components/GameEntityInfo.h"
#include "../../Components/Network/ConnectionComponent.h"
#include "../../Components/Network/InitializedConnection.h"
#include "../../Components/InitializeWorldState.h"

void InitializePlayerSystem::Update(entt::registry& registry)
{
    f32 deltaTime = registry.ctx<TimeSingleton>().deltaTime;

    // This is a very naive approach, primarily due to the fact that we are currently limited to just sending 8192 bytes at a time.
    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<8192>();

    auto initialConnectionView = registry.view<ConnectionComponent, InitializeWorldState>();
    /*if (connectionView.empty())
        return;*/

    auto creatureView = registry.view<GameEntityInfo, Transform>();
    creatureView.each([&registry, &initialConnectionView, &buffer, deltaTime](const entt::entity& entity, GameEntityInfo& gameEntityInfo, Transform& transform)
        {
            if (transform.positionForward)
            {
                transform.position.x += 1 * deltaTime;
                transform.positionOffset += 1 * deltaTime;
                
                if (transform.positionOffset >= 10)
                {
                    transform.positionForward = false;
                }
            }
            else
            {
                transform.position.x -= 1 * deltaTime;
                transform.positionOffset -= 1 * deltaTime;

                if (transform.positionOffset <= 0)
                {
                    transform.positionForward = true;
                }
            }

            transform.rotation.x += 30 * deltaTime;
            if (transform.rotation.x >= 360)
            {
                transform.rotation.x -= 360;
            }

            if (transform.scaleForward)
            {
                transform.scale.x += 1 * deltaTime;
                transform.scaleOffset += 1 * deltaTime;

                if (transform.scaleOffset >= 2)
                {
                    transform.scaleForward = false;
                }
            }
            else
            {
                transform.scale.x -= 1 * deltaTime;
                transform.scaleOffset -= 1 * deltaTime;

                if (transform.scaleOffset <= 0)
                {
                    transform.scaleForward = true;
                }
            }
            
            transform.isDirty = true;

            if (initialConnectionView.empty())
            {
                buffer->PutU16(Opcode::SMSG_UPDATE_ENTITY);
                buffer->PutU16(40);

                buffer->Put(entity);
                buffer->Put(transform.position);
                buffer->Put(transform.rotation);
                buffer->Put(transform.scale);
            }
            else
            {
                buffer->PutU16(Opcode::SMSG_CREATE_ENTITY);

                size_t sizeOffset = buffer->WrittenData;
                buffer->PutU16(0);

                u16 payloadSize = EntityUtils::Serialize(entity, gameEntityInfo, transform, buffer);
                buffer->Put(payloadSize, sizeOffset);
            }
        });

    if (initialConnectionView.empty())
    {
        auto connectionView = registry.view<ConnectionComponent, InitializedConnection>();
        connectionView.each([&registry, &buffer](const entt::entity& entity, ConnectionComponent& connectionComponent, InitializedConnection&)
            {
                connectionComponent.connection->Send(buffer.get());
            });
    }
    else
    {
        initialConnectionView.each([&registry, &buffer](const entt::entity& entity, ConnectionComponent& connectionComponent, InitializeWorldState& initializeWorldState)
            {
                connectionComponent.connection->Send(buffer.get());
            });

        registry.assign<InitializedConnection>(initialConnectionView.begin(), initialConnectionView.end());
        registry.remove<InitializeWorldState>(initialConnectionView.begin(), initialConnectionView.end());
    }
}