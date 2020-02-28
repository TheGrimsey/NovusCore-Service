#pragma once
#include <Networking/BaseSocket.h>
#include <Utils/DebugHandler.h>

enum BuildType
{
    Internal,
    Alpha,
    Beta,
    Release
};

// Align data perfectly
#pragma pack(push, 1)
struct ClientLogonChallenge
{
    u8 majorVersion;
    u8 patchVersion;
    u8 minorVersion;
    u8 buildType; // 0 Internal, 1 Alpha, 2 Beta, 3 Release
    u16 gameBuild;
    std::string gameName;
    std::string username;

    std::string BuildTypeString()
    {
        std::string ret;

        switch (buildType)
        {
        case BuildType::Internal:
            ret = "Internal";
            break;
        case BuildType::Alpha:
            ret = "Alpha";
            break;
        case BuildType::Beta:
            ret = "Beta";
            break;
        case BuildType::Release:
            ret = "Release";
            break;
        }


        return ret;
    }

    void Deserialize(std::shared_ptr<ByteBuffer> buffer)
    {
        buffer->GetU8(majorVersion);
        buffer->GetU8(patchVersion);
        buffer->GetU8(minorVersion);
        buffer->GetU8(buildType);
        buffer->GetU16(gameBuild);
        buffer->GetString(gameName);
        buffer->GetString(username);
    }
};
struct ServerLogonChallenge
{
    u8 status;
    u8 B[128];
    u8 s[8];

    u16 Serialize(std::shared_ptr<ByteBuffer> buffer)
    {
        u16 size = static_cast<u16>(buffer->WrittenData);

        buffer->PutU8(status);
        if (status == 0)
        {
            buffer->PutBytes(B, 128);
            buffer->PutBytes(s, 8);
        }

        return static_cast<u16>(buffer->WrittenData) - size;
    }
};
struct ClientLogonResponse
{
    u8 M1[32];

    void Deserialize(std::shared_ptr<ByteBuffer> buffer)
    {
        buffer->GetBytes(M1, 32);
    }
};
#pragma pack(pop)

class NetworkClient
{
public:
    using tcp = asio::ip::tcp;
    NetworkClient(tcp::socket* socket, u64 identity = 0, bool isInternal = false) : _identity(identity)
    {
        _baseSocket = new BaseSocket(socket, std::bind(&NetworkClient::HandleRead, this), std::bind(&NetworkClient::HandleDisconnect, this));
    }

    void Listen();
    bool Connect(tcp::endpoint endpoint);
    bool Connect(std::string address, u16 port);
    void HandleConnect();
    void HandleDisconnect();
    void HandleRead();
    void Send(ByteBuffer* buffer) { _baseSocket->Send(buffer); }
    void Close(asio::error_code code) { _baseSocket->Close(code); }
    bool IsClosed() { return _baseSocket->IsClosed(); }

    BaseSocket* GetBaseSocket() { return _baseSocket; }

    u64 GetIdentity() { return _identity; }
    void SetIdentity(u64 identity) { _identity = identity; }

    std::string username = "";
private:
    BaseSocket* _baseSocket;
    u64 _identity;
};