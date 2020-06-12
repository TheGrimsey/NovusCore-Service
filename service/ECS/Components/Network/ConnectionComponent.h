#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>

enum class PacketPriority
{
    LOW,
    MEDIUM,
    HIGH
};

#define LOW_PRIORITY_TIME 1
#define MEDIUM_PRIORITY_TIME 0.5f

struct ConnectionComponent
{
    ConnectionComponent() : packetQueue(256) 
    { 
        lowPriorityBuffer = ByteBuffer::Borrow<8192>();
        mediumPriorityBuffer = ByteBuffer::Borrow<8192>();
        highPriorityBuffer = ByteBuffer::Borrow<8192>();
    }

    std::shared_ptr<NetworkClient> connection;
    moodycamel::ConcurrentQueue<NetworkPacket*> packetQueue;

    void AddPacket(std::shared_ptr<ByteBuffer> buffer, PacketPriority priority = PacketPriority::MEDIUM)
    {
        assert(buffer->WrittenData <= 8192);

        std::shared_ptr<ByteBuffer> bufferToUse = nullptr;
        if (priority == PacketPriority::LOW)
        {
            bufferToUse = lowPriorityBuffer;
        }
        else if (priority == PacketPriority::MEDIUM)
        {
            bufferToUse = mediumPriorityBuffer;
        }
        else if (priority == PacketPriority::HIGH)
        {
            bufferToUse = highPriorityBuffer;
        }

        // Should we send priority buffer's content before we add this due to size constraints
        if (bufferToUse->GetRemainingSpace() < bufferToUse->WrittenData)
        {
            connection->Send(bufferToUse.get());
            bufferToUse->Reset();

            if (bufferToUse == lowPriorityBuffer)
                lowPriorityTimer = 0;
            else if (bufferToUse == mediumPriorityBuffer)
                mediumPriorityTimer = 0;
        }
        
        memcpy(bufferToUse->GetWritePointer(), buffer->GetDataPointer(), buffer->WrittenData);
        bufferToUse->WrittenData += buffer->WrittenData;
    }

    // The reason we are not using the Timer class is due to performance reasons (Timer creates a timepoint every call to GetLifetime())
    f32 lowPriorityTimer = 0;
    f32 mediumPriorityTimer = 0;
    std::shared_ptr<ByteBuffer> lowPriorityBuffer;
    std::shared_ptr<ByteBuffer> mediumPriorityBuffer;
    std::shared_ptr<ByteBuffer> highPriorityBuffer;
};