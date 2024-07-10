#include "processor.h"
#include "engine/log.h"
#include "engine/engine.h"
#include <string>

RegisterFile::RegisterFile(uint16_t b, uint16_t w, uint16_t d, uint16_t q) : 
    byte_regs{b}, word_regs{w}, dword_regs{d}, qword_regs{q},
    gen_registers(b + w + d + q, 0){}

void RegisterFile::set_genreg(uint64_t ix, DataSize size, uint64_t val) {
    switch (size) {
        case DataSize::BYTE:
            gen_registers[ix] = val & 0xFF;
            break;
        case DataSize::WORD:
            gen_registers[ix] = val & 0xFFFF;
            break;
        case DataSize::DWORD:
            gen_registers[ix] = val & 0xFFFFFFFF;
            break;
        case DataSize::QWORD:
            gen_registers[ix] = val;
            break;
        default:
            assert(false);
    }
    gEvents.notify_event(EventId::REGISTER_CHANGED, ix);
    zero_flag = gen_registers[ix] == 0;
}

uint64_t RegisterFile::gen_reg_count() const {
    return gen_registers.size();
}

uint64_t RegisterFile::get_genreg(uint64_t reg) const {
    return gen_registers[reg];
}

void RegisterFile::clear() {
    uint64_t i = 0;
    for (; i < byte_regs + word_regs + dword_regs + qword_regs; ++i) {
        gen_registers[i] = 0;
        gEvents.notify_event(EventId::REGISTER_CHANGED, i);
    }
    zero_flag = 0;
}

bool RegisterFile::from_name(const std::string& name, uint64_t& ix, DataSize& size) const {
    for (uint64_t i = 0; i < gen_registers.size(); ++i) {
        std::string num = std::to_string(i);
        if ((name[0] == 'R' || name[0] == 'r') && strcmp(num.c_str(), name.c_str() + 1) == 0) {
            ix = i;
            if (i < byte_regs) {
                size = DataSize::BYTE;
            } else if (i < word_regs) {
                size = DataSize::WORD;
            } else if (i < dword_regs) {
                size = DataSize::DWORD;
            } else {
                size = DataSize::QWORD;
            }
            return true;
        }
    }
    return false;
}

std::string RegisterFile::to_name(uint64_t ix) const {
    return "R" + std::to_string(ix);
}

Processor::Processor(uint32_t register_count) noexcept : registers{4, 0, 0, 0}, pc{0}, ticks{0} {
    instructions.push_back({InstructionType::NOP});
    instructions.back().line = 0;
}

bool Processor::compile_program(const std::vector<std::string> lines, std::vector<ErrorMsg>& errors) {
    LOG_DEBUG("COMPILE called");
    std::vector<std::string> in_names;
    std::vector<std::string> out_names;
    for (auto port: in_ports) {
        in_names.push_back(port.get_name());
    }
    for (auto port: out_ports) {
        out_names.push_back(port.get_name());
    }
    Compiler c {registers, std::move(in_names), std::move(out_names),
                instructions, BASIC_INSTRUCTIONS};
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
    switch(instructions[pc].id) {
        case InstructionType::IN: {
            uint64_t port = instructions[pc].operands[1].port;
            uint64_t reg = instructions[pc].operands[0].reg;
            DataSize size = instructions[pc].operands[0].size;
            if (in_ports[port].has_data()) {
                switch(size) {
                    case DataSize::BYTE:
                        registers.set_genreg(reg, size, in_ports[port].get_byte());
                        break;
                    case DataSize::WORD:
                        registers.set_genreg(reg, size, in_ports[port].get_word());
                        break;
                    case DataSize::DWORD:
                        registers.set_genreg(reg, size, in_ports[port].get_dword());
                        break;
                    case DataSize::QWORD:
                        registers.set_genreg(reg, size, in_ports[port].get_qword());
                        break;
                    default:
                        assert(false);
                }
                in_ports[port].pop_data();
                ++pc;
            }
            break;
        }
        case InstructionType::OUT: {
            uint64_t port = instructions[pc].operands[1].port;
            uint64_t reg = instructions[pc].operands[0].reg;
            DataSize size = instructions[pc].operands[0].size;
            if (out_ports[port].has_space()) {
                uint64_t val = registers.get_genreg(reg);
                switch (size) {
                    case DataSize::BYTE:
                        out_ports[port].push_byte(val);
                        break;
                    case DataSize::WORD:
                        out_ports[port].push_word(val);
                        break;
                    case DataSize::DWORD:
                        out_ports[port].push_dword(val);
                        break;
                    case DataSize::QWORD:
                        out_ports[port].push_qword(val);
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
            if (registers.zero_flag) {
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
    gEvents.notify_event(EventId::TICKS_CHANGED, ticks);
}

event_t EventId::REGISTER_CHANGED = 0;
event_t EventId::TICKS_CHANGED = 0;
event_t EventId::RUNNING_CHANGED = 0;

void Processor::register_events() {
    EventId::REGISTER_CHANGED = gEvents.register_event(EventType::UNIFIED_VEC, MAX_REGISTERS);
    EventId::TICKS_CHANGED = gEvents.register_event(EventType::UNIFIED);
    EventId::RUNNING_CHANGED = gEvents.register_event(EventType::UNIFIED);
    for (auto& port: in_ports) {
        port.register_event();
    }
    for (auto& port: out_ports) {
        port.register_event();
    }
}

bool Processor::is_valid() const noexcept {
    return valid;
}

bool Processor::is_running() const noexcept {
    return running;
}

void Processor::reset() {
    running = false;
    pc = 0;
    ticks = 0;
    registers.clear();

    for (auto& port: in_ports) {
        port.flush();
    }
    for (auto& port: out_ports) {
        port.flush();
    }
    pc = 0;
    gEvents.notify_event(EventId::RUNNING_CHANGED, static_cast<uint64_t>(0));
    gEvents.notify_event(EventId::TICKS_CHANGED, ticks);
}

void Processor::invalidate() {
    valid = false;
    running = false;
    gEvents.notify_event(EventId::RUNNING_CHANGED, static_cast<uint64_t>(1));
}

