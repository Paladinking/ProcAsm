#include "events.h"

Events::Events() noexcept : events{{EventType::UNIFIED}} {

}

int Events::register_event(EventType type, int id, int vector_size) {
    if (id <= 0) {
        id = events.size();
    }
    if (id >= events.size()) {
        events.resize(id + 1);
    }
    events[id] = {type};
    if (type == EventType::UNIFIED_VEC) {
        if (vector_size <= 1) {
            events[id].type = EventType::UNIFIED;
        } else {
            events[id].buffer.resize(vector_size);
            for (auto& e: events[id].buffer) {
                e.u = 0;
            }
        }
    }
    return id;
}

void Events::register_callback(int id, void(*callback)(EventInfo, void*), void* aux)  {
    events[id].callbacks.push_back({callback, aux});
}

void Events::notify_event(int id, EventInfo data) {
    switch (events[id].type) {
        case EventType::IMMEDIATE:
            for (auto& cb: events[id].callbacks) {
                cb.callback(data, cb.aux);
            }
            return;
        case EventType::DELAYED:
            events[id].buffer.push_back(data);
            return;
        case EventType::UNIFIED:
            events[id].data = data;
            events[id].triggered = true;
            return;
        case EventType::UNIFIED_VEC:
            events[id].buffer[data.u].u = 1;
            events[id].triggered = true;
            return;
    }
}

void Events::handle_events() {
    for (auto event = events.begin() + 1; event != events.end(); ++event) {
        if (event->type == EventType::UNIFIED && event->triggered) {
            for (auto& cb: event->callbacks) {
                cb.callback(event->data, cb.aux);
            }
            event->triggered = false;
        } else if (event->type == EventType::DELAYED) {
            for (EventInfo data: event->buffer) {
                for (auto& cb: event->callbacks) {
                    cb.callback(data, cb.aux);
                }
            }
            event->buffer.clear();
        } else if (event->type == EventType::UNIFIED_VEC && event->triggered) {
            for (uint64_t i = 0; i < event->buffer.size(); ++i) {
                if (event->buffer[i].u != 0) {
                    event->buffer[i].u = 0;
                    EventInfo data;
                    data.u = i;
                    for (auto& cb: event->callbacks) {
                        cb.callback(data, cb.aux);
                    }
                }
            }
            event->triggered = false;
        }
    }
}
