#ifndef PROC_ASM_PROBLEM_H
#define PROC_ASM_PROBLEM_H
#include <vector>
#include "ports.h"
#include "engine/log.h"

class ByteProblemGui;

class ByteProblem {
public:
    ByteProblem() {
        // TODO: fix memory leak
        input_ports.push_back(new BlockingBytePort<uint8_t>{"1"});
        output_ports.push_back(new BlockingBytePort<uint8_t>{"2"});
    }

    void reset() {
        ix = 0;
        output_ports[0]->flush_input();
    }

    OutBytePort<uint8_t>* get_input_port(int ix) {
        return input_ports[ix];
    }

    InBytePort<uint8_t>* get_output_port(int ix) {
        return output_ports[ix];
    }

    void clock_tick() {
        if (input_ports[0]->has_space()) {
            LOG_DEBUG("Push");
            input_ports[0]->push_data(ix);
            ++ix;
        }
        if (output_ports[0]->has_data()) {
            LOG_DEBUG("Got data: %d", output_ports[0]->get_data());
            output_ports[0]->pop_data();
        }
    }

private:
    friend class ByteProblemGui;

    std::size_t ix = 0;

    std::vector<OutBytePort<uint8_t>*> input_ports;
    std::vector<InBytePort<uint8_t>*> output_ports;

};



class ByteProblemGui {

};

#endif
