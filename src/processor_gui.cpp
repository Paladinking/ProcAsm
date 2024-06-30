#include "processor_gui.h"
#include "config.h"
#include "engine/log.h"

typedef void(*Callback)(ProcessorGui*);
typedef void(*Callback_u)(uint64_t, ProcessorGui*);
typedef void(*Callback_i)(int64_t, ProcessorGui*);

ProcessorGui::ProcessorGui() {}
ProcessorGui::ProcessorGui(Processor* processor, ByteProblem* problem, int x, int y, WindowState* window_state) : processor{processor},
    problem{problem}, x{x}, y{y}, window_state{window_state} {

    processor->register_events(&window_state->events);
    problem->register_events(&window_state->events);
    comps.set_window_state(window_state);

    Callback run_pressed = [](ProcessorGui* gui) {
        if (gui->processor->is_valid()) {
            if (gui->processor->is_running()) {
                gui->run->set_text("Run");
                gui->processor->running = false;
            } else {
                gui->run->set_text("Stop");
                gui->processor->running = true;
            }
        }
    };

    Callback step_pressed = [](ProcessorGui* gui) {
        if (gui->processor->is_valid()) {
            gui->problem->clock_tick();
            gui->processor->clock_tick();
        }
    };

    run = comps.add(Button(BOX_SIZE + 10, BOX_SIZE - 90, 80, 80, "Run", *window_state), run_pressed, this);
    comps.add(Button(BOX_SIZE + 10, BOX_SIZE - 180, 80, 80, "Step", *window_state), step_pressed, this);

    Callback_u run_change = [](uint64_t running, ProcessorGui* gui) {
        if (running) {
            gui->run->set_text("Stop");
        } else {
            gui->run->set_text("Run");
        }
    };

    Callback_u register_change = [](uint64_t reg, ProcessorGui* gui) {
        std::string name = "R" + std::to_string(reg);
        auto val = gui->processor->gen_registers[reg];
        gui->registers[reg]->set_text(name + ": " + std::to_string(val));
    };

    Callback_u outport_change = [](uint64_t port, ProcessorGui* gui) {
        gui->out_ports[port * 2 + 1]->set_text("");
    };

    Callback_u inport_change = [](uint64_t port, ProcessorGui* gui) {
        if (gui->processor->in_ports[port].has_data()) {
            gui->in_ports[port * 2 + 1]->set_text(std::to_string(gui->processor->in_ports[port].get_data()));
        } else {
            gui->in_ports[port * 2 + 1]->set_text("");
        }
    };

    Callback_u ticks_change = [](uint64_t ticks, ProcessorGui* gui) {
        gui->ticks->set_text("Ticks: " + std::to_string(ticks));
    };

    window_state->events.register_callback(EventId::RUNNING_CHANGED, run_change, this);
    window_state->events.register_callback(EventId::REGISTER_CHANGED, register_change, this);
    window_state->events.register_callback(EventId::OUT_PORT_CHANGED, outport_change, this);
    window_state->events.register_callback(EventId::IN_PORT_CHANGED, inport_change, this);
    window_state->events.register_callback(EventId::TICKS_CHANGED, ticks_change, this);

    for (uint32_t i = 0; i < processor->gen_registers.size(); ++i) {
        auto text = "R" + std::to_string(i) + ": 0";
        auto reg = comps.add(TextBox(5 + BOX_SIZE - 120, 5 + BOX_LINE_HEIGHT * i, 8, BOX_LINE_HEIGHT, text, *window_state));
        reg->set_align(Alignment::LEFT);
        registers.push_back(reg);
    }

    ticks = comps.add(TextBox(5 + BOX_SIZE - 120, BOX_SIZE - 30, 8, BOX_LINE_HEIGHT, "Ticks: 0", *window_state));
    ticks->set_align(Alignment::LEFT);
    flags = comps.add(TextBox(5 + BOX_SIZE - 120, BOX_SIZE - 30 - BOX_LINE_HEIGHT, 8, BOX_LINE_HEIGHT, "Z: 0", *window_state));
    flags->set_align(Alignment::LEFT);

    for (uint32_t i = 0; i < processor->in_ports.size(); ++i) {
        std::string s = processor->in_ports[i].has_data() ? std::to_string(processor->in_ports[i].get_data()) : "";
        std::string name = "In " + processor->in_ports[i].name;
        auto box = comps.add(TextBox(30 + 110 * i, -50, 80, BOX_LINE_HEIGHT, name, *window_state));
        in_ports.push_back(box);
        box = comps.add(TextBox(30 + 100 * i, -50 + BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, s, *window_state));
        in_ports.push_back(box);
        comps.add(Border(30 + 110 * i, -60, 80, 60, 2));
    }
    for (uint32_t i = 0; i < processor->out_ports.size(); ++i) {
        std::string name = "Out " + processor->out_ports[i].name;
        auto box = comps.add(TextBox(30 + 110 * i, BOX_SIZE + 10, 80, BOX_LINE_HEIGHT, name, *window_state));
        out_ports.push_back(box);
        box = comps.add(TextBox(30 + 110 * i, BOX_SIZE + 10 + BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, "", *window_state));
        out_ports.push_back(box);
        comps.add(Border(30 + 110 * i, BOX_SIZE, 80, 60, 2));
    }


    inputport_map.clear();
    for (int i = 0; i < problem->input_ports.size(); ++i) {
        std::string name = std::to_string(i);
        auto box = comps.add(TextBox(10 + 110 * i, -280 + 80 / 2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state));
        problem_inputs.push_back(box);
        box = comps.add(TextBox(10 + 110 * i, -280 + 80 / 2, 80, BOX_LINE_HEIGHT, "None", *window_state));
        problem_inputs.push_back(box);

        inputport_map.emplace_back();

        std::vector<std::string> port_names {};
        for (int j = 0; j < processor->in_ports.size(); ++j) {
            ForwardingInputPort<uint8_t> &port = processor->in_ports[j];
            InBytePort<uint8_t>* p = port.check_port(dynamic_cast<InPort*>(problem->input_ports[i]));
            if (p != nullptr) {
                port_names.push_back(port.get_name());
                inputport_map.back().push_back(j);
            }
        }

        dropdowns.push_back(comps.add(Dropdown(110 * i, -280 + 90, 100, 40, "Select - ", port_names, *window_state)));

        comps.add(Border(10 + 110 * i, -280, 80, 80, 2));
    }

    for (int i = 0; i < problem->output_ports.size(); ++i) {
        std::string name = std::to_string(i);
        auto box = comps.add(TextBox(10 + 100 * i, BOX_SIZE + 200 + 80 / 2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state));
        problem_outputs.push_back(box);
        box = comps.add(TextBox(10 + 100 * i, BOX_SIZE + 200 + 80 /2, 80, BOX_LINE_HEIGHT, "None", *window_state));
        problem_outputs.push_back(box);

        comps.add(Border(10, BOX_SIZE + 200, 80, 80, 2));
    }

    comps.add(Border(BOX_SIZE - 120, 0, 120, 100, 2));
}

