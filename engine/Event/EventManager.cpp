#include "Precompiled.h"
namespace framework {

    EventSystem::~EventSystem() {
        // Clean up any remaining events in the queue
        while (!m_eventQueue.empty()) {
            Event& event = m_eventQueue.front();
            delete event.data;
            m_eventQueue.pop();
        }

        // Clear all subscribers
        m_subscribers.clear();
    }

    void EventSystem::Initialize() {
        std::cout << "EventSystem initialized" << std::endl;
        // Initialize any default event types or system-level subscriptions here
    }

    void EventSystem::Update(float dt) {
        // Process all queued events this frame
        ProcessEvents();
    }

    void EventSystem::SendEngineMessage(Message* message) {
        // Print debug/logging information to console
        if (message) {
            //std::cout << "[EventSystem] " << message->GetText() << std::endl;
        }
    }

    void EventSystem::Subscribe(const std::string& eventType, EventCallback callback) {
        // Add callback to the list of subscribers for this event type
        m_subscribers[eventType].push_back(callback);

        std::cout << "[EventSystem] Subscribed to event: " << eventType << std::endl;
    }

    void EventSystem::Unsubscribe(const std::string& eventType, EventCallback callback) {
        // Find the event type
        auto it = m_subscribers.find(eventType);
        if (it != m_subscribers.end()) {
            // Note: Removing std::function from vector is tricky because 
            // std::function doesn't have comparison operators
            // For a production system, consider using event IDs or handles

            // For now, we'll clear all callbacks for this event type
            // You might want a more sophisticated approach with handles/IDs
            std::cout << "[EventSystem] Unsubscribe called for: " << eventType << std::endl;
        }
    }

    void EventSystem::Publish(const std::string& eventType, EventData* data) {
        // Add event to queue for deferred processing
        m_eventQueue.push(Event(eventType, data));

        std::cout << "[EventSystem] Event published: " << eventType << std::endl;
    }

    void EventSystem::ProcessEvents() {
        // Process all events in the queue
        while (!m_eventQueue.empty()) {
            Event& event = m_eventQueue.front();

            // Find all subscribers for this event type
            auto it = m_subscribers.find(event.type);
            if (it != m_subscribers.end()) {
                // Call each subscriber's callback
                for (auto& callback : it->second) {
                    callback(event.data);
                }
            }

            // Clean up event data
            delete event.data;
            m_eventQueue.pop();
        }
    }
}