#pragma once
#include <NovusTypes.h>
#include <Networking/AddressType.h>
#include <entity/fwd.hpp>
#include <vector>

#pragma pack(push, 1)
struct ServerInformation
{
    entt::entity entity = entt::null;
    AddressType type = AddressType::INVALID;
    u32 address = 0;
    u16 port = 0;
};
#pragma pack(pop)
struct LoadBalanceSingleton
{
    LoadBalanceSingleton()
    {
        authServers.reserve(8);
        realmServers.reserve(8);
        worldServers.reserve(8);
        instanceServers.reserve(8);
        loadBalancers.reserve(8);
        regionServers.reserve(8);
    }
    
    inline void Clear()
    {
        authServers.clear();
        realmServers.clear();
        worldServers.clear();
        instanceServers.clear();
        loadBalancers.clear();
        regionServers.clear();
    }

    inline void Remove(AddressType type, entt::entity entity)
    {
        std::vector<ServerInformation>* serverInformations = nullptr;

        if (type == AddressType::AUTH)
        {
            serverInformations = &authServers;
        }
        else if (type == AddressType::REALM)
        {
            serverInformations = &realmServers;
        }
        else if (type == AddressType::WORLD)
        {
            serverInformations = &worldServers;
        }
        else if (type == AddressType::INSTANCE)
        {
            serverInformations = &instanceServers;
        }
        else if (type == AddressType::CHAT)
        {
            serverInformations = &chatServers;
        }
        else if (type == AddressType::LOADBALANCE)
        {
            serverInformations = &loadBalancers;
        }
        else if (type == AddressType::REGION)
        {
            serverInformations = &regionServers;
        }

        auto itr = std::find_if(serverInformations->begin(), serverInformations->end(), [&entity](const ServerInformation& info) -> bool { return info.entity == entity; });
        if (itr != serverInformations->end())
        {
            serverInformations->erase(itr);
        }
    }

    template <AddressType type>
    inline void Add(ServerInformation info)
    {
        if constexpr (type == AddressType::AUTH)
        {
            authServers.emplace_back(info);
        }
        else if constexpr (type == AddressType::REALM)
        {
            realmServers.emplace_back(info);
        }
        else if constexpr (type == AddressType::WORLD)
        {
            worldServers.emplace_back(info);
        }
        else if constexpr (type == AddressType::INSTANCE)
        {
            instanceServers.emplace_back(info);
        }
        else if constexpr (type == AddressType::CHAT)
        {
            chatServers.emplace_back(info);
        }
        else if constexpr (type == AddressType::LOADBALANCE)
        {
            loadBalancers.emplace_back(info);
        }
        else if constexpr (type == AddressType::REGION)
        {
            regionServers.emplace_back(info);
        }
    }
    template <AddressType type>
    inline bool Get(ServerInformation& serverInformation)
    {
        if constexpr (type == AddressType::AUTH)
        {
            size_t numOf = authServers.size();
            if (numOf == 0)
                return false;

            serverInformation = authServers[authIndex++];

            // Wrap index around if needed
            if (authIndex == numOf)
                authIndex = 0;
        }
        else if constexpr (type == AddressType::REALM)
        {
            size_t numOf = realmServers.size();
            if (numOf == 0)
                return false;

            serverInformation = realmServers[realmIndex++];

            // Wrap index around if needed
            if (realmIndex == numOf)
                realmIndex = 0;
        }
        else if constexpr (type == AddressType::WORLD)
        {
            size_t numOf = worldServers.size();
            if (numOf == 0)
                return false;

            serverInformation = worldServers[worldIndex++];

            // Wrap index around if needed
            if (worldIndex == numOf)
                worldIndex = 0;
        }
        else if constexpr (type == AddressType::INSTANCE)
        {
            size_t numOf = instanceServers.size();
            if (numOf == 0)
                return false;

            serverInformation = instanceServers[instanceIndex++];

            // Wrap index around if needed
            if (instanceIndex == numOf)
                instanceIndex = 0;
        }
        else if constexpr (type == AddressType::CHAT)
        {
            size_t numOf = chatServers.size();
            if (numOf == 0)
                return false;

            serverInformation = chatServers[chatIndex++];

            // Wrap index around if needed
            if (chatIndex == numOf)
                chatIndex = 0;
        }
        else if constexpr (type == AddressType::LOADBALANCE)
        {
            size_t numOf = loadBalancers.size();
            if (numOf == 0)
                return false;

            serverInformation = loadBalancers[loadBalanceIndex++];

            // Wrap index around if needed
            if (loadBalanceIndex == numOf)
                loadBalanceIndex = 0;
        }
        else if constexpr (type == AddressType::REGION)
        {
            size_t numOf = regionServers.size();
            if (numOf == 0)
                return false;

            serverInformation = regionServers[realmIndex++];

            // Wrap index around if needed
            if (realmIndex == numOf)
                realmIndex = 0;
        }

        return true;
    }

    const std::vector<ServerInformation> GetServerInformations()
    {
        std::vector<ServerInformation> servers;
        servers.reserve(authServers.size() + realmServers.size() + worldServers.size() + instanceServers.size() + chatServers.size() + loadBalancers.size() + regionServers.size());

        servers.insert(servers.end(), authServers.begin(), authServers.end());
        servers.insert(servers.end(), realmServers.begin(), realmServers.end());
        servers.insert(servers.end(), worldServers.begin(), worldServers.end());
        servers.insert(servers.end(), instanceServers.begin(), instanceServers.end());
        servers.insert(servers.end(), chatServers.begin(), chatServers.end());
        servers.insert(servers.end(), loadBalancers.begin(), loadBalancers.end());
        servers.insert(servers.end(), regionServers.begin(), regionServers.end());

        return servers;
    }
    const std::vector<ServerInformation>& GetLoadBalancers()
    {
        return loadBalancers;
    }
private:
    u8 authIndex = 0;
    u8 realmIndex = 0;
    u8 worldIndex = 0;
    u8 instanceIndex = 0;
    u8 chatIndex = 0;
    u8 loadBalanceIndex = 0;
    u8 regionIndex = 0;

    std::vector<ServerInformation> authServers;
    std::vector<ServerInformation> realmServers;
    std::vector<ServerInformation> worldServers;
    std::vector<ServerInformation> instanceServers;
    std::vector<ServerInformation> chatServers;
    std::vector<ServerInformation> loadBalancers;
    std::vector<ServerInformation> regionServers;
};