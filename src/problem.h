#ifndef PROC_ASM_PROBLEM_H
#define PROC_ASM_PROBLEM_H
#include <vector>
#include "ports.h"

#ifndef PROC_GUI_HEAD
#define PROC_GUI_HEAD
class ProcessorGui;
#endif

class ByteProblem {
public:
    ByteProblem();

    void reset();

    ByteInputSlot<uint8_t>* get_input_port(std::size_t ix);

    ByteOutputSlot<uint8_t>* get_output_port(std::size_t ix);

    void clock_tick_input();

    void clock_tick_output();

    bool poll_input(std::size_t ix, std::string& s);

    bool poll_output(std::size_t ix, std::string& s);
private:
    friend class ProcessorGui;

    std::size_t ix = 0;

    int16_t last_output;

    std::vector<ByteInputSlot<uint8_t>> input_ports;
    std::vector<ByteOutputSlot<uint8_t>> output_ports;

    std::vector<uint8_t> input_changes;
    std::vector<uint8_t> output_changes;
};

#endif
