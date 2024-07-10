#ifndef PROC_ASM_PORT_H
#define PROC_ASM_PORT_H
#include <string>
#include <type_traits>
#include "engine/engine.h"

class InPort {
public:
    virtual void pop_data() noexcept = 0;

    virtual bool has_data() const noexcept = 0;

    virtual void flush_input() noexcept = 0;

    virtual const std::string& get_name() const noexcept = 0;
};

class OutPort {
public:
    virtual bool has_space() const noexcept = 0;
};

template <class I>
class InBytePort : public InPort {
public:
    virtual I get_data() const noexcept = 0;

    static_assert(std::is_integral_v<I>, "Type must be integral");
};

template <class O>
class OutBytePort : public OutPort {
public:
    virtual void push_data(O t) noexcept = 0;
    
    static_assert(std::is_integral_v<O>, "Type must be integral");
};

template<class I, class O>
class BytePort : public InBytePort<I>, public OutBytePort<O> {
public:
    const std::string name;

    explicit BytePort(std::string name) : name{std::move(name)} {}
};

template<class I>
class NullByteInput final : public InBytePort<I> {
public:
    const std::string& get_name() const noexcept override {
        return name;
    }

    I get_data() const noexcept override {
        return {};
    }

    void pop_data() noexcept override {}

    bool has_data() const noexcept override {
        return false;
    }

    void flush_input() noexcept override {}

    static const std::string name;
};

template<class I>
const std::string NullByteInput<I>::name = "NULL";

template <class O>
class NullByteOutput final : public OutBytePort<O> {
public:
    void push_data(O o) noexcept override {}

    bool has_space() const noexcept override {
        return false;
    }
};
template<class I> class ForwardingInputPort final : public InBytePort<I> {
public:
    const std::string name;

    explicit ForwardingInputPort(std::string name) : name{std::move(name)}, port{&null_port} {}

    virtual const std::string& get_name() const noexcept final {
        return name;
    }

    InBytePort<I>* check_port(InPort* in_port) const noexcept {
        return dynamic_cast<InBytePort<I>*>(in_port);
    }

    virtual I get_data() const noexcept final {
        return port->get_data();
    }

    virtual bool has_data() const noexcept final {
        return port->has_data();
    }

    virtual void pop_data() noexcept final {
        port->pop_data();
    }

    virtual void flush_input() noexcept final {
        port->flush_input();
    }

    void set_port(InBytePort<I>* new_port) noexcept {
        if (new_port == nullptr) {
            this->port = &null_port;
        } else {
            this->port = new_port;
        }
    }

private:
    InBytePort<I>* port;

    static NullByteInput<I> null_port;
};

template<class I>
NullByteInput<I> ForwardingInputPort<I>::null_port{};

template<class O>
class ForwardingOutputPort final : public OutBytePort<O> {
public:
    const std::string name;

    explicit ForwardingOutputPort(std::string name) : name{std::move(name)}, port{&null_port} {}

    OutBytePort<O>* check_port(OutPort* in_port) const noexcept {
        return dynamic_cast<OutBytePort<O>*>(in_port);
    }

    void push_data(O t) noexcept override {
        port->push_data(t);
    }

    bool has_space() const noexcept override {
        return port->has_space();
    }

    void set_port(OutBytePort<O>* new_port) noexcept {
        if (new_port == nullptr) {
            this->port = &null_port;
        } else {
            this->port = new_port;
        }
    }
private:
    OutBytePort<O>* port;

    static NullByteOutput<O> null_port;
};

template<class O>
NullByteOutput<O> ForwardingOutputPort<O>::null_port{};

template<class T>
class BlockingBytePort : public BytePort<T, T> {
public:
    BlockingBytePort(const std::string& name) : BytePort<T, T>(name), element{}, empty{true} {}

    int register_event() {
        event = gEvents.register_event(EventType::UNIFIED);
        return event;
    }

    int get_event() {
        return event;
    }

    const std::string& get_name() const noexcept override {
        return BytePort<T, T>::name;
    }

    bool has_data() const noexcept override {
        return !empty;
    }

    bool has_space() const noexcept override {
        return empty;
    }

    T get_data() const noexcept override {
        return element;
    }

    void push_data(T t) noexcept override {
        element = t;
        empty = false;
        gEvents.notify_event(event, t);
    }

    void pop_data() noexcept override {
        empty = true;
        gEvents.notify_event(event, static_cast<T>(0));
    }

    void flush_input() noexcept override {
        pop_data();
        gEvents.notify_event(event, static_cast<T>(0));
    }

private:
    T element;
    bool empty;

    event_t event = NULL_EVENT;
};

#endif
