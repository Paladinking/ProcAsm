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

    void register_events(Events* events);

    OutBytePort<uint8_t>* get_input_port(int ix);

    InBytePort<uint8_t>* get_output_port(int ix);

    void clock_tick();

    void on_processor_change();

private:
    friend class ProcessorGui;

    std::size_t ix = 0;

    std::vector<OutBytePort<uint8_t>*> input_ports;
    std::vector<InBytePort<uint8_t>*> output_ports;

    Events* events;
};

#endif
