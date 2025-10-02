/*
============================================================================== =
File:   MathTestSystem.h
Author : Josh Ong
Email : josh.o@digipen.edu
Date : 2025-09-22
Contribution : 100 %
------------------------------------------------------------------------------
Header file for the MathTestSystem class.

Design notes:
    This file declares the MathTestSystem.It's a simple, one-shot system that
    inherits from the engine's base InterfaceSystem. Its only purpose is to run
    a series of checks on the math library during engine initialization to ensure
    correctness.The main logic is contained within the Initialize() override.
    Other standard system functions like Update() are not required.
============================================================================== =
*/
#pragma once
#include "Precompiled.h" // The base class for all your engine systems

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