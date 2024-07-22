#ifndef PROCESSOR_MENU_H
#define PROCESSOR_MENU_H
#include "engine/game.h"
#include "engine/menu.h"
#include "processor.h"
#include <limits>

class ProcessorMenu : public Menu {
public:
    ProcessorMenu(State *parent, std::vector<ProcessorTemplate> &templates,
                  std::size_t selected_ix);

    void init(WindowState *window_state) override;

    void render() override;

    void resume() override;

    void handle_down(SDL_Keycode key, Uint8 mouse) override;

    void handle_up(SDL_Keycode key, Uint8 mouse) override;
private:
    void layout_instructions();

    void remove_instruction(std::size_t ix);

    std::vector<ProcessorTemplate> &templates;

    std::vector<Component<Button>> proc_buttons {};

    Component<TextBox> name {};
    std::array<Component<TextBox>, 4> register_types{};
    Component<TextBox> flags {};
    Component<TextBox> features {};
    Component<Button> add_instruction {};
    Component<TextBox> instr_slots {};

    Components instr_comps {};

    std::size_t instr_to_remove = std::numeric_limits<std::size_t>::max();

    std::size_t selected_ix;

    State *parent;
};

#endif
