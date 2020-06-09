#include "GeneralHandlers.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>
#include "Auth/AuthHandlers.h"
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/InitializeWorldState.h"

void Server::GeneralHandlers::Setup(MessageHandler* messageHandler)
{
    // Setup other handlers
    AuthHandlers::Setup(messageHandler); 
    messageHandler->SetMessageHandler(Opcode::CMSG_CONNECTED, Server::GeneralHandlers::CMSG_CONNECTED);
}

bool Server::GeneralHandlers::CMSG_CONNECTED(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    entt::entity entity = static_cast<entt::entity>(networkClient->GetIdentity());
    gameRegistry->assign<InitializeWorldState>(entity);

    return true;
}
