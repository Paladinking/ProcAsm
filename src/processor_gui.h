#ifndef PROC_ASM_PROCESSOR_GUI
#define PROC_ASM_PROCESSOR_GUI
#include "processor.h"
#include "problem.h"
#include "engine/ui.h"

class ProcessorGui {
public:
    ProcessorGui();

    ProcessorGui(Processor* ptr, ByteProblem* problem, int x, int y, WindowState* window_state);
    ProcessorGui(ProcessorGui&& other) = delete;
    ProcessorGui(const ProcessorGui& other) = delete;
    ProcessorGui& operator=(ProcessorGui&& other) = delete;
    ProcessorGui& operator=(const ProcessorGui& other) = delete;

    void render() const;

    void set_dpi(double dpi_scale);

    void mouse_change(bool press);

private:

    Processor* processor {nullptr};
    ByteProblem* problem {nullptr};

    WindowState* window_state {nullptr};

    int x {};
    int y {};

    std::vector<TextBox> problem_inputs {};
    std::vector<TextBox> problem_outputs {};
    std::vector<Dropdown> dropdowns {};

    // Contains indicies into processor->in_ports for each problem input
    std::vector<std::vector<std::size_t>> inputport_map;

    // Contains indicies into processor->out_ports for each problem output
    std::vector<std::vector<std::size_t>> oututport_map;

    std::vector<bool> register_state {};
    std::vector<TextBox> registers {};

    std::vector<TextBox> in_ports {};
    std::vector<TextBox> out_ports {};

    TextBox flags {};
    TextBox ticks {};

    Button step {};
    Button run {};
};

#endif
