#include "ServiceLocator.h"
#include <Networking/MessageHandler.h>

entt::registry* ServiceLocator::_registry = nullptr;
moodycamel::ConcurrentQueue<Message>* ServiceLocator::_inputQueue = nullptr;
MessageHandler* ServiceLocator::_networkMessageHandler = nullptr;

void ServiceLocator::SetRegistry(entt::registry* registry)
{
    assert(_registry == nullptr);
    _registry = registry;
}
void ServiceLocator::SetNetworkMessageHandler(MessageHandler* networkMessageHandler)
{
    assert(_networkMessageHandler == nullptr);
    _networkMessageHandler = networkMessageHandler;
}
void ServiceLocator::SetInputQueue(moodycamel::ConcurrentQueue<Message>* input)
{
    assert(_networkMessageHandler == nullptr);
    _inputQueue = input;
}