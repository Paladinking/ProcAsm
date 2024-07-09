#include "events.h"
#include "exceptions.h"
#include "log.h"

EventScope::EventScope(Events *events) : events{events} {}

EventScope::EventScope(EventScope &&other)
    : events{other.events}, cb_indicies{std::move(other.cb_indicies)},
      aux_indicies{std::move(other.aux_indicies)} {
    other.events = nullptr;
}

EventScope& EventScope::operator=(EventScope&& other) {
    if (this == &other) {
        return *this;
    }
    std::swap(events, other.events);
    std::swap(aux_indicies, other.aux_indicies);
    std::swap(cb_indicies, other.cb_indicies);
    return *this;
}

EventScope Events::end_scope() {
    if (scopes.size() > 1) {
        EventScope top = std::move(scopes.back());
        scopes.pop_back();
        return top;
    }
    throw base_exception("No matching begin_scope");
}

void EventScope::add_callback(int event, std::size_t ix) {
    cb_indicies.push_front(ix);
    cb_indicies.push_front(event);
}

void EventScope::add_aux(std::size_t ix) { aux_indicies.push_front(ix); }

void EventScope::add_event(int event) {
    evt_indicies.push_front(event);
}

EventScope::~EventScope() {
    if (events == nullptr) {
        return;
    }
    auto it = cb_indicies.begin();
    while (it != cb_indicies.end()) {
        int evt = *it;
        ++it;
        std::size_t ix = *it;
        ++it;
        events->remove_callback(evt, ix);
    }
    for (std::size_t aux : aux_indicies) {
        events->remove_aux(aux);
    }
    for (int evt: evt_indicies) {
        events->remove_event(evt);
    }
}

Events::Events() noexcept : events{{EventType::EMPTY}}, scopes{} {
    scopes.emplace_back(this);
}

void Events::begin_scope() { scopes.emplace_back(this); }

int Events::register_event(EventType type, int vector_size) {
    int id = free_ix;
    ++free_ix;
    for (; free_ix < events.size(); ++free_ix) {
        if (events[free_ix].type == EventType::EMPTY) {
            break;
        }
    }
    if (id > last_ix) {
        last_ix = id;
    }
    if (id == events.size()) {
        events.resize(id + 1);
    }
    events[id] = {type};
    if (type == EventType::UNIFIED_VEC) {
        if (vector_size <= 1) {
            events[id].type = EventType::UNIFIED;
        } else {
            events[id].buffer.resize(vector_size);
            for (auto &e : events[id].buffer) {
                e.u = 0;
            }
        }
    }
    scopes.back().add_event(id);
    return id;
}

void Events::register_callback(int id, void (*callback)(EventInfo, void *),
                               void *aux) {
    scopes.back().add_callback(id, events[id].free_ix);
    LOG_DEBUG("Event %d callback added at %d", id, events[id].free_ix);
    if (events[id].free_ix == events[id].callbacks.size()) {
        events[id].callbacks.push_back({callback, aux});
        events[id].last_ix = events[id].free_ix;
        ++events[id].free_ix;
    } else {
        events[id].callbacks[events[id].free_ix] = {callback, aux};
        if (events[id].free_ix > events[id].last_ix) {
            events[id].last_ix = events[id].free_ix;
        }
        std::size_t ix = events[id].free_ix + 1;
        for (; ix < events[id].callbacks.size(); ++ix) {
            if (events[id].callbacks[ix].callback == nullptr) {
                break;
            }
        }
        events[id].free_ix = ix;
    }
    LOG_DEBUG("New free: %d", events[id].free_ix);
}

void Events::notify_event(int id, EventInfo data) {
    switch (events[id].type) {
    case EventType::IMMEDIATE:
        for (auto &cb : events[id].callbacks) {
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
    case EventType::EMPTY:
        LOG_WARNING("Empty event %d called", id);
        return;
    }
}

void Events::add_aux(WrapperBase *aux) {
    scopes.back().add_aux(free_aux);
    if (free_aux == aux_data.size()) {
        aux_data.emplace_back(aux);
        ++free_aux;
    } else {
        aux_data[free_aux].reset(aux);
        ++free_aux;
        for (; free_aux < aux_data.size(); ++free_aux) {
            if (aux_data[free_aux].get() == nullptr) {
                break;
            }
        }
    }
}

void null_callback(EventInfo, void*) {}

void Events::remove_event(int event) {
    LOG_DEBUG("Removing event %d", event);
    if (event <= 0 || event >= events.size()) {
        LOG_WARNING("Out of bounds event removed");
    }
    events[event].type = EventType::EMPTY;
    events[event].buffer.clear();
    events[event].callbacks.clear();
    if (event < free_ix) {
        free_ix = event;
    }
    if (event == last_ix) {
        int64_t i = event;
        for (; i >= 1; --i) {
            if (events[event].type != EventType::EMPTY) {
                break;
            }
        }
        last_ix = i;
    }
}

void Events::remove_callback(int event, std::size_t ix) {
    LOG_DEBUG("Removing event %d callback %d", event, ix);
    if (event <= 0 || event >= events.size()) {
        LOG_WARNING("Out of bounds event callback removed");
        return;
    }
    if (events[event].callbacks.size() <= ix) {
        LOG_WARNING("Out of bounds event callback removed");
        return;
    }
    if (events[event].type == EventType::EMPTY) {
        LOG_WARNING("Removing calback from empty event");
        return;
    }
    events[event].callbacks[ix].callback = null_callback;
    events[event].callbacks[ix].aux = nullptr;
    if (ix < events[event].free_ix) {
        events[event].free_ix = ix;
    }
    if (ix == events[event].last_ix) {
        int64_t i = ix;
        for (; i >= 0; --i) {
            if (events[event].callbacks[ix].callback != null_callback) {
                break;
            }
        }
        events[event].last_ix = i;
        events[event].callbacks.resize(i + 1);
    }
}

void Events::remove_aux(std::size_t ix) {
    LOG_DEBUG("Removing aux %d", ix);
    aux_data[ix] = std::move(aux_data.back());
    aux_data.pop_back();
}

void Events::handle_events() {
    for (auto event = events.begin() + 1; event != events.end(); ++event) {
        if (event->type == EventType::UNIFIED && event->triggered) {
            for (auto &cb : event->callbacks) {
                cb.callback(event->data, cb.aux);
            }
            event->triggered = false;
        } else if (event->type == EventType::DELAYED) {
            for (EventInfo data : event->buffer) {
                for (auto &cb : event->callbacks) {
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
                    for (auto &cb : event->callbacks) {
                        cb.callback(data, cb.aux);
                    }
                }
            }
            event->triggered = false;
        }
    }
}
