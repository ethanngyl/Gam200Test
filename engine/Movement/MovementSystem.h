#pragma once
#include "Interface.h"
#include "ECSEntityManager.h"


namespace Framework
{
    class InputSystem;//Forward declaration
    class MovementSystem : public InterfaceSystem
    {
    public:
        MovementSystem();
        virtual ~MovementSystem();

        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendEngineMessage(Message* message) override;
        void SetInputSystem(InputSystem* input) { inputSystem = input; }
        void SetEntityManager(EntityManager* em) { entityManager = em; }


    private:
        EntityManager* entityManager;
        InputSystem* inputSystem;
    };
}