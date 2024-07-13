#ifndef PROC_ASM_PORT_H
#define PROC_ASM_PORT_H
#include <string>
#include <type_traits>
#include "engine/engine.h"
#include "cassert"

enum class PortDatatype {
    BYTE, WORD, DWORD, QWORD
};



constexpr inline std::size_t get_byte_size(PortDatatype type) {
    switch (type) {
        case PortDatatype::BYTE:
            return 1;
        case PortDatatype::WORD:
            return 2;
        case PortDatatype::DWORD:
            return 4;
        case PortDatatype::QWORD:
            return 8;
        default:
            return 0;
    }
}

template <class T>
constexpr inline PortDatatype from_c_type() {
    if (std::is_same_v<T, uint8_t>) {
        return PortDatatype::BYTE;
    } else if (std::is_same_v<T, uint16_t>) {
        return PortDatatype::WORD;
    } else if (std::is_same_v<T, uint32_t>) {
        return PortDatatype::DWORD;
    } else if (std::is_same_v<T, uint64_t>) {
        return PortDatatype::QWORD;
    }
}

enum class PortType {
    BLOCKING
};

class BytePort;

struct PortTemplate {
    std::string name;

    PortDatatype data_type;
    PortType port_type;

    std::unique_ptr<BytePort> instantiate() const;
};

template<class T>
constexpr bool is_valid_type() {
    return std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
           std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>;
}

class BytePort {
    const PortDatatype input_type {};
    const PortDatatype output_type {};

    const std::string name;
protected:
    event_t event = NULL_EVENT;
public:
    BytePort(std::string name, PortDatatype in, PortDatatype out) :
        name{std::move(name)}, input_type{in}, output_type{out} {}

    // Returns the type of input
    PortDatatype get_input_type() const noexcept { return input_type; }

    // Returns the type of output
    PortDatatype get_output_type() const noexcept { return output_type; }

    event_t register_event() noexcept {
        event = gEvents.register_event(EventType::UNIFIED);
        return event;
    }

    event_t get_event() {
        return event;
    }

    virtual ~BytePort() = default;

    virtual void pop_data() noexcept = 0;

    virtual bool has_data() const noexcept = 0;

    virtual void flush() noexcept = 0;

    template<class T>
    T get() const noexcept {
        static_assert(is_valid_type<T>, "Not specialized");
    }

    template<>
    uint8_t get() const noexcept { return get_byte(); }
    template<>
    uint16_t get() const noexcept { return get_word(); }
    template<>
    uint32_t get() const noexcept { return get_dword(); }
    template<>
    uint64_t get() const noexcept { return get_qword(); }

    std::string format() const noexcept {
        if (has_data()) {
            return std::to_string(get_qword());
        }
        return "-";
    }

    virtual uint8_t get_byte() const noexcept { return 0; }

    virtual uint16_t get_word() const noexcept { return 0; }

    virtual uint32_t get_dword() const noexcept { return 0; }

    virtual uint64_t get_qword() const noexcept { return 0; }

    virtual bool has_space() const noexcept = 0;

    template<class T>
    void push(T t) noexcept {
        static_assert(is_valid_type<T>, "Not specialized");
    }

    template<>
    void push(uint8_t v) noexcept { push_byte(v); } 
    template<>
    void push(uint16_t v) noexcept { push_word(v); } 
    template<>
    void push(uint32_t v) noexcept { push_dword(v); } 
    template<>
    void push(uint64_t v) noexcept { push_qword(v); } 

    virtual void push_byte(uint8_t v) noexcept {}

    virtual void push_word(uint16_t v) noexcept {}

    virtual void push_dword(uint32_t v) noexcept {}

    virtual void push_qword(uint64_t v) noexcept {}
    
    const std::string& get_name() const noexcept {
        return name;
    }
};


class NullBytePort final : public BytePort {
public:
    NullBytePort() : BytePort("NULL", PortDatatype::BYTE, PortDatatype::BYTE) {};

    void pop_data() noexcept override final {}

    bool has_data() const noexcept override final { return false; }

    void flush() noexcept override final {}

    bool has_space() const noexcept override final { return false; }
};


template<class T>
class ByteOutputSlot {
    static_assert(is_valid_type<T>(), "Invalid port type");
public:
    const std::string name;

