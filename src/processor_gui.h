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

    void update();

    void set_processor(Processor* processor);

    void set_dpi(double dpi_scale);

    void mouse_change(bool press);

    void menu_change(bool visible);
private:
    std::unique_ptr<EventScope> event_scope {};

    Processor* processor {nullptr};
    ByteProblem* problem {nullptr};

    WindowState* window_state {nullptr};

    int x {};
    int y {};

    std::vector<Component<TextBox>> problem_inputs {};
    std::vector<Component<TextBox>> problem_outputs {};
    std::vector<Component<Dropdown>> dropdowns {};

    std::vector<Component<Polygon>> input_wires{};
    std::vector<Component<Polygon>> output_wires{};

    std::vector<Component<TextBox>> registers {};

    Component<TextBox> flags {};
    Component<TextBox> ticks {};

    Component<Button> run {};

    Components comps {};
};

#endif
