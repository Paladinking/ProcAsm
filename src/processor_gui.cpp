#include "processor_gui.h"
#include "config.h"
#include "engine/log.h"

ProcessorGui::ProcessorGui() {}
ProcessorGui::ProcessorGui(Processor* processor, ByteProblem* problem, int x, int y, WindowState* window_state) : processor{processor},
    problem{problem}, x{x}, y{y}, window_state{window_state} {

    processor->register_events(&window_state->events);
    problem->register_events(&window_state->events);

    auto running_change = [](EventInfo info, void* aux) {
        if (info.u) {
            reinterpret_cast<ProcessorGui*>(aux)->run.set_text("Stop");
        } else {
            reinterpret_cast<ProcessorGui*>(aux)->run.set_text("Run");
        }
    };

    auto register_change = [](EventInfo info, void* aux) {
        ProcessorGui* gui = reinterpret_cast<ProcessorGui*>(aux);
        std::string name = "R" + std::to_string(info.u);
        auto val = gui->processor->gen_registers[info.u];
        gui->registers[info.u].set_text(name + ": " + std::to_string(val));
    };

    auto outport_change = [](EventInfo info, void* aux) {
        reinterpret_cast<ProcessorGui*>(aux)->out_ports[info.u * 2 + 1].set_text("");
    };

    auto inport_change = [](EventInfo info, void* aux) {
        auto* gui = reinterpret_cast<ProcessorGui*>(aux);
        LOG_INFO("INPORT_CHANGE: %llu", info.u);
        if (gui->processor->in_ports[info.u].has_data()) {
            gui->in_ports[info.u * 2 + 1].set_text(std::to_string(gui->processor->in_ports[info.u].get_data()));
        } else {
            gui->in_ports[info.u * 2 + 1].set_text("");
        }
    };

    auto ticks_change = [](EventInfo info, void* aux) {
        reinterpret_cast<ProcessorGui*>(aux)->ticks.set_text("Ticks: " + std::to_string(info.u));
    };

    window_state->events.register_callback(EventId::RUNNING_CHANGED, running_change, this);
    window_state->events.register_callback(EventId::REGISTER_CHANGED, register_change, this);
    window_state->events.register_callback(EventId::OUT_PORT_CHANGED, outport_change, this);
    window_state->events.register_callback(EventId::IN_PORT_CHANGED, inport_change, this);
    window_state->events.register_callback(EventId::TICKS_CHANGED, ticks_change, this);

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


    inputport_map.clear();
    for (int i = 0; i < problem->input_ports.size(); ++i) {
        std::string name = std::to_string(i);
        problem_inputs.emplace_back(10 + 90 * i, -280 + 80 / 2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state);
        problem_inputs.back().set_text_color(TEXT_COLOR);
        problem_inputs.emplace_back(10 + 90 * i, -280 + 80 / 2, 80, BOX_LINE_HEIGHT, "None", *window_state);
        problem_inputs.back().set_text_color(TEXT_COLOR);

        inputport_map.emplace_back();

        std::vector<std::string> port_names {};
        for (int j = 0; j < processor->in_ports.size(); ++j) {
            auto &port = processor->in_ports[j];
            auto p = port.check_port(dynamic_cast<InPort*>(problem->input_ports[i]));
            if (p != nullptr) {
                port_names.push_back(port.get_name());
                inputport_map.back().push_back(j);
            }
        }

        dropdowns.push_back(Dropdown(90 * i, -280 + 90, 100, 40, "Select - ", port_names, *window_state));
        dropdowns.back().set_text_color(TEXT_COLOR);
    }

    for (int i = 0; i < problem->output_ports.size(); ++i) {
        std::string name = std::to_string(i);
        problem_outputs.emplace_back(10 + 90 * i, BOX_SIZE + 200 + 80 /2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state);
        problem_outputs.back().set_text_color(TEXT_COLOR);
        problem_outputs.emplace_back(10 + 90 * i, BOX_SIZE + 200 + 80 /2, 80, BOX_LINE_HEIGHT, "None", *window_state);
        problem_outputs.back().set_text_color(TEXT_COLOR);
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

    for (int i = 0; i < problem->output_ports.size(); ++i) {
        SDL_Rect rect = {x + 10, y + BOX_SIZE + 200, 80, 80};
        SDL_RenderDrawRect(gRenderer, &rect);
    }
    for (auto& in: problem_inputs) {
        in.render(x, y, *window_state);
    }
    for (auto& out: problem_outputs) {
        out.render(x, y, *window_state);
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

    for (int i = 0; i < dropdowns.size(); ++i) {
        int last_choice = dropdowns[i].get_choice();
        int choice = dropdowns[i].handle_press(x_pos, y_pos, press);
        if (choice != -1 && choice != last_choice) {
            uint64_t ix = inputport_map[i][choice];
            for (int j = 0; j < dropdowns.size(); ++j) {
                if (i != j && dropdowns[j].get_choice() != -1) {
                    if (inputport_map[j][dropdowns[j].get_choice()] == ix) {
                        dropdowns[j].clear_choice();
                    }
                }
            }
            if (last_choice != -1) {
                uint64_t ix = inputport_map[i][last_choice];
                processor->in_ports[ix].set_port(nullptr);
                window_state->events.notify_event(EventId::IN_PORT_CHANGED, ix);
            }
            auto& port = processor->in_ports[ix];
            auto *var = port.check_port(dynamic_cast<InPort*>(problem->get_input_port(i)));
            port.set_port(var);
            window_state->events.notify_event(EventId::IN_PORT_CHANGED, ix);
            problem->reset();
            processor->invalidate();
            processor->reset();
        }
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
            problem->clock_tick();
            processor->clock_tick();
        }
    }
}
