#include "GeneralHandlers.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>
#include "Auth/AuthHandlers.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../Utils/EntityUtils.h"
#include "../../../ECS/Components/Transform.h"
#include "../../../ECS/Components/GameEntityInfo.h"
#include "../../../ECS/Components/InitializeWorldState.h"
#include "../../../ECS/Components/JustSpawned.h"

void Server::GeneralHandlers::Setup(MessageHandler* messageHandler)
{
    // Setup other handlers
    AuthHandlers::Setup(messageHandler); 
    messageHandler->SetMessageHandler(Opcode::CMSG_CONNECTED, Server::GeneralHandlers::CMSG_CONNECTED);
    messageHandler->SetMessageHandler(Opcode::MSG_MOVE_ENTITY, Server::GeneralHandlers::MSG_MOVE_ENTITY);
    messageHandler->SetMessageHandler(Opcode::MSG_MOVE_HEARTBEAT_ENTITY, Server::GeneralHandlers::MSG_MOVE_HEARTBEAT_ENTITY);
    messageHandler->SetMessageHandler(Opcode::MSG_MOVE_STOP_ENTITY, Server::GeneralHandlers::MSG_MOVE_STOP_ENTITY);
}

bool Server::GeneralHandlers::CMSG_CONNECTED(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    entt::entity entity = static_cast<entt::entity>(networkClient->GetIdentity());

    Transform transform;
    transform.position = vec3(16533.3320f, 0.0f, 26133.3320f);
    transform.rotation = vec3(0, 0, 0);
    transform.scale = vec3(1, 1, 1);

    GameEntityInfo gameEntityInfo;
    gameEntityInfo.type = GameEntityType::PLAYER;

    std::shared_ptr<ByteBuffer> buffer = ByteBuffer::Borrow<128>();
    buffer->Put(Opcode::SMSG_CREATE_PLAYER);
    buffer->PutU16(0);

    u16 payloadSize = EntityUtils::Serialize(entity, gameEntityInfo, transform, buffer);
    buffer->Put(payloadSize, 2);
    networkClient->Send(buffer.get());

    gameRegistry->assign<InitializeWorldState>(entity);
    gameRegistry->assign<JustSpawned>(entity);
    gameRegistry->assign<Transform>(entity, transform);
    gameRegistry->assign<GameEntityInfo>(entity, gameEntityInfo);
    return true;
}

bool Server::GeneralHandlers::MSG_MOVE_ENTITY(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    entt::entity entity;
    MovementFlags moveFlags;
    vec3 position;
    vec3 rotation;

    packet->payload->Get(entity);
    packet->payload->Get(moveFlags);
    packet->payload->Get(position);
    packet->payload->Get(rotation);

    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    Transform& transform = gameRegistry->get<Transform>(entity);
    transform.position = position;
    transform.rotation = rotation;
    transform.moveFlags = moveFlags;
    transform.isDirty = true;

    return true;
}

bool Server::GeneralHandlers::MSG_MOVE_HEARTBEAT_ENTITY(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    entt::entity entity;
    MovementFlags moveFlags;
    vec3 position;
    vec3 rotation;

    packet->payload->Get(entity);
    packet->payload->Get(moveFlags);
    packet->payload->Get(position);
    packet->payload->Get(rotation);

    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    Transform& transform = gameRegistry->get<Transform>(entity);
    transform.position = position;
    transform.rotation = rotation;
    transform.moveFlags = moveFlags;
    transform.isDirty = true;

    return true;
}

bool Server::GeneralHandlers::MSG_MOVE_STOP_ENTITY(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    entt::entity entity;
    MovementFlags moveFlags;
    vec3 position;
    vec3 rotation;

    packet->payload->Get(entity);
    packet->payload->Get(moveFlags);
    packet->payload->Get(position);
    packet->payload->Get(rotation);

    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    Transform& transform = gameRegistry->get<Transform>(entity);
    transform.position = position;
    transform.rotation = rotation;
    transform.moveFlags = moveFlags;
    transform.isDirty = true;

    return true;
}
