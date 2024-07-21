#include "processor.h"
#include "engine/engine.h"
#include "engine/log.h"
#include "json.h"
#include <string>

RegisterFile::RegisterFile(RegisterNames names, flag_t enabled_flags)
    : register_names{std::move(names)}, enabled_flags{enabled_flags},
      gen_registers(register_names.size(), {0, true}) {}

void RegisterFile::set_genreg(uint64_t ix, DataSize size, uint64_t val) {
    switch (size) {
    case DataSize::BYTE:
        gen_registers[ix].val = val & 0xFF;
        break;
    case DataSize::WORD:
        gen_registers[ix].val = val & 0xFFFF;
        break;
    case DataSize::DWORD:
        gen_registers[ix].val = val & 0xFFFFFFFF;
        break;
    case DataSize::QWORD:
        gen_registers[ix].val = val;
        break;
    default:
        assert(false);
    }
    gen_registers[ix].changed = true;
    flags = (flags & ~FLAG_ZERO_MASK) | 
            ((gen_registers[ix].val == 0) << FLAG_ZERO_IX);
    LOG_DEBUG("Flags: %d", flags);
}

uint64_t RegisterFile::gen_reg_count() const { return gen_registers.size(); }

uint64_t RegisterFile::get_genreg(uint64_t reg) const {
    return gen_registers[reg].val;
}

void RegisterFile::clear() {
    for (uint64_t i = 0; i < gen_registers.size(); ++i) {
        gen_registers[i].val = 0;
        gen_registers[i].changed = true;
    }
    flags = 0;
}

bool RegisterFile::from_name(const std::string &name, uint64_t &ix,
                             DataSize &size) const {
    for (uint64_t i = 0; i < register_names.size(); ++i) {
        if (name == register_names[i].first) {
            ix = i;
            size = register_names[i].second;
            return true;
        }
    }
    return false;
}

const std::string &RegisterFile::to_name(uint64_t ix) const {
    return register_names[ix].first;
}

bool RegisterFile::poll_value(uint64_t ix, std::string& s) {
    if (gen_registers[ix].changed) {
        s = to_name(ix) + ": " + std::to_string(gen_registers[ix].val);
        gen_registers[ix].changed = false;
        return true;
    }
    return false;
}


InstructionSlotType instruction_from_ix(int64_t i) {
    if (i >= static_cast<int64_t>(InstructionSlotType::NOP) || i < 0) {
        return InstructionSlotType::NOP;
    }
    return static_cast<InstructionSlotType>(i);
}

