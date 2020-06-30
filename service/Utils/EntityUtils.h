#pragma once
#include <NovusTypes.h>
#include <string>
#include <memory>
#include <entity/fwd.hpp>

class Bytebuffer;
struct GameEntityInfo;
struct Transform;
namespace EntityUtils
{
    // This function modifies the registry thus it should only be called from the main thread (entt:registry is not thread-safe)
    u16 Serialize(const entt::entity& entity, GameEntityInfo& gameEntityInfo, Transform& transform, std::shared_ptr<Bytebuffer>& bytebuffer);
}