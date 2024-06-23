#include "problem.h"

ByteProblem::ByteProblem() {
    // TODO: fix memory leak
    input_ports.push_back(new BlockingBytePort<uint8_t>{"1"});
    output_ports.push_back(new BlockingBytePort<uint8_t>{"2"});
}

void ByteProblem::reset() {
    ix = 0;
    for (auto &out : output_ports) {
        out->flush_input();
    }
}

OutBytePort<uint8_t> *ByteProblem::get_input_port(int ix) {
    return input_ports[ix];
}

InBytePort<uint8_t> *ByteProblem::get_output_port(int ix) {
    return output_ports[ix];
}

void ByteProblem::clock_tick() {
    if (input_ports[0]->has_space()) {
        input_ports[0]->push_data(ix);
        ++ix;
    }
    if (output_ports[0]->has_data()) {
        output_ports[0]->pop_data();
    }
}
