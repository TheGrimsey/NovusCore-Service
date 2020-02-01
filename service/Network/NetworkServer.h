#pragma once

#include <NovusTypes.h>
#include <asio.hpp>
#include "NetworkClient.h"

class NetworkServer
{
public:
    using tcp = asio::ip::tcp;
    NetworkServer(std::shared_ptr<asio::io_service> ioService, i16 port) : _ioService(ioService), _acceptor(*ioService.get(), tcp::endpoint(tcp::v4(), port)), _isRunning(false)
    {
        _clients.reserve(4096);
    }

    void Start();
    void Stop();
    void Listen();
    void Run();
    void HandleNewConnection(tcp::socket* socket, const asio::error_code& error);
    void AddConnection(std::shared_ptr<NetworkClient> client)
    {
        _clients.push_back(client);
        client->HandleConnect();
    }

    u16 GetPort() { return _acceptor.local_endpoint().port(); }
    bool IsRunning() { return _isRunning; }
private:
    std::shared_ptr<asio::io_service> _ioService;
    asio::ip::tcp::acceptor _acceptor;

    std::mutex _mutex;
    std::thread runThread;
    bool _isRunning;
    std::vector<std::shared_ptr<NetworkClient>> _clients;
};