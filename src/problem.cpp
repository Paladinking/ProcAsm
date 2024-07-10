#include "problem.h"
#include "engine/engine.h"

ByteProblem::ByteProblem() {
    input_ports.emplace_back("Port 1");
    input_ports.emplace_back("Port 2");
    output_ports.emplace_back("Port 2");
}

void ByteProblem::register_events() {
    input_event = gEvents.register_event(EventType::UNIFIED_VEC, input_ports.size());
    output_event = gEvents.register_event(EventType::UNIFIED_VEC, output_ports.size());
}

void ByteProblem::reset() {
    ix = 0;
    last_output = -1;
    gEvents.notify_event(input_event, 0);
    gEvents.notify_event(output_event, 0);
}

ByteInputSlot<uint8_t> *ByteProblem::get_input_port(std::size_t ix) {
    return &input_ports[ix];
}

ByteOutputSlot<uint8_t> *ByteProblem::get_output_port(std::size_t ix) {
    return &output_ports[ix];
}

bool ByteProblem::poll_input(std::size_t ix, uint8_t& val) const {
    val = this->ix;
    return true;
}

bool ByteProblem::poll_output(std::size_t ix, uint8_t& val) const {
    val = last_output;
    return last_output >= 0;
}

void ByteProblem::clock_tick_input() {
    if (input_ports[0].has_space()) {
        input_ports[0].push_byte(ix);
        ++ix;
        gEvents.notify_event(input_event, 0);
    }
}

void ByteProblem::clock_tick_output() {
    if (output_ports[0].has_data()) {
        last_output = output_ports[0].get_byte();
        output_ports[0].pop_data();
        gEvents.notify_event(output_event, 0);
    }
}
