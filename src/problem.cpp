#include "problem.h"
#include "engine/log.h"

ByteProblem::ByteProblem() {
    input_ports.emplace_back("Port 1");
    input_ports.emplace_back("Port 2");
    output_ports.emplace_back("Port 2");
}

void ByteProblem::register_events(Events* events) {
    this->events = events;
    input_event = events->register_event(EventType::UNIFIED_VEC, -1, input_ports.size());
    output_event = events->register_event(EventType::UNIFIED_VEC, -1, output_ports.size());
}

void ByteProblem::reset() {
    ix = 0;
    last_output = -1;
    events->notify_event(input_event, 0);
    events->notify_event(output_event, 0);
}

ForwardingOutputPort<uint8_t> *ByteProblem::get_input_port(std::size_t ix) {
    return &input_ports[ix];
}

ForwardingInputPort<uint8_t> *ByteProblem::get_output_port(std::size_t ix) {
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
        input_ports[0].push_data(ix);
        ++ix;
        events->notify_event(input_event, 0);
    }
}

void ByteProblem::clock_tick_output() {
    if (output_ports[0].has_data()) {
        last_output = output_ports[0].get_data();
        output_ports[0].pop_data();
        events->notify_event(output_event, 0);
    }
}
