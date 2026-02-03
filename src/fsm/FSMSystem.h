/**
===============================================================================
 File:           FSMSystem.h
 Author:
 Date:           2025-01-26
 Contribution:   100%
 ------------------------------------------------------------------------------

 FINITE STATE MACHINE - Engine System

 Design notes:
   FSMSystem is an EngineSystem that updates all entities with FSMComponent.
   Follows your engine's system pattern (like SkillSystem, MovementSystem).

 Integration:
   1. Add to Core.h:
      - Forward declare: class FSMSystem;
      - Member: FSMSystem* fsmSystem;
      - Getter: FSMSystem* GetFSMSystem() const { return fsmSystem; }

   2. Add to Core.cpp CreateAllSystems():
      fsmSystem = new FSMSystem();

   3. Add to Core.cpp WireSystemDependencies():
      fsmSystem->SetEntityManager(entityManager);

   4. Add to Core.cpp AddSystemsToEngine():
      AddSystem(fsmSystem);

   5. Add to Core.cpp Cleanup():
      delete fsmSystem;

===============================================================================
*/

#pragma once

#include "Interface.h"

namespace Framework {

    // Forward declarations
    class EntityManager;

    /**
     * @class FSMSystem
     * @brief Updates all entities that have FSMComponent
     *
     * This system iterates through all entities with FSMComponent
     * and calls Update() on their state machines each frame.
     */
    class FSMSystem : public EngineSystem {
    public:
        FSMSystem();
        ~FSMSystem();

        // === ENGINE INTERFACE ===
        void Initialize() override;
        void Update(float dt) override;
        void SendEngineMessage(Message* msg) override;

        // === DEPENDENCIES ===
        void SetEntityManager(EntityManager* em) { m_entityManager = em; }

    private:
        EntityManager* m_entityManager;
    };

} // namespace Framework