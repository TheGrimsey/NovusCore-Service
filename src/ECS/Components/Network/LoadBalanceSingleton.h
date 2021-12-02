#pragma once
#include <NovusTypes.h>
#include <Networking/NetStructures.h>
#include <entity/fwd.hpp>
#include <robin_hood.h>
#include <vector>

#pragma pack(push, 1)
struct ServerInformation
{
    entt::entity entity = entt::null;
    AddressType type = AddressType::INVALID;
    u8 realmId = 0;
    u32 address = 0;
    u16 port = 0;
};
#pragma pack(pop)
struct LoadBalanceSingleton
{
    LoadBalanceSingleton()
    {
        authServers.reserve(8);
        loadBalancers.reserve(8);
        regionServers.reserve(8);
        chatServers.reserve(8);

        realmServersMap.reserve(8);
        realmServersIndex.reserve(8);

        worldServersMap.reserve(8);
        worldServersIndex.reserve(8);

        instanceServersMap.reserve(8);
        instanceServersIndex.reserve(8);
    }
    
    inline void Clear()
    {
        authServers.clear();
        loadBalancers.clear();
        regionServers.clear();
        chatServers.clear();

        realmServersMap.clear();
        realmServersIndex.clear();

        worldServersMap.clear();
        worldServersIndex.clear();

        instanceServersMap.clear();
        instanceServersIndex.clear();
    }

    inline void Remove(AddressType type, entt::entity entity, u8 realmId = 0)
    {
        std::vector<ServerInformation>* serverInformations = nullptr;

        if (type == AddressType::AUTH)
        {
            serverInformations = &authServers;
        }
        else if (type == AddressType::LOADBALANCE)
        {
            serverInformations = &loadBalancers;
        }
        else if (type == AddressType::REGION)
        {
            serverInformations = &regionServers;
        }
        else if (type == AddressType::CHAT)
        {
            serverInformations = &chatServers;
        }
        else if (type == AddressType::REALM)
        {
            serverInformations = &realmServersMap[realmId];
        }
        else if (type == AddressType::WORLD)
        {
            serverInformations = &worldServersMap[realmId];
        }
        else if (type == AddressType::INSTANCE)
        {
            serverInformations = &instanceServersMap[realmId];
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
        else if constexpr (type == AddressType::LOADBALANCE)
        {
            loadBalancers.emplace_back(info);
        }
        else if constexpr (type == AddressType::REGION)
        {
            regionServers.emplace_back(info);
        }
        else if constexpr (type == AddressType::CHAT)
        {
            chatServers.emplace_back(info);
        }
        else if constexpr (type == AddressType::REALM)
        {
            if (realmServersMap.find(info.realmId) == realmServersMap.end())
            {
                realmServersMap[info.realmId] = std::vector<ServerInformation>();
                realmServersMap[info.realmId].reserve(8);
                realmServersIndex.push_back(0);
            }
            
            realmServersMap[info.realmId].push_back(info);
        }
        else if constexpr (type == AddressType::WORLD)
        {
            if (worldServersMap.find(info.realmId) == worldServersMap.end())
            {
                worldServersMap[info.realmId] = std::vector<ServerInformation>();
                worldServersMap[info.realmId].reserve(8);
            }

            worldServersMap[info.realmId].push_back(info);
        }
        else if constexpr (type == AddressType::INSTANCE)
        {
            if (instanceServersMap.find(info.realmId) == instanceServersMap.end())
            {
                instanceServersMap[info.realmId] = std::vector<ServerInformation>();
                instanceServersMap[info.realmId].reserve(8);
            }

            instanceServersMap[info.realmId].push_back(info);
        }
    }
    template <AddressType type>
    inline bool Get(ServerInformation& serverInformation, u8 realmId = 0)
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

            serverInformation = regionServers[regionIndex++];

            // Wrap index around if needed
            if (regionIndex == numOf)
                regionIndex = 0;
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
        else if constexpr (type == AddressType::REALM)
        {
            size_t numOf = realmServersMap[realmId].size();
            if (numOf == 0)
                return false;

            u8& index = realmServersIndex[realmId];
            serverInformation = realmServersMap[realmId][index++];

            // Wrap index around if needed
            if (index == numOf)
                index = 0;
        }
        else if constexpr (type == AddressType::WORLD)
        {
            size_t numOf = worldServersMap[realmId].size();
            if (numOf == 0)
                return false;

            u8& index = worldServersIndex[realmId];
            serverInformation = worldServersMap[realmId][index++];

            // Wrap index around if needed
            if (index == numOf)
                index = 0;
        }
        else if constexpr (type == AddressType::INSTANCE)
        {
            size_t numOf = instanceServersMap[realmId].size();
            if (numOf == 0)
                return false;

            u8& index = instanceServersIndex[realmId];
            serverInformation = instanceServersMap[realmId][index++];

            // Wrap index around if needed
            if (index == numOf)
                index = 0;
        }

        return true;
    }

    const std::vector<ServerInformation> GetServerInformations()
    {
        std::vector<ServerInformation> servers;

        size_t reserveSize = authServers.size() + chatServers.size() + loadBalancers.size() + regionServers.size();

        for (auto& itr : realmServersMap)
        {
            reserveSize += itr.second.size();
        }
        for (auto& itr : worldServersMap)
        {
            reserveSize += itr.second.size();
        }
        for (auto& itr : instanceServersMap)
        {
            reserveSize += itr.second.size();
        }

        servers.reserve(reserveSize);
        servers.insert(servers.end(), authServers.begin(), authServers.end());
        servers.insert(servers.end(), chatServers.begin(), chatServers.end());
        servers.insert(servers.end(), loadBalancers.begin(), loadBalancers.end());
        servers.insert(servers.end(), regionServers.begin(), regionServers.end());

        for (auto& itr : realmServersMap)
        {
            servers.insert(servers.end(), itr.second.begin(), itr.second.end());
        }
        for (auto& itr : worldServersMap)
        {
            servers.insert(servers.end(), itr.second.begin(), itr.second.end());
        }
        for (auto& itr : instanceServersMap)
        {
            servers.insert(servers.end(), itr.second.begin(), itr.second.end());
        }

        return servers;
    }
    const std::vector<ServerInformation>& GetLoadBalancers()
    {
        return loadBalancers;
    }
private:
    u8 authIndex = 0;
    u8 loadBalanceIndex = 0;
    u8 regionIndex = 0;
    u8 chatIndex = 0;

    std::vector<ServerInformation> authServers;
    std::vector<ServerInformation> loadBalancers;
    std::vector<ServerInformation> regionServers;
    std::vector<ServerInformation> chatServers;

    std::vector<u8> realmServersIndex;
    std::vector<u8> worldServersIndex;
    std::vector<u8> instanceServersIndex;

    robin_hood::unordered_map<u8, std::vector<ServerInformation>> realmServersMap;
    robin_hood::unordered_map<u8, std::vector<ServerInformation>> worldServersMap;
    robin_hood::unordered_map<u8, std::vector<ServerInformation>> instanceServersMap;
};