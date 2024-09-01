#ifndef PROCESSOR_MENU_H
#define PROCESSOR_MENU_H
#include "engine/game.h"
#include "processor.h"
#include "overlay_menu.h"
#include <limits>

class ProcessorMenu : public OverlayMenu {
public:
    ProcessorMenu(State *parent, std::vector<ProcessorTemplate> &templates,
                  std::size_t selected_ix);

    void init(WindowState *window_state) override;

    void render() override;

    void resume() override;

    void handle_down(SDL_Keycode key, Uint8 mouse) override;

    void handle_up(SDL_Keycode key, Uint8 mouse) override;
private:
    void change_processor(std::size_t ix);

    void layout_instructions();

    void remove_instruction(std::size_t ix);

    std::vector<ProcessorTemplate> &templates;

    std::vector<Component<Button>> proc_buttons {};

    Component<TextBox> name {};
    std::array<Component<TextBox>, 6> register_types{};
    Component<TextBox> flags {};
    Component<TextBox> features {};
    Component<Button> add_instruction {};
    Component<TextBox> instr_slots {};
    Component<TextBox> ports {};

    Components instr_comps {};

    std::size_t selected_ix;
};

#endif
