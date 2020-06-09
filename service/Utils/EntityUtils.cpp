#include "EntityUtils.h"
#include <entt.hpp>
#include <Utils/ByteBuffer.h>
#include "../ECS/Components/Transform.h"
#include "../ECS/Components/GameEntityInfo.h"

u16 EntityUtils::Serialize(const entt::entity& entity, GameEntityInfo& gameEntityInfo, Transform& transform, std::shared_ptr<ByteBuffer>& bytebuffer)
{
    size_t initialWrittenData = bytebuffer->WrittenData;

    // There is a world in which we want to use GUIDs and simply not care about registry ids
    bytebuffer->Put(entity); // Registry Id
    bytebuffer->Put(gameEntityInfo.type); // Type: (None, Gameobject, Creature, Player, Item)
    bytebuffer->PutU32(gameEntityInfo.entryId); // Entry Id (This can be used to do lookups on model paths etc)
    bytebuffer->Put<vec3>(transform.position); // Position
    bytebuffer->Put<vec3>(transform.rotation); // Rotation
    bytebuffer->Put<vec3>(transform.scale); // Scale

    return static_cast<u16>(bytebuffer->WrittenData - initialWrittenData);
}
