#include "processor_gui.h"
#include "config.h"
#include "engine/log.h"

typedef void(*Callback)(ProcessorGui*);
typedef void(*Callback_u)(uint64_t, ProcessorGui*);
typedef void(*Callback_i)(int64_t, ProcessorGui*);

ProcessorGui::ProcessorGui() {}
ProcessorGui::ProcessorGui(Processor* processor, ByteProblem* problem, int x, int y, WindowState* window_state) : processor{processor},
    problem{problem}, x{x}, y{y}, window_state{window_state} {

    processor->register_events();
    problem->register_events();
    comps.set_window_state(window_state);

    Callback_u menu_toggle = [](uint64_t show_menu, ProcessorGui* gui) {
        gui->comps.enable_hover(show_menu == 0);
    };

    gEvents.register_callback(EventId::MENU_CHANGE, menu_toggle, this);

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
        LOG_DEBUG("Step pressed");
        if (gui->processor->is_valid()) {
            gui->problem->clock_tick_output();
            gui->processor->clock_tick();
            gui->problem->clock_tick_input();
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
        std::string name = gui->processor->registers.to_name(reg);
        auto val = gui->processor->registers.get_genreg(reg);
        gui->registers[reg]->set_text(name + ": " + std::to_string(val));
    };

    Callback_u outport_change = [](uint64_t port, ProcessorGui* gui) {
        gui->out_ports[port * 2 + 1]->set_text(gui->processor->out_ports[port].format());
    };

    Callback_u inport_change = [](uint64_t port, ProcessorGui* gui) {
        gui->in_ports[port * 2 + 1]->set_text(gui->processor->in_ports[port].format());
    };

    Callback_u ticks_change = [](uint64_t ticks, ProcessorGui* gui) {
        gui->ticks->set_text("Ticks: " + std::to_string(ticks));
    };

    gEvents.register_callback(EventId::RUNNING_CHANGED, run_change, this);
    gEvents.register_callback(EventId::REGISTER_CHANGED, register_change, this);
    gEvents.register_callback(EventId::TICKS_CHANGED, ticks_change, this);

    Callback_u indata_change = [](uint64_t data, ProcessorGui* gui) {
        uint8_t val;
        if (gui->problem->poll_input(data, val)) {
            gui->problem_inputs[2 * data + 1]->set_text(std::to_string(val));
        } else {
            gui->problem_inputs[2 * data + 1]->set_text("None");
        }
    };

    Callback_u outdata_change = [](uint64_t data, ProcessorGui* gui) {
        uint8_t val;
        if (gui->problem->poll_output(data, val)) {
            gui->problem_outputs[2 * data + 1]->set_text(std::to_string(val));
        } else {
            gui->problem_outputs[2 * data + 1]->set_text("None");
        }
    };

    gEvents.register_callback(problem->input_event, indata_change, this);
    gEvents.register_callback(problem->output_event, outdata_change, this);

    for (uint64_t i = 0; i < processor->registers.gen_reg_count(); ++i) {
        auto text = processor->registers.to_name(i);
        auto reg = comps.add(TextBox(5 + BOX_SIZE - 120, 5 + BOX_LINE_HEIGHT * i, 8, BOX_LINE_HEIGHT, text, *window_state));
        reg->set_align(Alignment::LEFT);
        registers.push_back(reg);
    }

    ticks = comps.add(TextBox(5 + BOX_SIZE - 120, BOX_SIZE - 30, 8, BOX_LINE_HEIGHT, "Ticks: 0", *window_state));
    ticks->set_align(Alignment::LEFT);
    flags = comps.add(TextBox(5 + BOX_SIZE - 120, BOX_SIZE - 30 - BOX_LINE_HEIGHT, 8, BOX_LINE_HEIGHT, "Z: 0", *window_state));
    flags->set_align(Alignment::LEFT);

    for (uint32_t i = 0; i < processor->in_ports.size(); ++i) {
        int event = processor->in_ports[i].get_event();
        gEvents.register_callback(event, inport_change,
                                               static_cast<uint64_t>(i), this);

        std::string s = processor->in_ports[i].format();
        std::string name = "In " + processor->in_ports[i].get_name();
        auto box = comps.add(TextBox(30 + 110 * i, -50, 80, BOX_LINE_HEIGHT, name, *window_state));
        in_ports.push_back(box);
        box = comps.add(TextBox(30 + 110 * i, -50 + BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, s, *window_state));
        in_ports.push_back(box);
        comps.add(Box(30 + 110 * i, -60, 80, 60, 2));
    }
    for (uint32_t i = 0; i < processor->out_ports.size(); ++i) {
        int event = processor->out_ports[i].get_event();
        gEvents.register_callback(event, outport_change,
                                               static_cast<uint64_t>(i), this);

        std::string name = "Out " + processor->out_ports[i].get_name();
        auto box = comps.add(TextBox(30 + 110 * i, BOX_SIZE + 10, 80, BOX_LINE_HEIGHT, name, *window_state));
        out_ports.push_back(box);
        box = comps.add(TextBox(30 + 110 * i, BOX_SIZE + 10 + BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, "", *window_state));
        out_ports.push_back(box);
        comps.add(Box(30 + 110 * i, BOX_SIZE, 80, 60, 2));
    }


    inputport_map.clear();
    outputport_map.clear();
    
    void(*on_in_dropdown)(int64_t, int, ProcessorGui*) = [](int64_t select, int ix, ProcessorGui* gui) {
        for (auto& port: gui->problem->input_ports) {
            port.set_port(nullptr);
        }
        std::size_t port = -1;

        if (select != -1) {
            port = gui->inputport_map[ix][select];
            auto* p = &gui->processor->in_ports[port];
            auto* ptr = gui->problem->input_ports[ix].check_port(p);
            gui->problem->input_ports[ix].set_port(ptr);
            gui->input_wires[ix]->set_points({
                       {30 + 110.0f * ix, -140}, {70 + 110.0f * ix, -140}, {50 + 110.0f * port, -80},
                       {70 + 110.0f * ix , -140}, {50 + 110.0f * port, -80}, {90 + 110.0f * port, -80},
                       {30 + 110.0f * ix, -200}, {70 + 110.0f * ix, -200}, {30 + 110.0f * ix, -140},
                       {70 + 110.0f * ix, -200}, {70 + 110.0f * ix, -140}, {30 + 110.0f * ix, -140},
                       {50 + 110.0f * port, -80}, {90 + 110.0f * port, -80}, {50 + 110.0f * port, -60},
                       {90 + 110.0f * port, -80}, {90 + 110.0f * port, -60}, {50 + 110.0f * port, -60}
            });
        } else {
            gui->input_wires[ix]->set_points({});
        }

        Component<Dropdown>* dropdowns = gui->dropdowns.data();
        for (int i = 0; i < gui->problem->input_ports.size(); ++i) {
            int select_i = dropdowns[i]->get_choice();
            if (select_i == -1 || i == ix) {
                continue;
            }
            std::size_t port_i = gui->inputport_map[i][select_i];
            if (port_i == port && i != ix) {
                dropdowns[i]->clear_choice();
                gui->input_wires[i]->set_points({});
                continue;
            } 
            auto* p = &gui->processor->in_ports[port_i];
            auto* ptr = gui->problem->input_ports[i].check_port(p);
            gui->problem->input_ports[i].set_port(ptr);
        }
        gui->problem->reset();
        gui->processor->reset();
    };

    void(*on_out_dropdown)(int64_t, int, ProcessorGui*) = [](int64_t select, int ix, ProcessorGui* gui) {
        for (auto& port: gui->problem->output_ports) {
            port.set_port(nullptr);
        }
        std::size_t port = -1;

        if (select != -1) {
            port = gui->outputport_map[ix][select];
            auto* p = &gui->processor->out_ports[port];
            auto* ptr = gui->problem->output_ports[ix].check_port(p);
            gui->problem->output_ports[ix].set_port(ptr);
            LOG_DEBUG("%d, %d", ix, gui->output_wires.size());
            gui->output_wires[ix]->set_points({
                       {30 + 110.0f * ix, BOX_SIZE + 140}, {70 + 110.0f * ix, BOX_SIZE + 140}, {50 + 110.0f * port, BOX_SIZE + 80},
                       {70 + 110.0f * ix, BOX_SIZE + 140}, {50 + 110.0f * port, BOX_SIZE + 80}, {90 + 110.0f * port, BOX_SIZE + 80},
                       {30 + 110.0f * ix, BOX_SIZE + 140}, {70 + 110.0f * ix, BOX_SIZE + 140}, {30 + 110.0f * ix, BOX_SIZE + 200},
                       {70 + 110.0f * ix, BOX_SIZE + 140}, {70 + 110.0f * ix, BOX_SIZE + 200}, {30 + 110.0f * ix, BOX_SIZE + 200},
                       {50 + 110.0f * port, BOX_SIZE + 80}, {90 + 110.0f * port, BOX_SIZE + 80}, {50 + 110.0f * port, BOX_SIZE + 60},
                       {90 + 110.0f * port, BOX_SIZE + 80}, {90 + 110.0f * port, BOX_SIZE + 60}, {50 + 110.0f * port, BOX_SIZE + 60}
            });
        } else {
            gui->output_wires[ix]->set_points({});
        }

        Component<Dropdown>* dropdowns = gui->dropdowns.data() + gui->problem->input_ports.size();
        for (int i = 0; i < gui->problem->output_ports.size(); ++i) {
            int select_i = dropdowns[i]->get_choice();
            if (select_i == -1 || i == ix) {
                continue;
            }
            std::size_t port_i = gui->outputport_map[i][select_i];
            if (port_i == port && i != ix) {
                dropdowns[i]->clear_choice();
                continue;
            } 
            auto* p = &gui->processor->out_ports[port_i];
            auto* ptr = gui->problem->output_ports[i].check_port(p);
            gui->problem->output_ports[i].set_port(ptr);
        }
        gui->problem->reset();
        gui->processor->reset();
    };

    for (int i = 0; i < problem->input_ports.size(); ++i) {
        const std::string& name = problem->input_ports[i].name;
        auto box = comps.add(TextBox(10 + 110 * i, -280 + 80 / 2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state));
        problem_inputs.push_back(box);
        box = comps.add(TextBox(10 + 110 * i, -280 + 80 / 2, 80, BOX_LINE_HEIGHT, "None", *window_state));
        problem_inputs.push_back(box);
        input_wires.emplace_back(comps.add(Polygon({})));

        inputport_map.emplace_back();

        std::vector<std::string> port_names {};
        for (int j = 0; j < processor->in_ports.size(); ++j) {
            auto* port = &processor->in_ports[j];
            BytePort* p = problem->input_ports[i].check_port(port);
            if (p != nullptr) {
                port_names.push_back(port->get_name());
                inputport_map.back().push_back(j);
            }
        }

        dropdowns.push_back(comps.add(Dropdown(110 * i, -280 + 90, 100, 40, "Select - ", port_names, *window_state), on_in_dropdown, i, this));
        comps.add(Box(10 + 110 * i, -280, 80, 80, 2));
    }

    for (int i = 0; i < problem->output_ports.size(); ++i) {
        const std::string& name = problem->output_ports[i].name;
        auto box = comps.add(TextBox(10 + 100 * i, BOX_SIZE + 200 + 80 / 2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state));
        problem_outputs.push_back(box);
        box = comps.add(TextBox(10 + 100 * i, BOX_SIZE + 200 + 80 /2, 80, BOX_LINE_HEIGHT, "None", *window_state));
        problem_outputs.push_back(box);
        output_wires.emplace_back(comps.add(Polygon({})));

        outputport_map.emplace_back();

        std::vector<std::string> port_names {};
        for (int j = 0; j < processor->out_ports.size(); ++j) {
            auto* port = &processor->out_ports[j];
            BytePort* p = problem->output_ports[i].check_port(port);
            if (p != nullptr) {
                port_names.push_back(port->get_name());
                outputport_map.back().push_back(j);
            }
        }

        dropdowns.push_back(comps.add(Dropdown(110 * i, BOX_SIZE + 150, 100, 40, "Select - ", port_names, *window_state), on_out_dropdown, i, this));
        comps.add(Box(10 + 110 * i, BOX_SIZE + 200, 80, 80, 2));
    }

    comps.add(Box(BOX_SIZE - 120, 0, 120, 100, 2));

    processor_info = comps.add(Button(BOX_SIZE + 20, 20, 40, 40, "i", *window_state));

}

void ProcessorGui::render() const {
    comps.render(x, y);
    
    if (processor->valid) {
        int row = processor->instructions[processor->pc].line;
        SDL_Rect rect = {x + 4, y + BOX_TEXT_MARGIN + BOX_LINE_HEIGHT * row + 7, 6, 6};
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        SDL_RenderFillRect(gRenderer, &rect);
    }

}

void ProcessorGui::set_dpi(double dpi_scale) {
    comps.set_dpi(dpi_scale);
}

void ProcessorGui::mouse_change(bool press) {
    comps.handle_press(x, y, press);
}
