#pragma once

#pragma once
#include "Interface.h" // The base class for all your engine systems
#include <iostream>

namespace Framework {

    // This system runs a one-time check of the math library
    // and prints the results during engine initialization.
    class MathTestSystem : public InterfaceSystem
    {
    public:
        MathTestSystem() = default;
        ~MathTestSystem() override = default;

        // The tests will be executed inside the Initialize() method.
        virtual void Initialize() override;

        // These methods are required but will be empty for this test system.
        virtual void Update(float dt) override;
        virtual void SendEngineMessage(Message* message) override;
    };

} // namespace Framework