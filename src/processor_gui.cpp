#include "processor_gui.h"
#include "config.h"

ProcessorGui::ProcessorGui() {}
ProcessorGui::ProcessorGui(Processor* processor, ByteProblem* problem, int x, int y, WindowState* window_state) : processor{processor},
    problem{problem}, x{x}, y{y}, window_state{window_state} {
        std::function<void(ProcessorChange, uint32_t)> f {std::bind(&ProcessorGui::change_callback, this,
                                     std::placeholders::_1, std::placeholders::_2)};
    processor->register_change_callback(f);
    for (uint32_t i = 0; i < processor->gen_registers.size(); ++i) {
        registers.emplace_back(5 + BOX_SIZE - 120, 5 + BOX_LINE_HEIGHT * i, 8, BOX_LINE_HEIGHT, "R" + std::to_string(i) + ": 0", *window_state);
        registers.back().set_align(Alignment::LEFT);
        registers.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
    }

    ticks = TextBox(5 + BOX_SIZE - 120, BOX_SIZE - 30, 8, BOX_LINE_HEIGHT, "Ticks: 0", *window_state);
    ticks.set_align(Alignment::LEFT);
    ticks.set_text_color(0xf0, 0xf0, 0xf0, 0xff);
    flags = TextBox(5 + BOX_SIZE - 120, BOX_SIZE - 30 - BOX_LINE_HEIGHT, 8, BOX_LINE_HEIGHT, "Z: 0", *window_state);
    flags.set_align(Alignment::LEFT);
    flags.set_text_color(0xf0, 0xf0, 0xf0, 0xff);

    for (uint32_t i = 0; i < processor->in_ports.size(); ++i) {
        std::string s = processor->in_ports[i].has_data() ? std::to_string(processor->in_ports[i].get_data()) : "";
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

    step = Button(BOX_SIZE + 10, BOX_SIZE - 180, 80, 80, "Step", *window_state);
    step.set_text_color(0xf0, 0xf0, 0xf0, 0xff);
    run = Button(BOX_SIZE + 10, BOX_SIZE - 90, 80, 80, "Run", *window_state);
    run.set_text_color(0xf0, 0xf0, 0xf0, 0xff);


    for (int i = 0; i < problem->input_ports.size(); ++i) {
        std::string name = std::to_string(i);
        problem_inputs.emplace_back(10 + 90 * i, -280 + 80 / 2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state);
        problem_inputs.back().set_text_color(TEXT_COLOR);
        problem_inputs.emplace_back(10 + 90 * i, -280 + 80 / 2, 80, BOX_LINE_HEIGHT, "None", *window_state);
        problem_inputs.back().set_text_color(TEXT_COLOR);

        dropdowns.push_back(Dropdown(90 * i, -280 + 90, 100, 40, "Select - ", {"N / A"}, *window_state));
        dropdowns.back().set_text_color(TEXT_COLOR);
    }
}

void ProcessorGui::render() const {
    SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
    SDL_Rect rect {x + BOX_SIZE - 120, y, 120, 100};
    SDL_RenderDrawRect(gRenderer, &rect);
    for (const auto& b : registers) {
        b.render(x, y, *window_state);
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
    flags.render(x, y, *window_state);
    ticks.render(x, y, *window_state);
    
    if (processor->valid) {
        int row = processor->instructions[processor->pc].line;
        rect = {x + 4, y + BOX_TEXT_MARGIN + BOX_LINE_HEIGHT * row + 7, 6, 6};
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        SDL_RenderFillRect(gRenderer, &rect);
    }

    auto* me = const_cast<ProcessorGui*>(this);
    int x_pos = window_state->mouseX - x;
    int y_pos = window_state->mouseY - y;
    me->run.set_hover(run.is_pressed(x_pos, y_pos));
    me->step.set_hover(step.is_pressed(x_pos, y_pos));
    run.render(x, y, *window_state);
    step.render(x, y, *window_state);

    SDL_SetRenderDrawColor(gRenderer, TEXT_COLOR);
    for (int i = 0; i < problem->input_ports.size(); ++i) {
        SDL_Rect rect = {x + 10, y - 280, 80, 80};
        SDL_RenderDrawRect(gRenderer, &rect);
    }
    for (auto& in: problem_inputs) {
        in.render(x, y, *window_state);
    }
    for (auto& dd: dropdowns) {
        dd.render(x, y, *window_state);
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
    ticks.set_dpi_ratio(dpi_scale);
    flags.set_dpi_ratio(dpi_scale);
}

void ProcessorGui::mouse_change(bool press) {
    if (processor == nullptr) {
        return;
    }
    int x_pos = window_state->mouseX - x;
    int y_pos = window_state->mouseY - y;

    for (auto& dd: dropdowns) {
        dd.handle_press(x_pos, y_pos, press);
    }

    if (run.handle_press(x_pos, y_pos, press)) {
        if (processor->is_valid()) {
            if (processor->running) {
                processor->running = false;
                run.set_text("Run");
            } else {
                processor->running = true;
                run.set_text("Stop");
            }
        }
    }
    if (step.handle_press(x_pos, y_pos, press)) {
        if (processor->is_valid()) {
            processor->clock_tick();
        }
    }
}

void ProcessorGui::change_callback(ProcessorChange change, uint32_t reg) {
    if (change == ProcessorChange::REGISTER) {
        std::string name = "R" + std::to_string(reg);
        registers[reg].set_text(name + ": " + std::to_string(processor->gen_registers[reg]));
        flags.set_text("Z: " + std::to_string(processor->zero_flag));
    } else if (change == ProcessorChange::OUT_PORT) {
        out_ports[reg * 2 + 1].set_text("");
    } else if (change == ProcessorChange::IN_PORT) {
        if (processor->in_ports[reg].has_data()) {
            in_ports[reg * 2 + 1].set_text(std::to_string(processor->in_ports[reg].get_data()));
        } else {
            in_ports[reg * 2 + 1].set_text("");
        }
    } else if (change == ProcessorChange::RUNNING) {
        if (reg) {
            run.set_text("Stop");
        } else {
            run.set_text("Run");
        }
    } else {
        ticks.set_text("Ticks: " + std::to_string(processor->ticks));
    }
}
