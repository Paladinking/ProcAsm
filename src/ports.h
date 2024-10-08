#ifndef PROC_ASM_PORT_H
#define PROC_ASM_PORT_H
#include "cassert"
#include "engine/log.h"
#include "json.h"
#include <memory>
#include <string>
#include <type_traits>

enum class PortDatatype { BYTE = 0, WORD = 1, DWORD = 2, QWORD = 3 };

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

template <class T> constexpr inline PortDatatype from_c_type() {
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

enum class PortType { BLOCKING = 0, INSTANT = 1 };

class BytePort;
class SharedPort;

struct PortTemplate {
    std::string name;

    PortDatatype data_type;
    PortType port_type;

    bool read_from_json(const JsonObject &obj) {
        if (!obj.has_key_of_type<std::string>("name")) {
            return false;
        }
        if (!obj.has_key_of_type<std::string>("data_type")) {
            return false;
        }
        if (!obj.has_key_of_type<std::string>("port_type")) {
            return false;
        }
        name = obj.get<std::string>("name");
        const std::string &dt = obj.get<std::string>("data_type");
        const std::string &pt = obj.get<std::string>("port_type");
        auto eq = [](const std::string &a, const std::string &b) -> bool {
            if (a.size() != b.size()) {
                return false;
            }
            for (std::size_t i = 0; i < a.size(); ++i) {
                if (std::toupper(a[i]) != b[i]) {
                    return false;
                }
            }
            return true;
        };
        if (eq(dt, "WORD")) {
            data_type = PortDatatype::WORD;
        } else if (eq(dt, "DWORD")) {
            data_type = PortDatatype::DWORD;
        } else if (eq(dt, "QWORD")) {
            data_type = PortDatatype::QWORD;
        } else if (eq(dt, "BYTE")) {
            data_type = PortDatatype::BYTE;
        } else {
            LOG_WARNING("Invalid port data type '%s'", dt.c_str());
            data_type = PortDatatype::BYTE;
        }
        if (eq(pt, "BLOCKING")) {
            port_type = PortType::BLOCKING;
        } else {
            LOG_WARNING("Invalid port type '%s'", pt.c_str());
            port_type = PortType::BLOCKING;
        }

        return true;
    }

    std::shared_ptr<SharedPort> instantiate() const;
};

template <class T> constexpr bool is_valid_type() {
    return std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
           std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>;
}

class SharedPort {
public:
    virtual ~SharedPort() = default;

    virtual void prepare_pop() noexcept = 0;

    bool pop_byte(uint8_t &v) noexcept {
        uint64_t val;
        if (pop(val)) {
            v = val & 0xff;
            return true;
        }
        return false;
    }

    bool pop_word(uint16_t &v) noexcept {
        uint64_t val;
        if (pop(val)) {
            v = val & 0xffff;
            return true;
        }
        return false;
    }

    bool pop_dword(uint32_t &v) noexcept {
        uint64_t val;
        if (pop(val)) {
            v = val & 0xffffffff;
            return true;
        }
        return false;
    }

    bool pop_qword(uint64_t &v) noexcept { return pop(v); }

    virtual bool push_byte(uint8_t v) noexcept { return push(v); }

    virtual bool push_word(uint16_t v) noexcept { return push(v); }

    virtual bool push_dword(uint32_t v) noexcept { return push(v); };

    virtual bool push_qword(uint64_t v) noexcept { return push(v); }

    virtual void flush() noexcept = 0;

protected:
    virtual bool push(uint64_t v) noexcept = 0;

    virtual bool pop(uint64_t &v) noexcept = 0;
};

class BufferdSharedPort : public SharedPort {
    virtual void flush() noexcept override {
        has_data = false;
        pending_reader = false;
        delay = false;
    }

    virtual void prepare_pop() noexcept override {
        pending_reader = true;
    }

    virtual bool push(uint64_t v) noexcept override {
        if (has_data) {
            return false;
        }
        has_data = true;
        data = v;
        if (pending_reader) {
            delay = true;
        }
        return true;
    }

    virtual bool pop(uint64_t &v) noexcept override {
        pending_reader = false;
        if (!has_data) {
            return false;
        }
        if (delay) {
            delay = false;
            return false;
        }
        v = data;
        has_data = false;
        return true;
    }

private:
    uint64_t data = 0;
    bool pending_reader = false;
    bool has_data = false;
    bool delay = false;
};

class DirectSharedPort : public SharedPort {
public:
    virtual void flush() noexcept override {
        pending_writer = false;
        pending_reader = false;
    }

    virtual void prepare_pop() noexcept override {
        pending_writer = false;
        pending_reader = true;
    }

    virtual bool push(uint64_t v) noexcept override {
        if (!pending_reader) {
            return false;
        }
        data = v;
        pending_writer = true;
        return true;
    }

    virtual bool pop(uint64_t &v) noexcept override {
        pending_reader = false;
        if (pending_writer) {
            v = data;
            pending_writer = false;
            return true;
        }
        return false;
    }

private:
    uint64_t data = 0;
    bool pending_reader = false;
    bool pending_writer = false;
};

class BytePort {
    const PortDatatype input_type{};
    const PortDatatype output_type{};

    const std::string name;

protected:
    bool changed = false;

public:
    BytePort(std::string name, PortDatatype in, PortDatatype out)
        : name{std::move(name)}, input_type{in}, output_type{out} {}

    // Returns the type of input
    PortDatatype get_input_type() const noexcept { return input_type; }

    // Returns the type of output
    PortDatatype get_output_type() const noexcept { return output_type; }

    virtual ~BytePort() = default;

    virtual void pop_data() noexcept = 0;

    virtual bool has_data() const noexcept = 0;

    virtual void flush() noexcept = 0;

    bool format_new(std::string &s) {
        if (changed) {
            s = format();
            changed = false;
            return true;
        }
        return false;
    }

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

    virtual void push_byte(uint8_t v) noexcept {}

    virtual void push_word(uint16_t v) noexcept {}

    virtual void push_dword(uint32_t v) noexcept {}

    virtual void push_qword(uint64_t v) noexcept {}

    const std::string &get_name() const noexcept { return name; }
};

class NullBytePort final : public BytePort {
public:
    NullBytePort() : BytePort("NULL", PortDatatype::BYTE, PortDatatype::BYTE){};

    void pop_data() noexcept override final {}

    bool has_data() const noexcept override final { return false; }

    void flush() noexcept override final {}

    bool has_space() const noexcept override final { return false; }
};

template <class T> class ByteOutputSlot {
    static_assert(is_valid_type<T>(), "Invalid port type");

public:
    const std::string name;

    explicit ByteOutputSlot(std::string name)
        : name{std::move(name)}, port{&null_port} {}

    BytePort *check_port(BytePort *out_port) const noexcept {
        if (from_c_type<T>() != out_port->get_output_type()) {
            return nullptr;
        }
        return out_port;
    }

    uint8_t get_byte() const noexcept { return port->get_byte(); }

    uint16_t get_word() const noexcept { return port->get_word(); }

    uint32_t get_dword() const noexcept { return port->get_dword(); }

    uint64_t get_qword() const noexcept { return port->get_qword(); }

    bool has_data() const noexcept { return port->has_data(); }

    void pop_data() noexcept { port->pop_data(); }

    void set_port(BytePort *new_port) noexcept {
        if (new_port == nullptr) {
            this->port = &null_port;
        } else {
            this->port = new_port;
        }
    }

private:
    BytePort *port;

    static NullBytePort null_port;
};

template <class T> NullBytePort ByteOutputSlot<T>::null_port{};

template <class T> class ByteInputSlot {
    static_assert(is_valid_type<T>(), "Invalid port type");

public:
    const std::string name;

    explicit ByteInputSlot(std::string name)
        : name{std::move(name)}, port{&null_port} {}

    BytePort *check_port(BytePort *in_port) const noexcept {
        if (from_c_type<T>() != in_port->get_input_type()) {
            return nullptr;
        }
        return in_port;
    }

    bool has_space() const noexcept { return port->has_space(); }

    void push_byte(uint8_t v) noexcept { port->push_byte(v); }

    void push_word(uint16_t v) noexcept { port->push_word(v); }

    void push_dword(uint32_t v) noexcept { port->push_dword(v); }

    void push_qword(uint64_t v) noexcept { port->push_qword(v); }

    void set_port(BytePort *new_port) noexcept {
        if (new_port == nullptr) {
            this->port = &null_port;
        } else {
            this->port = new_port;
        }
    }

private:
    BytePort *port;

    static NullBytePort null_port;
};

template <class T> NullBytePort ByteInputSlot<T>::null_port{};

template <class T> class BlockingBytePort : public BytePort {
    static_assert(is_valid_type<T>(), "Invalid port type");

public:
    BlockingBytePort(const std::string &name)
        : BytePort(name, from_c_type<T>(), from_c_type<T>()), element{},
          empty{true} {}

    bool has_data() const noexcept override { return !empty; }

    void pop_data() noexcept override {
        empty = true;
        changed = true;
    }

    bool has_space() const noexcept override { return empty; }

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

    void push_byte(uint8_t v) noexcept override { push_qword(v); }

    void push_word(uint16_t v) noexcept override { push_qword(v); }

    void push_dword(uint32_t v) noexcept override { push_qword(v); }

    void push_qword(uint64_t v) noexcept override {
        element = v;
        empty = false;
        changed = true;
    }

    void flush() noexcept override { pop_data(); }

private:
    T element;
    bool empty;
};

inline std::shared_ptr<SharedPort> PortTemplate::instantiate() const {
    if (port_type == PortType::BLOCKING) {
        return std::make_shared<BufferdSharedPort>();
    } else if (port_type == PortType::INSTANT) {
        return std::make_shared<DirectSharedPort>();
    }
    return {};
}
#endif
