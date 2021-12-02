#include "ServiceLocator.h"
#include <Networking/NetPacketHandler.h>

entt::registry* ServiceLocator::_registry = nullptr;
moodycamel::ConcurrentQueue<Message>* ServiceLocator::_inputQueue = nullptr;
NetPacketHandler* ServiceLocator::_netPacketHandler = nullptr;

void ServiceLocator::SetRegistry(entt::registry* registry)
{
    assert(_registry == nullptr);
    _registry = registry;
}
void ServiceLocator::SetNetPacketHandler(NetPacketHandler* netPacketHandler)
{
    assert(_netPacketHandler == nullptr);
    _netPacketHandler = netPacketHandler;
}
void ServiceLocator::SetInputQueue(moodycamel::ConcurrentQueue<Message>* input)
{
    assert(_inputQueue == nullptr);
    _inputQueue = input;
}