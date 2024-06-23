#include "processor.h"
#include "engine/log.h"
#include "engine/ui.h"
#include "config.h"
#include "channel.h"
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

    for (int i = 0; i < gen_registers.size(); ++i) {
        gen_registers[i] = 0;
        change_callback(ProcessorChange::REGISTER, i);
    }
    running = false;
    pc = 0;
    ticks = 0;
    zero_flag = false;

    for (int i = 0; i < in_ports.size(); ++i) {
        in_ports[i].flush_input();
        change_callback(ProcessorChange::IN_PORT, i);
    }

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
            int port = instructions[pc].operands[1].port;
            int reg = instructions[pc].operands[0].reg;
            if (in_ports[port].has_data()) {
                gen_registers[reg] = in_ports[port].get_data();
                in_ports[port].pop_data();
                ++pc;
                zero_flag = gen_registers[reg] == 0;
                change_callback(ProcessorChange::REGISTER, reg);
                change_callback(ProcessorChange::IN_PORT, port);
            }
            break;
        }
        case InstructionType::OUT: {
            int port = instructions[pc].operands[1].port;
            int reg = instructions[pc].operands[0].reg;
            if (out_ports[port].has_space()) {
                out_ports[port].push_data(gen_registers[reg]);
                ++pc;
                change_callback(ProcessorChange::OUT_PORT, port);
            }
            break;
        }
        case InstructionType::MOVE_REG: {
            int dest = instructions[pc].operands[0].reg;
            int src = instructions[pc].operands[1].reg;
            gen_registers[dest] = gen_registers[src];
            ++pc;
            zero_flag = gen_registers[dest] == 0;
            change_callback(ProcessorChange::REGISTER, dest);
            break;
        }
        case InstructionType::MOVE_IMM: {
            int dest = instructions[pc].operands[0].reg;
            uint64_t val = instructions[pc].operands[1].imm_u;
            gen_registers[dest] = val;
            ++pc;
            zero_flag = gen_registers[dest] == 0;
            change_callback(ProcessorChange::REGISTER, dest);
            break;
        }
        case InstructionType::ADD: {
            int dest = instructions[pc].operands[0].reg;
            int src = instructions[pc].operands[1].reg;
            gen_registers[dest] += gen_registers[src];
            ++pc;
            zero_flag = gen_registers[dest] == 0;
            change_callback(ProcessorChange::REGISTER, dest);
            break;
        }
        case InstructionType::SUB: {
            int dest = instructions[pc].operands[0].reg;
            int src = instructions[pc].operands[1].reg;
            gen_registers[dest] -= gen_registers[src];
            ++pc;
            zero_flag = gen_registers[dest] == 0;
            change_callback(ProcessorChange::REGISTER, dest);
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
    change_callback(ProcessorChange::TICKS, 0);
}

void Processor::register_change_callback(std::function<void(ProcessorChange, uint32_t)> fn) {
    change_callback = fn;
}

bool Processor::is_valid() const noexcept {
    return valid;
}

bool Processor::is_running() const noexcept {
    return running;
}

void Processor::invalidate() {
    valid = false;
    running = false;
    change_callback(ProcessorChange::RUNNING, 0);
}

std::vector<InPort*> Processor::get_inputs() {
    std::vector<InPort*> res {};
    res.resize(in_ports.size());
    for (std::size_t i = 0; i < in_ports.size(); ++i) {
        res[i] = &in_ports[i];
    }
    return res;
}

std::vector<OutPort*> Processor::get_outputs() {
    std::vector<OutPort*> res {};
    res.resize(out_ports.size());
    for (std::size_t i = 0; i < out_ports.size(); ++i) {
        res[i] = &out_ports[i];
    }
    return res;
}

