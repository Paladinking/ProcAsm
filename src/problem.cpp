#include "problem.h"
#include "engine/engine.h"

ByteProblem::ByteProblem() {
    input_ports.push_back(nullptr);
    input_ports.push_back(nullptr);
    input_changes.push_back(0);
    input_changes.push_back(0);
    output_ports.push_back(nullptr);
    output_ports.push_back(nullptr);
    output_changes.push_back(0);
    output_changes.push_back(0);
}


void ByteProblem::reset() {
    ix = 0;
    last_output = -1;
    for (auto& b: input_changes) {
        b = 1;
    }
    for (auto& b: output_changes) {
        b = 1;
    }
}

std::string ByteProblem::format_input(std::size_t ix) {
    if (ix == 0) {
        return std::to_string(this->ix);
    } else {
        return "None";
    }
}

std::string ByteProblem::format_output(std::size_t ix) {
    if (last_output >= 0) {
        return std::to_string(last_output);
    } else {
        return "None";
    }
}

bool ByteProblem::poll_input(std::size_t ix, std::string& s) {
    if (input_changes[ix]) {
        s = format_input(ix);
        input_changes[ix] = 0;
        return true;
    }
    return false;
}

bool ByteProblem::poll_output(std::size_t ix, std::string& s) {
    if (output_changes[ix]) {
        s = format_output(ix);
        output_changes[ix] = 0;
        return true;
    }
    return false;
}

void ByteProblem::in_tick() {
    if (output_ports[0] != nullptr) {
        output_ports[0]->prepare_pop();
    }
}

void ByteProblem::out_tick() {
    if (input_ports[0] != nullptr) {
        if (input_ports[0]->push_byte(ix)) {
            ++ix;
            input_changes[0] = 1;
        }
    }
}

void ByteProblem::clock_tick() {
    if (output_ports[0] != nullptr) {
        uint8_t res;
        if (output_ports[0]->pop_byte(res)) {
            last_output = res;
            output_changes[0] = 1;
        }
    }
}
