#ifndef PROC_ASM_PORT_H
#define PROC_ASM_PORT_H
#include <string>
#include <type_traits>

class InPort {
public:
    virtual void pop_data() noexcept = 0;

    virtual bool has_data() const noexcept = 0;

    virtual void flush_input() noexcept = 0;
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

    BytePort(const std::string& name) : name{name} {}
};

template<class I>
class NullByteInput final : public InBytePort<I> {
public:
    virtual I get_data() const noexcept override {
        return {};
    }

    virtual void pop_data() noexcept override {}

    virtual bool has_data() const noexcept override {
        return false;
    }

    virtual void flush_input() noexcept override {}
};

template <class O>
class NullByteOutput final : public OutBytePort<O> {
public:
    virtual void push_data(O o) noexcept override {}

    virtual bool has_space() const noexcept override {
        return false;
    }
};
template<class I> class ForwardingInputPort final : public InBytePort<I> {
public:
    const std::string name;

    ForwardingInputPort(const std::string& name) : name{name}, port{&null_port} {}

    virtual I get_data() const noexcept final {
        return port->get_data();
    }

    virtual bool has_data() const noexcept final {
        return port->has_data();
    }

    virtual void pop_data() noexcept final {
        port->pop_data();
    }

    virtual void flush_input() noexcept override {
        port->flush_input();
    }

    void set_port(InBytePort<I>* port) noexcept {
        if (port == nullptr) {
            this->port = &null_port;
        } else {
            this->port = port;
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

    ForwardingOutputPort(const std::string& name) : name{name}, port{&null_port} {}

    virtual void push_data(O t) noexcept override {
        port->push_data(t);
    }

    virtual bool has_space() const noexcept override {
        return port->has_space();
    }

    void set_port(OutBytePort<O>* port) noexcept {
        if (port == nullptr) {
            this->port = &null_port;
        } else {
            this->port = port;
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

    virtual bool has_data() const noexcept override {
        return !empty;
    }

    virtual bool has_space() const noexcept override {
        return empty;
    }

    virtual T get_data() const noexcept override {
        return element;
    }

    virtual void push_data(T t) noexcept override {
        element = t;
        empty = false;
    }

    virtual void pop_data() noexcept override {
        empty = true;
    }

    virtual void flush_input() noexcept override {
        pop_data();
    }

private:
    T element;
    bool empty;
};

#endif
