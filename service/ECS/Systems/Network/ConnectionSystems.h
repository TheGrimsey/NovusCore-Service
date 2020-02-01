#pragma once
#include <entt.hpp>

class ConnectionUpdateSystem
{
public:
    static void Update(entt::registry& registry); 
};

class ConnectionDeferredSystem
{
public:
    static void Update(entt::registry& registry);
};