#include "GeneralHandlers.h"
#include "../../MessageHandler.h"
#include "Auth/AuthHandlers.h"

void Server::GeneralHandlers::Setup(MessageHandler* messageHandler)
{
    // Setup other handlers
    AuthHandlers::Setup(messageHandler);
}
