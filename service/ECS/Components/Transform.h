#pragma once
#include <NovusTypes.h>
#include <glm/matrix.hpp>
#include <glm/gtx/rotate_vector.hpp>

enum class MovementFlags : u32
{
    NONE,
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,

    VERTICAL = FORWARD | BACKWARD,
    HORIZONTAL = LEFT | RIGHT,
    ALL = HORIZONTAL | VERTICAL
};

inline MovementFlags operator &(MovementFlags lhs, MovementFlags rhs)
{
    return static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) &
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );
}
inline MovementFlags operator ^(MovementFlags lhs, MovementFlags rhs)
{
    return static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) ^
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );
}
inline MovementFlags operator ~(MovementFlags rhs)
{
    return static_cast<MovementFlags> (
        ~static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );
}
inline MovementFlags& operator |=(MovementFlags& lhs, MovementFlags rhs)
{
    lhs = static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) |
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );

    return lhs;
}
inline MovementFlags& operator &=(MovementFlags& lhs, MovementFlags rhs)
{
    lhs = static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) &
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );

    return lhs;
}
inline MovementFlags& operator ^=(MovementFlags& lhs, MovementFlags rhs)
{
    lhs = static_cast<MovementFlags> (
        static_cast<std::underlying_type<MovementFlags>::type>(lhs) ^
        static_cast<std::underlying_type<MovementFlags>::type>(rhs)
        );

    return lhs;
}

struct Transform
{
    vec3 position;
    vec3 rotation;
    vec3 scale = vec3(1, 1, 1);

    MovementFlags moveFlags;
    inline void AddMoveFlag(MovementFlags flag)
    {
        moveFlags |= flag;
    }
    inline void RemoveMoveFlag(MovementFlags flag)
    {
        moveFlags &= ~flag;
    }
    inline bool HasMoveFlag(MovementFlags flag)
    {
        return (moveFlags & flag) == flag;
    }

    bool positionForward = true;
    f32 positionOffset = 0;
    bool scaleForward = true;
    f32 scaleOffset = 0;
    bool isDirty = true;


};