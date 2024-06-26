#ifndef ENGINE_ENVENT_H
#define ENGINE_ENVENT_H
#include <cstdint>
#include <vector>

enum class EventType {
    IMMEDIATE, // Event triggers callaback instantly
    DELAYED, // Event triggers callback after tick
    UNIFIED, // Event triggers one callback for all changes this tick
    UNIFIED_VEC // Event triggers one callback for each unique data
};

union EventInfo {
    int64_t i;
    uint64_t u;
    void* ptr;
};

class Events {
public:
    void register_event(EventType type, int id, int vector_size = -1);

    void register_callback(int id, void (*callback)(EventInfo, void*), void* aux);

    void notify_event(int id, EventInfo data);
    void notify_event(int id, uint64_t u);
    void notify_event(int id, int64_t i);
    void notify_event(int id, void* ptr);

    void handle_events();
private:
    struct CallbackData {
        void (*callback)(EventInfo, void*);
        void* aux;
    };
    struct EventData {
        EventType type;
        EventInfo data;
        bool triggered;
        std::vector<EventInfo> buffer {};
        std::vector<CallbackData> callbacks {};
    };

    std::vector<EventData> events;
};


#endif