void ProcessorGui::on_input_dropdown(uint64_t ix, uint64_t val) {
    LOG_DEBUG("%llu, %llu Choosen", ix, val);
}

void ProcessorGui::render() const {
    comps.render(x, y);
    
    if (processor->valid) {
        int row = processor->instructions[processor->pc].line;
        SDL_Rect rect = {x + 4, y + BOX_TEXT_MARGIN + BOX_LINE_HEIGHT * row + 7, 6, 6};
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        SDL_RenderFillRect(gRenderer, &rect);
    }

    return;

    for (int i = 0; i < problem->output_ports.size(); ++i) {
        SDL_Rect rect = {x + 10, y + BOX_SIZE + 200, 80, 80};
        SDL_RenderDrawRect(gRenderer, &rect);
    }
}

void ProcessorGui::set_dpi(double dpi_scale) {
    comps.set_dpi(dpi_scale);
}

void ProcessorGui::mouse_change(bool press) {
    comps.handle_press(x, y, press);

    /*for (int i = 0; i < dropdowns.size(); ++i) {
        int last_choice = dropdowns[i]->get_choice();
        int choice = dropdowns[i]->handle_press(*window_state, x, y, press);
        if (choice >= 0 && choice != last_choice) {
            uint64_t ix = inputport_map[i][choice];
            for (int j = 0; j < dropdowns.size(); ++j) {
                if (i != j && dropdowns[j]->get_choice() != -1) {
                    if (inputport_map[j][dropdowns[j]->get_choice()] == ix) {
                        dropdowns[j]->clear_choice();
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
    }*/

    /*if (run.handle_press(x_pos, y_pos, press)) {
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
    }*/
}
