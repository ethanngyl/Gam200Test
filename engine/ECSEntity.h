#pragma once
#include <cstdint>

namespace Framework
{
    using EntityID = uint32_t;
    constexpr EntityID INVALID_ENTITY = 0;

    class Entity
    {
    public:
        Entity() : id(INVALID_ENTITY) {}
        Entity(EntityID entityId) : id(entityId) {}

        EntityID GetID() const { return id; }
        bool IsValid() const { return id != INVALID_ENTITY; }

        bool operator==(const Entity& other) const { return id == other.id; }
        bool operator!=(const Entity& other) const { return id != other.id; }

    private:
        EntityID id;
    };
}