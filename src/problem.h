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

    void in_tick();

    void out_tick();

    void clock_tick();

    std::string format_input(std::size_t ix);

    std::string format_output(std::size_t ix);

    bool poll_input(std::size_t ix, std::string& s);

    bool poll_output(std::size_t ix, std::string& s);
private:
    friend class ProcessorGui;

    std::size_t ix = 0;

    int16_t last_output;

    // Ports that problem inputs are written to
    std::vector<SharedPort*> input_ports;
    // Ports that problem results are received from
    std::vector<SharedPort*> output_ports;

    std::vector<uint8_t> input_changes;
    std::vector<uint8_t> output_changes;
};

#endif
