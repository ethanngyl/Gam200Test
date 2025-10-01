#pragma once
#include <cstdint>

namespace Framework
{
    using ComponentTypeID = uint32_t;

    class ComponentBase
    {
    public:
        virtual ~ComponentBase() = default;
    };

    template<typename T>
    class Component : public ComponentBase
    {
    public:
        static ComponentTypeID GetTypeID()
        {
            static ComponentTypeID typeID = GetNextTypeID();
            return typeID;
        }

    private:
        static ComponentTypeID GetNextTypeID()
        {
            static ComponentTypeID nextID = 0;
            return ++nextID;
        }
    };
}