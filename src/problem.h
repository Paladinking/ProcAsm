#ifndef PROC_ASM_PROBLEM_H
#define PROC_ASM_PROBLEM_H
#include <vector>
#include "ports.h"
#include "event_id.h"

#ifndef PROC_GUI_HEAD
#define PROC_GUI_HEAD
class ProcessorGui;
#endif

class ByteProblem {
public:
    ByteProblem();

    void reset();

    void register_events();

    ForwardingOutputPort<uint8_t>* get_input_port(std::size_t ix);

    ForwardingInputPort<uint8_t>* get_output_port(std::size_t ix);

    void clock_tick_input();

    void clock_tick_output();

    bool poll_input(std::size_t ix, uint8_t& val) const;

    bool poll_output(std::size_t ix, uint8_t& val) const;
private:
    friend class ProcessorGui;

    std::size_t ix = 0;

    int16_t last_output;

    std::vector<ForwardingOutputPort<uint8_t>> input_ports;
    std::vector<ForwardingInputPort<uint8_t>> output_ports;

    int input_event;
    int output_event;
};

#endif
