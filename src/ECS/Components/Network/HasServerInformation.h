#pragma once
#include <Networking/AddressType.h>
#include <entity/fwd.hpp>

struct HasServerInformation
{
    entt::entity entity = entt::null;
    AddressType type = AddressType::INVALID;
};