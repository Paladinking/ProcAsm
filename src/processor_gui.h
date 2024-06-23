#ifndef PROC_ASM_PROCESSOR_GUI
#define PROC_ASM_PROCESSOR_GUI
#include "processor.h"
#include "problem.h"

class ProcessorGui {
public:
    ProcessorGui();

    ProcessorGui(Processor* ptr, ByteProblem* problem, int x, int y, WindowState* window_state);

    void render() const;

    void set_dpi(double dpi_scale);

    void mouse_change(bool press);

private:
    void change_callback(ProcessorChange, uint32_t reg);

    Processor* processor {nullptr};
    ByteProblem* problem {nullptr};

    WindowState* window_state {nullptr};

    int x {};
    int y {};

    std::vector<TextBox> problem_inputs {};
    std::vector<TextBox> problem_outputs {};
    std::vector<Dropdown> dropdowns {};

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
