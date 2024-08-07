#include "processor_gui.h"
#include "config.h"
#include "engine/log.h"

typedef void(*Callback)(ProcessorGui*);
typedef void(*Callback_u)(uint64_t, ProcessorGui*);
typedef void(*Callback_i)(int64_t, ProcessorGui*);

ProcessorGui::ProcessorGui() {}
ProcessorGui::ProcessorGui(Processor* processor, ByteProblem* problem, int x, int y, WindowState* window_state) : processor{processor},
    problem{problem}, x{x}, y{y}, window_state{window_state} {
        comps.set_window_state(window_state);
        set_processor(processor);
}

void ProcessorGui::set_processor(Processor* new_processor) {
    this->processor = new_processor;
    auto stamp = SDL_GetTicks64();

    problem_inputs.clear();
    problem_outputs.clear();
    dropdowns.clear();

    input_wires.clear();
    output_wires.clear();
    registers.clear();

    in_ports.clear();
    out_ports.clear(); 
    comps.clear();

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
            auto ticks_str = "Ticks: " + std::to_string(gui->processor->ticks);
            gui->ticks->set_text(ticks_str);
        }
    };

    run = comps.add(Button(BOX_SIZE + 10, BOX_SIZE - 90, 80, 80, "Run", *window_state), run_pressed, this);
    comps.add(Button(BOX_SIZE + 10, BOX_SIZE - 180, 80, 80, "Step", *window_state), step_pressed, this);

    Callback_u register_change = [](uint64_t reg, ProcessorGui* gui) {
        std::string name = gui->processor->registers.to_name_genreg(reg);
        auto val = gui->processor->registers.get_genreg(reg);
        gui->registers[reg]->set_text(name + ": " + std::to_string(val));
    };

    Callback_u ticks_change = [](uint64_t ticks, ProcessorGui* gui) {
        gui->ticks->set_text("Ticks: " + std::to_string(ticks));
    };

    LOG_DEBUG("Reg count: %d", processor->registers.count_genreg());
    for (uint64_t i = 0; i < processor->registers.count_genreg(); ++i) {
        auto text = processor->registers.to_name_genreg(i);
        LOG_INFO("Reg name: %s", text.c_str());
        auto reg = comps.add(TextBox(5 + BOX_SIZE - 120, 5 + BOX_LINE_HEIGHT * i, 0, BOX_LINE_HEIGHT, text, *window_state));
        reg->set_align(Alignment::LEFT);
        registers.push_back(reg);
    }

    ticks = comps.add(TextBox(5 + BOX_SIZE - 120, BOX_SIZE - 30, 100, BOX_LINE_HEIGHT, "Ticks: 0", *window_state));
    ticks->set_align(Alignment::LEFT);
    flags = comps.add(TextBox(5 + BOX_SIZE - 120, BOX_SIZE - 30 - BOX_LINE_HEIGHT, 100, BOX_LINE_HEIGHT, "Z: 0", *window_state));
    flags->set_align(Alignment::LEFT);

    for (uint32_t i = 0; i < processor->in_ports.size(); ++i) {
        std::string s = processor->in_ports[i]->format();
        std::string name = "In " + processor->in_ports[i]->get_name();
        auto box = comps.add(TextBox(30 + 110 * i, -50, 80, BOX_LINE_HEIGHT, name, *window_state));
        in_ports.push_back(box);
        box = comps.add(TextBox(30 + 110 * i, -50 + BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, s, *window_state));
        in_ports.push_back(box);
        comps.add(Box(30 + 110 * i, -60, 80, 60, 2));
    }
    for (uint32_t i = 0; i < processor->out_ports.size(); ++i) {
        std::string name = "Out " + processor->out_ports[i]->get_name();
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
            auto* p = gui->processor->in_ports[port].get();
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
            auto* p = gui->processor->in_ports[port_i].get();
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
            auto* p = gui->processor->out_ports[port].get();
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
            auto* p = gui->processor->out_ports[port_i].get();
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
        std::string s = problem->format_input(i);
        box = comps.add(TextBox(10 + 110 * i, -280 + 80 / 2, 80, BOX_LINE_HEIGHT, s, *window_state));
        problem_inputs.push_back(box);
        input_wires.emplace_back(comps.add(Polygon({})));

        inputport_map.emplace_back();

        std::vector<std::string> port_names {};
        for (int j = 0; j < processor->in_ports.size(); ++j) {
            auto* port = processor->in_ports[j].get();
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
        std::string s = problem->format_output(i);
        box = comps.add(TextBox(10 + 100 * i, BOX_SIZE + 200 + 80 /2, 80, BOX_LINE_HEIGHT, s, *window_state));
        problem_outputs.push_back(box);
        output_wires.emplace_back(comps.add(Polygon({})));

        outputport_map.emplace_back();

        std::vector<std::string> port_names {};
        for (int j = 0; j < processor->out_ports.size(); ++j) {
            auto* port = processor->out_ports[j].get();
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
}

void ProcessorGui::update() {
    std::string s;
    if (processor->is_running()) {
        if (run->get_text()[0] != 'S') {
            run->set_text("Stop");
        }
        ticks->set_text("Ticks: " + std::to_string(processor->ticks));
    } else {
        if (run->get_text()[0] != 'R') {
            run->set_text("Run");
        }
    }

    for (uint64_t i = 0; i < processor->in_ports.size(); ++i) { 
        auto& port = in_ports[2 * i + 1];
        if (processor->in_ports[i]->format_new(s)) {
            port->set_text(s);
        }
    }
    for (uint64_t i = 0; i < processor->out_ports.size(); ++i) { 
        auto& port = out_ports[2 * i + 1];
        if (processor->out_ports[i]->format_new(s)) {
            port->set_text(s);
        }
    }
    for (uint64_t i = 0; i < problem->input_ports.size(); ++i) { 
        auto& port = problem_inputs[2 * i + 1];
        if (problem->poll_input(i, s)) {
            port->set_text(s);
        }
    }
    for (uint64_t i = 0; i < problem->output_ports.size(); ++i) { 
        auto& port = problem_outputs[2 * i + 1];
        if (problem->poll_output(i, s)) {
            port->set_text(s);
        }
    }
    bool register_changed = false;
    for (uint64_t i = 0; i < processor->registers.count_genreg(); ++i) {
        if (processor->registers.poll_value_genreg(i, s)) {
            register_changed = true;
            registers[i]->set_text(s);
        }
    }
    if (register_changed) {
        bool z = processor->registers.flags & FLAG_ZERO_MASK;
        LOG_DEBUG("Zero: %d", z);
        flags->set_text(std::string("Z: ") + (z ? "1" : "0"));
    }
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

void ProcessorGui::menu_change(bool visible) {
    comps.enable_hover(!visible);
}

void ProcessorGui::set_dpi(double dpi_scale) {
    comps.set_dpi(dpi_scale);
}

void ProcessorGui::mouse_change(bool press) {
    comps.handle_press(x, y, press);
}
