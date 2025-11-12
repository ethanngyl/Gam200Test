/**
===============================================================================
 File:           Event.h
 Author:         Josh Ong
 Email:          josh.o@digipen.edu
 Date:           2025-11-03
 Contribution:   100&
 ------------------------------------------------------------------------------

  Brief:
  - Defines the entire framework for the Asynchronous Event Messaging System.
  - This file establishes the core concepts: Message IDs, the base EventMessage
    class, concrete Message classes (e.g., EnemyDamagedMessage), the
    IMessageHandler interface, and the EventSystem structure.

  Key features:
  - **Message Definitions**: Uses MessageIDs to uniquely identify event types.
  - **Double Dispatch**: The Dispatch virtual method enables type-safe handling
    of messages via the IMessageHandler interface.
  - **EventSystem API**: Provides methods for registering subscribers (callbacks/handlers),
    queueing new messages (QueueMessage), and processing the queue (Update/ProcessMessages).
  - **Data Carriers**: Concrete message classes (like EnemyDamagedMessage) carry
    payload data (damage amount, entity IDs, position) between systems.
===============================================================================
 */

#pragma once
#include "Precompiled.h"
#include "Interface.h"
#include "ECSEntity.h"
#include "Vector2D.h"

namespace Framework {

    using MessageID = uint32_t;

    namespace MessageIds {
        constexpr MessageID unknown = 0;
        constexpr MessageID enemyDamaged = 1;
        constexpr MessageID enemyDeath = 2;
    }

    class EventMessage {
    public: 
        MessageID id;

        EventMessage(MessageID msgId) : id(msgId) {}
        virtual ~EventMessage() = default;

        virtual void Dispatch(class IMessageHandler& handler) const = 0;
    };

    class EnemyDamagedMessage : public EventMessage {
    public:
        Entity enemyEntity;
        Entity playerEntity;
        int damage;
        int health;
        Vector2D hitPosition;

        EnemyDamagedMessage(Entity enemy, Entity player, int dmg, int hp, Vector2D pos)
            : EventMessage(MessageIds::enemyDamaged)
            ,enemyEntity(enemy), playerEntity(player)
            , damage(dmg)
            , health(hp)
            , hitPosition(pos)
        {
        }

        void Dispatch(IMessageHandler& handler) const override;
    };

    class EnemyDeathMessage : public EventMessage {
    public:
        Entity enemyEntity;
        Entity playerEntity;
        Vector2D deathPosition;

        EnemyDeathMessage(Entity enemy, Entity killer,
            Vector2D pos)
            : EventMessage(MessageIds::enemyDeath)
            , enemyEntity(enemy)
            , playerEntity(killer)
            , deathPosition(pos)
        {
        }

        void Dispatch(IMessageHandler& handler) const override;
    };

    class IMessageHandler {
    public:
        virtual ~IMessageHandler() = default;

        virtual void HandleMessage(const EventMessage& msg) { (void)msg; }

        virtual void HandleMessage(const EnemyDamagedMessage& msg) { (void)msg; }

        virtual void HandleMessage(const EnemyDeathMessage& msg) { (void)msg; }

    };


    //// Base event data class
    //struct EventData {
    //    virtual ~EventData() = default;
    //};

    //// Event structure
    //struct Event {
    //    std::string type;
    //    EventData* data;

    //    Event(const std::string& t, EventData* d) : type(t), data(d) {}
    //};

    // Callback type
    using MessageCallback = std::function<void(const EventMessage*)>;

    class EventSystem : public EngineSystem
    {
    public:
        virtual ~EventSystem();

        // EngineSystem interface
        virtual void Initialize() override;
        virtual void Update(float dt) override;
        virtual void SendEngineMessage(Message* message) override;


        // ====================================================================
        // BROADCASTING
        // ====================================================================

        /**
         * @brief Broadcast message to all subscribers
         * @param msg Message to broadcast (system takes ownership)
         */
        void BroadcastMessage(EventMessage* msg);

        /**
         * @brief Queue message for deferred processing
         */
        void QueueMessage(EventMessage* msg);

        // ====================================================================
        // OBSERVER REGISTRATION
        // ====================================================================

        /**
         * @brief Subscribe to specific message type with callback
         * @param messageId Type of message to listen for
         * @param callback Function to call when message received
         */
        void Subscribe(MessageID messageId, MessageCallback callback);

        /**
         * @brief Subscribe an object that implements IMessageHandler
         * @param messageId Type of message to listen for
         * @param handler Object to notify (uses double dispatch)
         */
        void RegisterObserver(MessageID messageId, IMessageHandler* handler);

        /**
         * @brief Unregister an observer
         */
        void UnregisterObserver(MessageID messageId, IMessageHandler* handler);

        /**
         * @brief Clear all subscribers for a message type
         */
        void ClearSubscribers(MessageID messageId);

        /**
         * @brief Clear all subscribers
         */
        void ClearAllSubscribers();

        // ====================================================================
        // DIAGNOSTICS
        // ====================================================================

        /**
         * @brief Get number of subscribers for a message type
         */
        size_t GetSubscriberCount(MessageID messageId) const;

        /**
         * @brief Get total number of queued messages
         */
        size_t GetQueuedMessageCount() const { return messageQueue.size(); }

        /**
         * @brief Enable/disable message logging
         */
        void SetLogging(bool enable) { loggingEnabled = enable; }

    private:
        // Process all queued messages
        void ProcessMessages();

        // Send message to all registered observers
        void SendToObservers(const EventMessage* msg);

        // Subscriber storage
        struct Subscriber {
            MessageCallback callback;
            IMessageHandler* handler;

            Subscriber(MessageCallback cb) : callback(cb), handler(nullptr) {}
            Subscriber(IMessageHandler* h) : callback(nullptr), handler(h) {}
        };

        std::unordered_map<MessageID, std::vector<Subscriber>> subscribers;
        std::queue<EventMessage*> messageQueue;
        bool loggingEnabled = true;

        // Statistics
        struct Stats {
            int messagesProcessed = 0;
            int messagesQueued = 0;
            int broadcastsSent = 0;
        } stats;
    };
}