    explicit ByteOutputSlot(std::string name) : name{std::move(name)}, port{&null_port} {
    }

    BytePort* check_port(BytePort* out_port) const noexcept {
        if (from_c_type<T>() != out_port->get_output_type()) {
            return nullptr;
        }
        return out_port;
    }

    T get() const noexcept {
        return port->get<T>();
    }

    uint8_t get_byte() const noexcept {
        return port->get_byte();
    }

    uint16_t get_word() const noexcept {
        return port->get_word();
    }

    uint32_t get_dword() const noexcept {
        return port->get_dword();
    }

    uint64_t get_qword() const noexcept {
        return port->get_qword();
    }

    bool has_data() const noexcept {
        return port->has_data();
    }

    void pop_data() noexcept {
        port->pop_data();
    }

    void set_port(BytePort* new_port) noexcept {
        if (new_port == nullptr) {
            this->port = &null_port;
        } else {
            this->port = new_port;
        }
    }

private:
    BytePort* port;

    static NullBytePort null_port;
};

template<class T>
NullBytePort ByteOutputSlot<T>::null_port{};

template<class T>
class ByteInputSlot {
    static_assert(is_valid_type<T>(), "Invalid port type");
public:
    const std::string name;

    explicit ByteInputSlot(std::string name) : name{std::move(name)}, port{&null_port} {}

    BytePort* check_port(BytePort* in_port) const noexcept {
        if (from_c_type<T>() != in_port->get_input_type()) {
            return nullptr;
        }
        return in_port;
    }

    bool has_space() const noexcept { return port->has_space(); }

    void push(T t) noexcept { port->push<T>(t); }

    void push_byte(uint8_t v) noexcept { port->push_byte(v); }

    void push_word(uint16_t v) noexcept { port->push_word(v); }

    void push_dword(uint32_t v) noexcept { port->push_dword(v); }

    void push_qword(uint64_t v) noexcept { port->push_qword(v); }

    void set_port(BytePort* new_port) noexcept {
        if (new_port == nullptr) {
            this->port = &null_port;
        } else {
            this->port = new_port;
        }
    }
private:
    BytePort* port;

    static NullBytePort null_port;
};

template<class T>
NullBytePort ByteInputSlot<T>::null_port{};

template<class T>
class BlockingBytePort : public BytePort {
    static_assert(is_valid_type<T>(), "Invalid port type");
public:
    BlockingBytePort(const std::string& name) : BytePort(name, from_c_type<T>(), from_c_type<T>()),
                                                element{}, empty{true} {}

    bool has_data() const noexcept override {
        return !empty;
    }

    void pop_data() noexcept override {
        empty = true;
        gEvents.notify_event(event, static_cast<T>(0));
    }

    bool has_space() const noexcept override {
        return empty;
    }

    uint8_t get_byte() const noexcept override {
        return static_cast<uint8_t>(element);
    }

    uint16_t get_word() const noexcept override {
        return static_cast<uint16_t>(element);
    }

    uint32_t get_dword() const noexcept override {
        return static_cast<uint32_t>(element);
    }

    uint64_t get_qword() const noexcept override {
        return static_cast<uint64_t>(element);
    }

    void push_byte(uint8_t v) noexcept override {
        push_qword(v);
    }

    void push_word(uint16_t v) noexcept override {
        push_qword(v);
    }

    void push_dword(uint32_t v) noexcept override {
        push_qword(v);
    }

    void push_qword(uint64_t v) noexcept override {
        element = v;
        empty = false;
        gEvents.notify_event(event, static_cast<T>(v));
    }

    void flush() noexcept override {
        pop_data();
    }

private:
    T element;
    bool empty;

};

inline std::unique_ptr<BytePort> PortTemplate::instantiate() const {
    if (port_type == PortType::BLOCKING) {
        switch (data_type) {
            case PortDatatype::BYTE:
                return std::make_unique<BlockingBytePort<uint8_t>>(name);
            case PortDatatype::WORD:
                return std::make_unique<BlockingBytePort<uint16_t>>(name);
            case PortDatatype::DWORD:
                return std::make_unique<BlockingBytePort<uint32_t>>(name);
            case PortDatatype::QWORD:
                return std::make_unique<BlockingBytePort<uint64_t>>(name);
        }
    }
    return {};
}
#endif
