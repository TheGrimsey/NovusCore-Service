#pragma once
#include "NetworkClient.h"
#include <NovusTypes.h>
#include <Utils/Message.h>
#include <Utils/DebugHandler.h>
#include "NetworkPacket.h"
#include "../Utils/ServiceLocator.h"
#include "../ECS/Components/Network/ConnectionComponent.h"
#include "../ECS/Components/Network/ConnectionDeferredSingleton.h"

void NetworkClient::Listen()
{
    _baseSocket->AsyncRead();
}
bool NetworkClient::Connect(tcp::endpoint endpoint)
{
    try
    {
        _baseSocket->socket()->connect(endpoint);
    }
    catch (std::exception e)
    {
        return false;
    }
    _baseSocket->AsyncRead();
    return true;
}
bool NetworkClient::Connect(std::string address, u16 port)
{
    return Connect(tcp::endpoint(asio::ip::address::from_string(address), port));
}
void NetworkClient::HandleConnect()
{
    Listen();
}
void NetworkClient::HandleDisconnect()
{
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();
    gameRegistry->ctx<ConnectionDeferredSingleton>().droppedConnectionQueue.enqueue(_identity);
}
void NetworkClient::HandleRead()
{
    std::shared_ptr<ByteBuffer> buffer = _baseSocket->GetReceiveBuffer();
    entt::registry* gameRegistry = ServiceLocator::GetGameRegistry();

    entt::entity entity = static_cast<entt::entity>(_identity);
    ConnectionComponent& connectionComponent = gameRegistry->get<ConnectionComponent>(entity);

    while (buffer->GetActiveSize())
    {
        u16 opcode = 0;
        u16 size = 0;

        buffer->GetU16(opcode);
        buffer->GetU16(size);

        if (size > NETWORK_BUFFER_SIZE)
        {
            _baseSocket->Close(asio::error::shut_down);
            return;
        }

        NetworkPacket* packet = new NetworkPacket();
        {
            // Header
            {
                packet->header.opcode = opcode;
                packet->header.size = size;
            }

            // Payload
            {
                if (size)
                {
                    packet->payload = ByteBuffer::Borrow<NETWORK_BUFFER_SIZE>();
                    packet->payload->Size = size;
                    packet->payload->WrittenData = size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), size);
                }
            }

            connectionComponent.packetQueue.enqueue(packet);
        }

        buffer->ReadData += size;
    }

    _baseSocket->AsyncRead();
}