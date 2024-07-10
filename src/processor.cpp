#include "processor.h"
#include "engine/log.h"
#include "engine/engine.h"
#include <string>

Processor::Processor(uint32_t register_count) noexcept : gen_registers(register_count, 0), pc{0}, ticks{0} {
    instructions.push_back({InstructionType::NOP});
    instructions.back().line = 0;
}

bool Processor::compile_program(const std::vector<std::string> lines, std::vector<ErrorMsg>& errors) {
    LOG_DEBUG("COMPILE called");
    std::vector<std::string> in_names;
    std::vector<std::string> out_names;
    for (auto port: in_ports) {
        in_names.push_back(port.name);
    }
    for (auto port: out_ports) {
        out_names.push_back(port.name);
    }
    Compiler c {static_cast<uint32_t>(gen_registers.size()), std::move(in_names), std::move(out_names),
                instructions, BASIC_INSTRUCTIONS};
    valid = c.compile(lines, errors);
    if (!valid) {
        return false;
    }

    for (uint64_t i = 0; i < gen_registers.size(); ++i) {
        gen_registers[i] = 0;
        gEvents.notify_event(EventId::REGISTER_CHANGED, i);
    }

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
            if (in_ports[port].has_data()) {
                gen_registers[reg] = in_ports[port].get_data();
                in_ports[port].pop_data();
                ++pc;
                zero_flag = gen_registers[reg] == 0;
                gEvents.notify_event(EventId::REGISTER_CHANGED, reg);
            }
            break;
        }
        case InstructionType::OUT: {
            uint64_t port = instructions[pc].operands[1].port;
            uint64_t reg = instructions[pc].operands[0].reg;
            if (out_ports[port].has_space()) {
                out_ports[port].push_data(gen_registers[reg]);
                ++pc;
            }
            break;
        }
        case InstructionType::MOVE_REG: {
            uint64_t dest = instructions[pc].operands[0].reg;
            uint64_t src = instructions[pc].operands[1].reg;
            gen_registers[dest] = gen_registers[src];
            ++pc;
            zero_flag = gen_registers[dest] == 0;
            gEvents.notify_event(EventId::REGISTER_CHANGED, dest);
            break;
        }
        case InstructionType::MOVE_IMM: {
            uint64_t dest = instructions[pc].operands[0].reg;
            uint64_t val = instructions[pc].operands[1].imm_u;
            gen_registers[dest] = val;
            ++pc;
            zero_flag = gen_registers[dest] == 0;
            gEvents.notify_event(EventId::REGISTER_CHANGED, dest);
            break;
        }
        case InstructionType::ADD: {
            uint64_t dest = instructions[pc].operands[0].reg;
            uint64_t src = instructions[pc].operands[1].reg;
            gen_registers[dest] += gen_registers[src];
            ++pc;
            zero_flag = gen_registers[dest] == 0;
            gEvents.notify_event(EventId::REGISTER_CHANGED, dest);
            break;
        }
        case InstructionType::SUB: {
            uint64_t dest = instructions[pc].operands[0].reg;
            uint64_t src = instructions[pc].operands[1].reg;
            gen_registers[dest] -= gen_registers[src];
            ++pc;
            zero_flag = gen_registers[dest] == 0;
            gEvents.notify_event(EventId::REGISTER_CHANGED, dest);
            break;
        }
        case InstructionType::NOP: {
            ++pc;
            break;
        }
        case InstructionType::JEZ: {
            if (zero_flag) {
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
    zero_flag = false;

    for (auto& port: in_ports) {
        port.flush_input();
    }
    for (auto& port: out_ports) {
        port.flush_input();
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

