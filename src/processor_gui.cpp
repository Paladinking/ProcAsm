#include "processor_gui.h"
#include "config.h"
#include "engine/log.h"

typedef void(*Callback)(ProcessorGui*);
typedef void(*Callback_u)(uint64_t, ProcessorGui*);
typedef void(*Callback_i)(int64_t, ProcessorGui*);

ProcessorGui::ProcessorGui() {}
ProcessorGui::ProcessorGui(Processor* processor, ByteProblem* problem, int x, int y, WindowState* window_state) : processor{processor},
    problem{problem}, x{x}, y{y}, window_state{window_state}, box{x, y, *window_state} {
        comps.set_window_state(window_state);
        set_processor(processor);
}

void ProcessorGui::set_processor(Processor* new_processor) {
    this->processor = new_processor;
    auto stamp = SDL_GetTicks64();

    problem_inputs.clear();
    problem_outputs.clear();

    input_wires.clear();
    output_wires.clear();
    registers.clear();

    comps.clear();

    Callback_u register_change = [](uint64_t reg, ProcessorGui* gui) {
        std::string name = gui->processor->registers.to_name_genreg(reg);
        auto val = gui->processor->registers.get_genreg(reg);
        gui->registers[reg]->set_text(name + ": " + std::to_string(val));
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

    for (int i = 0; i < problem->input_ports.size(); ++i) {
        const std::string name = "Input " + std::to_string(i);
        auto box = comps.add(TextBox(10 + 110 * i, -280 + 80 / 2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state));
        problem_inputs.push_back(box);
        std::string s = problem->format_input(i);
        box = comps.add(TextBox(10 + 110 * i, -280 + 80 / 2, 80, BOX_LINE_HEIGHT, s, *window_state));
        problem_inputs.push_back(box);
        comps.add(Box(10 + 110 * i, -280, 80, 80, 2));
    }

    for (int i = 0; i < problem->output_ports.size(); ++i) {
        const std::string name = "Output " + std::to_string(i);
        auto box = comps.add(TextBox(10 + 100 * i, BOX_SIZE + 200 + 80 / 2 - BOX_LINE_HEIGHT, 80, BOX_LINE_HEIGHT, name, *window_state));
        problem_outputs.push_back(box);
        std::string s = problem->format_output(i);
        box = comps.add(TextBox(10 + 100 * i, BOX_SIZE + 200 + 80 /2, 80, BOX_LINE_HEIGHT, s, *window_state));
        problem_outputs.push_back(box);
        comps.add(Box(10 + 110 * i, BOX_SIZE + 200, 80, 80, 2));
    }

    comps.add(Box(BOX_SIZE - 120, 0, 120, 100, 2));

    int count = processor->port_layout.up;
    int pw = (BOX_SIZE - 20) / count;
    int ps = pw / 2 - 20 + 10;
    int px = 0;
    for (int i = 0; i < count; ++i, ++px) {
        comps.add(Polygon({{ps + 10.0f + pw * i, -15.0f},
                          {ps + 40.0f + pw * i, 0.0f},
                          {ps + 30.0f + pw * i, -15.0f},
                          {static_cast<float>(ps) + pw * i, 0.0f},
                          {ps + 40.0f + pw * i, 0.0f},
                          {ps + 10.0f + pw * i, -15.0f}
                          }))->set_border_color(0x7f, 0x7f, 0x7f, 0xff);
        std::string name = processor->port_layout.name(px);
        comps.add(TextBox(ps + pw * i, -54, 40, 50, name, *window_state));
    }
    count = processor->port_layout.right;
    pw = (BOX_SIZE - 20) / count;
    ps = pw / 2 - 20 + 10;
    for (int i = 0; i < count; ++i, ++px) {
        comps.add(Polygon({{BOX_SIZE + 15.0f, ps + 10.0f + pw * i},
                           {BOX_SIZE, ps + 40.0f + pw * i},
                           {BOX_SIZE + 15.0f, ps + 30.0f + pw *i},
                           {BOX_SIZE, static_cast<float>(ps) + pw * i},
                           {BOX_SIZE, ps + 40.0f + pw * i},
                           {BOX_SIZE + 15.0f, ps + 10.0f + pw * i}
                          }))->set_border_color(0x7f, 0x7f, 0x7f, 0xff);
        std::string name = processor->port_layout.name(px);
        comps.add(TextBox(BOX_SIZE + 5, ps + pw * i, 50, 40, name, *window_state));
    }
    count = processor->port_layout.down;
    pw = (BOX_SIZE - 20) / count;
    ps = pw / 2 - 20 + 10;
    for (int i = 0; i < count; ++i, ++px) {
        comps.add(Polygon({{ps + 10.0f + pw * i, BOX_SIZE + 15.0f},
                           {ps + 40.0f + pw * i, BOX_SIZE},
                           {ps + 30.0f + pw *i, BOX_SIZE + 15.0f},
                           {static_cast<float>(ps) + pw * i, BOX_SIZE},
                           {ps + 40.0f + pw * i, BOX_SIZE},
                           {ps + 10.0f + pw * i, BOX_SIZE + 15.0f}
                          }))->set_border_color(0x7f, 0x7f, 0x7f, 0xff);
        std::string name = processor->port_layout.name(px);
        comps.add(TextBox(ps + pw * i - 4, BOX_SIZE + 10, 50, 40, name, *window_state));
    }
    count = processor->port_layout.left;
    pw = (BOX_SIZE - 20) / count;
    ps = pw / 2 - 20 + 10;
    for (int i = 0; i < count; ++i, ++px) {
        comps.add(Polygon({{-15.0f, ps + 10.0f + pw * i},
                           {0.0f, ps + 40.0f + pw * i},
                           {-15.0f, ps + 30.0f + pw *i},
                           {0.0f, static_cast<float>(ps) + pw * i},
                           {0.0f, ps + 40.0f + pw * i},
                           {-15.0f, ps + 10.0f + pw * i}
                          }))->set_border_color(0x7f, 0x7f, 0x7f, 0xff);
        std::string name = processor->port_layout.name(px);
        comps.add(TextBox(-54, ps + pw * i, 50, 40, name, *window_state));
    }

    std::vector<ErrorMsg> errors;
    const auto& text = box.get_text();
    processor->compile_program(text, errors);
    box.set_errors(errors);
}

void ProcessorGui::tick(Uint64 passed) {
    box.tick(passed);
}

void ProcessorGui::update() {
    std::string s;
    ticks->set_text("Ticks: " + std::to_string(processor->ticks));

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
    box.render();
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

void ProcessorGui::set_edit_text(std::string& text) {
    box.set_text(text);
    std::vector<ErrorMsg> errors;
    processor->compile_program(box.get_text(), errors);
    box.set_errors(errors);
}

const std::vector<std::string>& ProcessorGui::get_edit_text() const {
    return box.get_text();
}

void ProcessorGui::set_dpi(double dpi_scale) {
    comps.set_dpi(dpi_scale);
}

bool ProcessorGui::is_pressed(int x, int y) const {
    return box.is_pressed(x, y);
}

void ProcessorGui::set_selected(bool selected) {
    if (selected) {
        box.select();
    } else {
        box.unselect();
    }
    if (!processor->is_valid()) {
        std::vector<ErrorMsg> errors;
        const auto& text = box.get_text();
        processor->compile_program(text, errors);
        box.set_errors(errors);
    }
}

void ProcessorGui::input_char(char c) {
    box.input_char(c);
}

void ProcessorGui::handle_keypress(const SDL_Keycode key) {
    box.handle_keypress(key);
}
