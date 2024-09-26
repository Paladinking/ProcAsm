#ifndef PROC_ASM_PROCESSOR_GUI
#define PROC_ASM_PROCESSOR_GUI
#include "processor.h"
#include "problem.h"
#include "editbox.h"
#include "engine/ui.h"

/*
 * Class containing Gui that exists for all processors.
 * This means the textbox and all ports.
 */
class ProcessorGui {
public:
    ProcessorGui();

    ProcessorGui(Processor* ptr, ByteProblem* problem, int x, int y, WindowState* window_state);
    ProcessorGui(ProcessorGui&& other) = delete;
    ProcessorGui(const ProcessorGui& other) = delete;
    ProcessorGui& operator=(ProcessorGui&& other) = delete;
    ProcessorGui& operator=(const ProcessorGui& other) = delete;

    void render() const;

    // Call when something on the processor behind the gui might have changed
    void update();

    // Call regularly for cursor animation etc.
    void tick(Uint64 passed);

    void set_processor(Processor* processor);

    void set_dpi(double dpi_scale);

    bool is_pressed(int x, int y) const;

    void set_selected(bool selected);

    void menu_change(bool visible);

    void set_edit_text(std::string& text);

    void input_char(char c);

    void handle_keypress(SDL_Keycode key);

    const std::vector<std::string>& get_edit_text() const;
private:
    std::unique_ptr<EventScope> event_scope {};

    Editbox box;

    Processor* processor {nullptr};
    ByteProblem* problem {nullptr};

    WindowState* window_state {nullptr};

    int x {};
    int y {};

    std::vector<Component<TextBox>> problem_inputs {};
    std::vector<Component<TextBox>> problem_outputs {};

    std::vector<Component<Polygon>> input_wires{};
    std::vector<Component<Polygon>> output_wires{};

    std::vector<Component<TextBox>> registers {};

    Component<TextBox> flags {};
    Component<TextBox> ticks {};

    Components comps {};
};

#endif
