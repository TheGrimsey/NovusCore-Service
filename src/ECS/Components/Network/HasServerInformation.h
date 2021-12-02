#pragma once
#include <Networking/NetStructures.h>
#include <entity/fwd.hpp>

struct HasServerInformation
{
    entt::entity entity = entt::null;
    AddressType type = AddressType::INVALID;
    u8 realmId = 0;
};