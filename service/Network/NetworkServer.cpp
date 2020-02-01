#include "NetworkServer.h"

#include <Utils/Timer.h>
#include <Utils/Message.h>
#include "../Utils/ServiceLocator.h"
#include "../ECS/Components/Network/ConnectionDeferredSingleton.h"

void NetworkServer::Start()
{
    if (_isRunning)
        return;

    _isRunning = true;
    runThread = std::thread(&NetworkServer::Run, this);

    Listen();
}
void NetworkServer::Stop()
{
    if (!_isRunning)
        return;

    _isRunning = false;
    _acceptor.close();
}
void NetworkServer::Listen()
{
    tcp::socket* socket = new tcp::socket(*_ioService.get());
    _acceptor.async_accept(*socket, std::bind(&NetworkServer::HandleNewConnection, this, socket, std::placeholders::_1));
}
void NetworkServer::Run()
{
    const f32 targetDelta = 5;

    Timer timer;
    while (_isRunning)
    {
        timer.Tick();

        _mutex.lock();
        if (_clients.size())
        {
            _clients.erase(
                std::remove_if(_clients.begin(), _clients.end(), [](std::shared_ptr<NetworkClient>& client)
                    {
                        return client->GetBaseSocket()->IsClosed();
                    }),
                _clients.end());
        }
        _mutex.unlock();

        // This sacrifices percision for performance, but we don't need precision here
        f32 deltaTime = timer.GetDeltaTime();
        if (deltaTime <= targetDelta)
        {
            i32 timeToSleep = Math::FloorToInt(targetDelta - deltaTime);
            std::this_thread::sleep_for(std::chrono::seconds(timeToSleep));
        }
    }
}
void NetworkServer::HandleNewConnection(tcp::socket* socket, const asio::error_code& error)
{
    if (!error)
    {
        _mutex.lock();
        {
            socket->non_blocking(true);
            socket->set_option(asio::socket_base::send_buffer_size(4096));
            socket->set_option(asio::socket_base::receive_buffer_size(4096));
            socket->set_option(tcp::no_delay(true));

            entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
            gameRegistry->ctx<ConnectionDeferredSingleton>().newConnectionQueue.enqueue(socket);
        }
        _mutex.unlock();
    }

    Listen();
}