#ifndef ENGINE_ENVENT_H
#define ENGINE_ENVENT_H
#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>
#include <forward_list>

constexpr int NULL_EVENT = 0;

enum class EventType {
    EMPTY,
    IMMEDIATE,  // Event triggers callaback instantly
    DELAYED,    // Event triggers callback after tick
    UNIFIED,    // Event triggers one callback for all changes this tick
    UNIFIED_VEC // Event triggers one callback for each unique data
};

union EventInfo {
    int64_t i;
    uint64_t u;
    void *ptr;

    template <class T> T get() { return get_helper<T>(); }

    template <> uint64_t get() { return u; }

    template <> uint32_t get() { return static_cast<uint32_t>(u); }

    template <> uint16_t get() { return static_cast<uint16_t>(u); }

    template <> uint8_t get() { return static_cast<uint8_t>(u); }

    template <> int64_t get() { return i; }

    template <> int32_t get() { return static_cast<int32_t>(i); }

    template <> int16_t get() { return static_cast<int16_t>(i); }

    template <> int8_t get() { return static_cast<int8_t>(i); }

    template <class T> void set(T t) { set_helper<T>(t); }

    template <> void set(uint64_t t) { u = t; }

    template <> void set(uint32_t t) { u = t; }
    
    template <> void set(uint16_t t) { u = t; }
    
    template <> void set(uint8_t t) { u = t; }

    template <> void set(int64_t t) { i = t; }

    template <> void set(int32_t t) { i = t; }
    
    template <> void set(int16_t t) { i = t; }
    
    template <> void set(int8_t t) { i = t; }

private:
    template <class T, T = nullptr> T get_helper() {
        return reinterpret_cast<T>(ptr);
    }

    template <class T, T = nullptr> void set_helper(T t) {
        ptr = t;
    }
};

class Events;

class EventScope {
public:
    EventScope() = default;

    EventScope(Events* events);

    EventScope(const EventScope&) = delete;
    EventScope& operator=(const EventScope&) = delete;
    EventScope(EventScope&& other);
    EventScope& operator=(EventScope&& other);

    ~EventScope();

    void add_callback(int event, std::size_t ix);

    void add_aux(std::size_t ix);
    
    void add_event(int event);
private:
    Events* events {nullptr};

    std::forward_list<std::size_t> cb_indicies {};
    std::forward_list<std::size_t> aux_indicies {};
    std::forward_list<int> evt_indicies {};
};

class Events {
    friend class EventScope;
public:
    Events() noexcept;
    
    void begin_scope();

    EventScope end_scope();

    int register_event(EventType type, int vector_size = -1);

    void register_callback(int id, void (*callback)(EventInfo, void *),
                           void *aux);

    template <class T, class... Args>
    void register_callback(int id, void (*callback)(T, Args...), Args... args) {
        struct Wrapper : public WrapperBase {
            Wrapper(Args... args, void (*cb)(T, Args...))
                : args{args...}, cb{cb} {}
            std::tuple<Args...> args;
            void (*cb)(T, Args...);
        };
        add_aux(new Wrapper{args..., callback});
        auto call = [](EventInfo i, void *aux) {
            Wrapper &w = *reinterpret_cast<Wrapper *>(aux);
            w.cb(i.get<T>(), std::get<Args>(w.args)...);
        };
        register_callback(id, call, aux_data.back().get());
    }

    template <class... Args>
    void register_callback(int id, void (*callback)(Args...), Args... args) {
        struct Wrapper : public WrapperBase {
            Wrapper(Args... args, void (*cb)(Args...))
                : args{args...}, cb{cb} {}
            std::tuple<Args...> args;
            void (*cb)(Args...);
        };
        add_aux(new Wrapper{args..., callback});
        auto call = [](EventInfo i, void *aux) {
            Wrapper &w = *reinterpret_cast<Wrapper *>(aux);
            w.cb(std::get<Args>(w.args)...);
        };
        register_callback(id, call, aux_data.back().get());
    }

    template<class T>
    void notify_event(int id, T t) {
        EventInfo info;
        info.set<T>(t);
        notify_event(id, info);
    }

    void notify_event(int id, EventInfo data);

    void handle_events();

private:
    struct WrapperBase {
        virtual ~WrapperBase() = default;
    };

    void add_aux(WrapperBase* ptr);

    void remove_callback(int event, std::size_t ix);

    void remove_aux(std::size_t ix);

    void remove_event(int event);

    std::vector<std::unique_ptr<WrapperBase>> aux_data{};
    std::size_t free_aux = 0;

    struct CallbackData {
        void (*callback)(EventInfo, void *);
        void *aux;
    };
    struct EventData {
        EventType type;
        EventInfo data;
        bool triggered;
        std::vector<EventInfo> buffer{};
        std::vector<CallbackData> callbacks{};
        std::size_t free_ix = 0;
        int64_t last_ix = -1;
    };

    std::vector<EventData> events {};
    std::size_t free_ix = 1;
    int64_t last_ix = 0;

    std::vector<EventScope> scopes {};
};

#endif
