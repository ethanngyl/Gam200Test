//#include "Precompiled.h"
//#include "Interface.h"
//
//namespace framework {
//
//    // Base event data class
//    struct EventData {
//        virtual ~EventData() = default;
//    };
//
//    // Event structure
//    struct Event {
//        std::string type;
//        EventData* data;
//
//        Event(const std::string& t, EventData* d) : type(t), data(d) {}
//    };
//
//    // Callback type
//    using EventCallback = std::function<void(EventData*)>;
//
//    class EventSystem : public EngineSystem
//    {
//    public:
//        virtual ~EventSystem();
//
//        // EngineSystem interface
//        virtual void Initialize() override;
//        virtual void Update(float dt) override;
//        virtual void SendEngineMessage(Message* message) override;
//
//        // Event system API
//        void Subscribe(const std::string& eventType, EventCallback callback);
//        void Unsubscribe(const std::string& eventType, EventCallback callback);
//        void Publish(const std::string& eventType, EventData* data);
//
//    private:
//        void ProcessEvents();
//
//        std::unordered_map<std::string, std::vector<EventCallback>> m_subscribers;
//        std::queue<Event> m_eventQueue;
//    };
//}