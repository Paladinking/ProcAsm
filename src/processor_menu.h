#ifndef PROCESSOR_MENU_H
#define PROCESSOR_MENU_H
#include "engine/game.h"
#include "engine/menu.h"
#include "processor.h"

class ProcessorMenu : public Menu {
public:
    ProcessorMenu(State *parent, std::vector<ProcessorTemplate> &templates,
                  std::size_t selected_ix);

    void init(WindowState *window_state) override;

    void render() override;

private:
    std::vector<ProcessorTemplate> &templates;

    std::vector<Component<Button>> proc_buttons {};

    Component<TextBox> name {};
    std::array<Component<TextBox>, 4> register_types{};
    Component<TextBox> flags {};

    std::size_t selected_ix;

    State *parent;
};

#endif