bool ProcessorTemplate::read_from_json(const JsonObject& obj) {
    if (!obj.has_key_of_type<std::string>("name")) {
        return false;
    }
    if (!obj.has_key_of_type<JsonList>("in_ports")) {
        return false;
    }
    if (!obj.has_key_of_type<JsonList>("out_ports")) {
        return false;
    }
    if (!obj.has_key_of_type<JsonList>("registers")) {
        return false;
    }
    if (!obj.has_key_of_type<int64_t>("max_instructions")) {
        return false;
    }
    if (!obj.has_key_of_type<JsonObject>("instructions")) {
        return false;
    }
    if (!obj.has_key_of_type<std::string>("flags")) {
        return false;
    }
    if (!obj.has_key_of_type<int64_t>("features")) {
        return false;
    }

    name = obj.get<std::string>("name");
    instruction_slots = obj.get<int64_t>("max_instructions");
    supported_flags = 0;
    for (auto c: obj.get<std::string>("flags")) { 
        if (c == 'z' || c == 'Z') {
            supported_flags |= FLAG_ZERO_MASK;
        } else if (c == 'c' || c == 'C') {
            supported_flags |= FLAG_CARRY_MASK;
        }
    }
    features = obj.get<int64_t>("features") & ProcessorFeature::ALL;

    const JsonList& in_ports = obj.get<JsonList>("in_ports");
    this->in_ports.clear();
    for (const auto& val: in_ports) {
        if (const JsonObject* port = val.get<JsonObject>()) {
            PortTemplate t;
            if (!t.read_from_json(*port)) {
                continue;
            }
            this->in_ports.push_back(t);
        }
    }
    const JsonList& out_ports = obj.get<JsonList>("out_ports");
    this->out_ports.clear();
    for (const auto& val: out_ports) {
        if (const JsonObject* port = val.get<JsonObject>()) {
            PortTemplate t;
            if (!t.read_from_json(*port)) {
                continue;
            }
            this->out_ports.push_back(t);
        }
    }
    const JsonList& registers = obj.get<JsonList>("registers");
    this->register_names.clear();
    for (const auto& val: registers) {
        if (const JsonObject* reg = val.get<JsonObject>()) {
            if (!reg->has_key_of_type<std::string>("name")) {
                continue;
            }
            if (!reg->has_key_of_type<std::string>("type")) {
                continue;
            }
            std::string name = reg->get<std::string>("name");
            for (auto& c: name) {
                c = std::toupper(c);
            }
            std::string type = reg->get<std::string>("type");
            for (auto& c: type) {
                c = std::toupper(c);
            }
            DataSize size;
            if (type == "BYTE") {
                size = DataSize::BYTE;
            } else if (type == "WORD") {
                size = DataSize::WORD;
            } else if (type == "DWORD") {
                size = DataSize::DWORD;
            } else if (type == "QWORD") {
                size = DataSize::QWORD;
            } else {
                LOG_WARNING("Invalid register type: '%s'", type.c_str());
                size = DataSize::BYTE;
            }
            register_names.push_back({name, size});
        }
    }
    const JsonObject& instr = obj.get<JsonObject>("instructions");
    instruction_set.clear();
    for (const auto& kv: instr) {
        if (const int64_t* i = kv.second.get<int64_t>()) {
            std::string key = kv.first;
            for (auto& c : key) {
                c = std::toupper(c);
            }
            instruction_set[key] = instruction_from_ix(*i);
        }
    }

    return true;
}

Processor ProcessorTemplate::instantiate() const {
    LOG_DEBUG("Name: %s", name.c_str());
    std::vector<std::unique_ptr<BytePort>> in{}, out{};
    for (auto i : in_ports) {
        in.push_back(std::move(i.instantiate()));
    }
    for (auto o : out_ports) {
        out.push_back(std::move(o.instantiate()));
    }
    return {std::move(in), std::move(out), instruction_set, {register_names, supported_flags}, features};
}

Processor::Processor(std::vector<std::unique_ptr<BytePort>> in_ports,
                     std::vector<std::unique_ptr<BytePort>> out_ports,
                     InstructionSet instruction_set,
                     RegisterFile registers, feature_t features) noexcept
    : in_ports{std::move(in_ports)}, out_ports{std::move(out_ports)},
      instruction_set{std::move(instruction_set)},
      registers{std::move(registers)}, pc{0}, ticks{0}, features{features} {
          LOG_DEBUG("Size: %d", instruction_set.size());
    instructions.push_back({InstructionType::NOP});
    instructions.back().line = 0;
}

bool Processor::compile_program(const std::vector<std::string> lines,
                                std::vector<ErrorMsg> &errors) {
    LOG_DEBUG("COMPILE called");
    std::vector<std::string> in_names;
    std::vector<std::string> out_names;
    for (auto &port : in_ports) {
        in_names.push_back(port->get_name());
    }
    for (auto &port : out_ports) {
        out_names.push_back(port->get_name());
    }
    Compiler c{registers, std::move(in_names), std::move(out_names),
               instructions, instruction_set, features};
    valid = c.compile(lines, errors);
    if (!valid) {
        return false;
    }

    registers.clear();

    reset();

    if (instructions.size() == 0) {
        instructions.push_back({InstructionType::NOP});
        instructions.back().line = 0;
    }

    return true;
}

