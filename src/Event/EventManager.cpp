/**
===============================================================================
 File:           EventManager.cpp
 Author:         Josh Ong
 Email:          josh.o@digipen.edu 
 Date:           2025-11-03
 Contribution:   100%
 ------------------------------------------------------------------------------

  Brief:
  - Implements the core logic of the EventSystem (defined in Event.h).
  - Handles the registration, storage, queuing, and delivery of all messages
    within the framework.

  Key implementations:
  - **Double Dispatch**: Implementation of the concrete message Dispatch() methods
    to facilitate calling the correct HandleMessage() overload on the subscriber.
  - **Message Processing**: The ProcessMessages() loop dequeues messages and
    sends them to all registered observers based on their MessageID.
  - **Subscriber Management**: Logic for storing IMessageHandlers and functional
    callbacks in an unordered_map.
  - **Cleanup**: Safe destruction of the EventSystem, including clearing the
    message queue and subscribers.
===============================================================================
 */

#include "Precompiled.h"
#include "Event.h"

namespace Framework {

    // ========================================================================
    // DOUBLE DISPATCH IMPLEMENTATION
    // ========================================================================

    void EnemyDamagedMessage::Dispatch(IMessageHandler& handler) const {
        handler.HandleMessage(*this);
    }

    void EnemyDeathMessage::Dispatch(IMessageHandler& handler) const {
        handler.HandleMessage(*this);
    }

    // ========================================================================
    // EVENT SYSTEM IMPLEMENTATION
    // ========================================================================

    EventSystem::~EventSystem() {
        // Clean up queued messages
        while (!messageQueue.empty()) {
            delete messageQueue.front();
            messageQueue.pop();
        }
        subscribers.clear();

        if (loggingEnabled) {
            std::cout << "[EventSystem] Destroyed\n";
            std::cout << "  Stats - Processed: " << stats.messagesProcessed
                << " | Queued: " << stats.messagesQueued
                << " | Broadcasts: " << stats.broadcastsSent << "\n";
        }
    }

    void EventSystem::Initialize() {
        if (loggingEnabled) {
            std::cout << "[EventSystem] Initialized\n";
        }
        stats = Stats();  // Reset statistics
    }

    void EventSystem::Update(float dt) {
        (void)dt;
        ProcessMessages();
    }

    void EventSystem::SendEngineMessage(Message* message) {
        if (message && loggingEnabled) {
            std::cout << "[EventSystem] Engine message received\n";
        }
    }

    // ========================================================================
    // BROADCASTING
    // ========================================================================

    void EventSystem::BroadcastMessage(EventMessage* msg) {
        if (!msg) return;

        if (loggingEnabled) {
            std::cout << "[EventSystem] Broadcasting message ID: " << msg->id << "\n";
        }

        SendToObservers(msg);
        stats.broadcastsSent++;

        delete msg;
    }

    void EventSystem::QueueMessage(EventMessage* msg) {
        if (!msg) return;

        messageQueue.push(msg);
        stats.messagesQueued++;

        if (loggingEnabled) {
            std::cout << "[EventSystem] Message queued (ID: " << msg->id
                << "). Queue size: " << messageQueue.size() << "\n";
        }
    }

    // ========================================================================
    // OBSERVER REGISTRATION
    // ========================================================================

    void EventSystem::Subscribe(MessageID messageId, MessageCallback callback) {
        subscribers[messageId].push_back(Subscriber(callback));

        if (loggingEnabled) {
            std::cout << "[EventSystem] Callback subscribed to message ID: " << messageId
                << " (Total: " << subscribers[messageId].size() << ")\n";
        }
    }

    void EventSystem::RegisterObserver(MessageID messageId, IMessageHandler* handler) {
        if (!handler) return;

        subscribers[messageId].push_back(Subscriber(handler));

        if (loggingEnabled) {
            std::cout << "[EventSystem] Observer registered for message ID: " << messageId
                << " (Total: " << subscribers[messageId].size() << ")\n";
        }
    }

    void EventSystem::UnregisterObserver(MessageID messageId, IMessageHandler* handler) {
        if (!handler) return;

        auto it = subscribers.find(messageId);
        if (it != subscribers.end()) {
            auto& subs = it->second;
            subs.erase(
                std::remove_if(subs.begin(), subs.end(),
                    [handler](const Subscriber& s) { return s.handler == handler; }),
                subs.end()
            );

            if (loggingEnabled) {
                std::cout << "[EventSystem] Observer unregistered from message ID: " << messageId << "\n";
            }
        }
    }

    void EventSystem::ClearSubscribers(MessageID messageId) {
        auto it = subscribers.find(messageId);
        if (it != subscribers.end()) {
            subscribers.erase(it);
            if (loggingEnabled) {
                std::cout << "[EventSystem] Cleared all subscribers for message ID: " << messageId << "\n";
            }
        }
    }

    void EventSystem::ClearAllSubscribers() {
        subscribers.clear();
        if (loggingEnabled) {
            std::cout << "[EventSystem] Cleared all subscribers\n";
        }
    }

    size_t EventSystem::GetSubscriberCount(MessageID messageId) const {
        auto it = subscribers.find(messageId);
        return (it != subscribers.end()) ? it->second.size() : 0;
    }

    // ========================================================================
    // MESSAGE PROCESSING
    // ========================================================================

    void EventSystem::ProcessMessages() {
        while (!messageQueue.empty()) {
            EventMessage* msg = messageQueue.front();
            messageQueue.pop();

            if (loggingEnabled) {
                std::cout << "[EventSystem] Processing message ID: " << msg->id
                    << " (Queue remaining: " << messageQueue.size() << ")\n";
            }

            SendToObservers(msg);
            stats.messagesProcessed++;

            delete msg;
        }
    }

    void EventSystem::SendToObservers(const EventMessage* msg) {
        if (!msg) return;

        auto it = subscribers.find(msg->id);
        if (it != subscribers.end()) {
            if (loggingEnabled) {
                std::cout << "[EventSystem] Notifying " << it->second.size()
                    << " subscribers for message ID: " << msg->id << "\n";
            }

            for (auto& subscriber : it->second) {
                if (subscriber.callback) {
                    subscriber.callback(msg);
                }
                else if (subscriber.handler) {
                    msg->Dispatch(*subscriber.handler);
                }
            }
        }
        else {
            if (loggingEnabled) {
                std::cout << "[EventSystem] No subscribers for message ID: " << msg->id << "\n";
            }
        }
    }

} // namespace framework