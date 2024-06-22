#include "processor.h"
#include "engine/log.h"
#include "engine/ui.h"
#include "config.h"
#include <string>

InputPort::InputPort(const std::string& name) noexcept : name(name) {}

OutputPort::OutputPort(const std::string& name) noexcept : name(name) {}

Processor::Processor(uint32_t register_count) noexcept : gen_registers(register_count, 0), pc{0} {}

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
    bool success = c.compile(lines, errors);
    if (!success) {
        return false;
    }

    for (int i = 0; i < gen_registers.size(); ++i) {
        gen_registers[i] = 0;
        change_callback(ProcessorChange::REGISTER, i);
    }
    pc = 0;
    zero_flag = false;

    for (int i = 0; i < in_ports.size(); ++i) {
        in_ports[i].reset();
        change_callback(ProcessorChange::IN_PORT, i);
    }
    for (int i = 0; i < out_ports.size(); ++i) {
        out_ports[i].reset();
        change_callback(ProcessorChange::IN_PORT, i);
    }

    if (instructions.size() == 0) {
        instructions.push_back({InstructionType::NOP});
    }

    return true;
}

void Processor::clock_tick() {
    switch(instructions[pc].id) {
        case InstructionType::IN: {
            int port = instructions[pc].operands[1].port;
            int reg = instructions[pc].operands[0].reg;
            if (in_ports[port].has_val()) {
                gen_registers[reg] = in_ports[port].get_val();
                in_ports[port].pop_val();
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
                out_ports[port].push_val(gen_registers[reg]);
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
    if (pc == instructions.size()) {
        pc = 0;
    }
}

void Processor::register_change_callback(std::function<void(ProcessorChange, uint32_t)> fn) {
    change_callback = fn;
}

ProcessorGui::ProcessorGui() {}
ProcessorGui::ProcessorGui(Processor* processor, int x, int y, WindowState* window_state) : processor{processor},
        x{x}, y{y}, window_state{window_state} {
    if (processor != nullptr) {
        std::function<void(ProcessorChange, uint32_t)> f {std::bind(&ProcessorGui::change_callback, this,
                                     std::placeholders::_1, std::placeholders::_2)};
        processor->register_change_callback(f);
        for (uint32_t i = 0; i < processor->gen_registers.size(); ++i) {
            registers.emplace_back(5, 5 + BOX_LINE_HEIGHT * i, 8, BOX_LINE_HEIGHT, "R" + std::to_string(i) + ": 0", *window_state);
            registers.back().set_left_align(true);
            registers.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
        }
        for (uint32_t i = 0; i < processor->in_ports.size(); ++i) {
            std::string s = processor->in_ports[i].has_val() ? std::to_string(processor->in_ports[i].get_val()) : "";
            in_ports.emplace_back(30 + 110 * i, -50, 80, BOX_LINE_HEIGHT, "In " + processor->in_ports[i].name, *window_state);
            in_ports.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
            in_ports.emplace_back(30 + 110 * i, -50 + BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, s, *window_state);
            in_ports.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
        }
        for (uint32_t i = 0; i < processor->out_ports.size(); ++i) {
            out_ports.emplace_back(30 + 110 * i, BOX_SIZE + 10, 80, BOX_LINE_HEIGHT, "Out " + processor->out_ports[i].name, *window_state);
            out_ports.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
            out_ports.emplace_back(30 + 110 * i, BOX_SIZE + 10 + BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, "", *window_state);
            out_ports.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
        }
    }
}

void ProcessorGui::render() const {
    SDL_Rect rect {x + BOX_SIZE - 80, y, 80, 100};
    SDL_RenderDrawRect(gRenderer, &rect);
    if (processor != nullptr) {
        for (const auto& b : registers) {
            b.render(x + BOX_SIZE - 80, y, *window_state);
        }
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        for (int i = 0; i < processor->in_ports.size(); ++i) {
            SDL_Rect rect = {x + 30 + 110 * i, y - 60, 80, 60};
            SDL_RenderDrawRect(gRenderer, &rect);
        }
        for (int i = 0; i < processor->out_ports.size(); ++i) {
            SDL_Rect rect = {x + 30 + 110 * i, y  + BOX_SIZE, 80, 60};
            SDL_RenderDrawRect(gRenderer, &rect);

        }
        for (const auto&b : in_ports) {
            b.render(x, y, *window_state);
        }
        for (const auto& b: out_ports) {
            b.render(x, y, *window_state);
        }
        
        rect = {x + 4, y + BOX_TEXT_MARGIN + BOX_LINE_HEIGHT * static_cast<int>(processor->pc) + 7, 6, 6};
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        SDL_RenderFillRect(gRenderer, &rect);
    }
}

void ProcessorGui::set_dpi(double dpi_scale) {
    for (auto& b: registers) {
        b.set_dpi_ratio(dpi_scale);
    }
    for (auto& b: in_ports) {
        b.set_dpi_ratio(dpi_scale);
    }
    for (auto& b: out_ports) {
        b.set_dpi_ratio(dpi_scale);
    }
}

void ProcessorGui::change_callback(ProcessorChange change, uint32_t reg) {
    if (change == ProcessorChange::REGISTER) {
        std::string name = "R" + std::to_string(reg);
        registers[reg].set_text(name + ": " + std::to_string(processor->gen_registers[reg]));
    } else if (change == ProcessorChange::OUT_PORT) {
        if (processor->out_ports[reg].has_val()) {
            out_ports[reg * 2 + 1].set_text(std::to_string(processor->out_ports[reg].get_val()));
        } else {
            out_ports[reg * 2 + 1].set_text("");
        }
    } else if (change == ProcessorChange::IN_PORT) {
        if (processor->in_ports[reg].has_val()) {
            in_ports[reg * 2 + 1].set_text(std::to_string(processor->in_ports[reg].get_val()));
        } else {
            in_ports[reg * 2 + 1].set_text("");
        }
    }
}