void Processor::clock_tick() {
    if (!valid) {
        return;
    }
    switch (instructions[pc].id) {
    case InstructionType::IN: {
        uint64_t port = instructions[pc].operands[1].port;
        uint64_t reg = instructions[pc].operands[0].reg;
        DataSize size = instructions[pc].operands[0].size;
        if (in_ports[port]->has_data()) {
            switch (size) {
            case DataSize::BYTE:
                registers.set_genreg(reg, size, in_ports[port]->get_byte());
                break;
            case DataSize::WORD:
                registers.set_genreg(reg, size, in_ports[port]->get_word());
                break;
            case DataSize::DWORD:
                registers.set_genreg(reg, size, in_ports[port]->get_dword());
                break;
            case DataSize::QWORD:
                registers.set_genreg(reg, size, in_ports[port]->get_qword());
                break;
            default:
                assert(false);
            }
            in_ports[port]->pop_data();
            ++pc;
        }
        break;
    }
    case InstructionType::OUT: {
        uint64_t port = instructions[pc].operands[1].port;
        uint64_t reg = instructions[pc].operands[0].reg;
        DataSize size = instructions[pc].operands[0].size;
        if (out_ports[port]->has_space()) {
            uint64_t val = registers.get_genreg(reg);
            switch (size) {
            case DataSize::BYTE:
                out_ports[port]->push_byte(val);
                break;
            case DataSize::WORD:
                out_ports[port]->push_word(val);
                break;
            case DataSize::DWORD:
                out_ports[port]->push_dword(val);
                break;
            case DataSize::QWORD:
                out_ports[port]->push_qword(val);
                break;
            default:
                assert(false);
            }
            ++pc;
        }
        break;
    }
    case InstructionType::MOVE_REG: {
        uint64_t dest = instructions[pc].operands[0].reg;
        uint64_t src = instructions[pc].operands[1].reg;
        DataSize size = instructions[pc].operands[0].size;
        registers.set_genreg(dest, size, registers.get_genreg(src));
        ++pc;
        break;
    }
    case InstructionType::MOVE_IMM: {
        uint64_t dest = instructions[pc].operands[0].reg;
        uint64_t val = instructions[pc].operands[1].imm_u;
        DataSize size = instructions[pc].operands[0].size;
        registers.set_genreg(dest, size, val);
        ++pc;
        break;
    }
    case InstructionType::ADD: {
        uint64_t dest = instructions[pc].operands[0].reg;
        uint64_t src = instructions[pc].operands[1].reg;
        DataSize size = instructions[pc].operands[0].size;
        uint64_t val = registers.get_genreg(dest) + registers.get_genreg(src);
        registers.set_genreg(dest, size, val);
        ++pc;
        break;
    }
    case InstructionType::SUB: {
        uint64_t dest = instructions[pc].operands[0].reg;
        uint64_t src = instructions[pc].operands[1].reg;
        DataSize size = instructions[pc].operands[0].size;
        uint64_t val = registers.get_genreg(dest) - registers.get_genreg(src);
        registers.set_genreg(dest, size, val);
        ++pc;
        break;
    }
    case InstructionType::NOP: {
        ++pc;
        break;
    }
    case InstructionType::JEZ: {
        if (registers.flags & FLAG_ZERO_MASK) {
            pc = instructions[pc].operands[0].label;
        } else {
            ++pc;
        }
        break;
    }
    }
    ++ticks;
    if (pc == instructions.size()) {
        pc = 0;
    }
}

bool Processor::is_valid() const noexcept { return valid; }

bool Processor::is_running() const noexcept { return running; }

void Processor::reset() {
    running = false;
    pc = 0;
    ticks = 0;
    registers.clear();

    for (auto &port : in_ports) {
        port->flush();
    }
    for (auto &port : out_ports) {
        port->flush();
    }
    pc = 0;
}

void Processor::invalidate() {
    valid = false;
    running = false;
}
