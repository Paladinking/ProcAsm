#ifndef PROCESSOR_MENU_H
#define PROCESSOR_MENU_H
#include "engine/game.h"
#include "engine/menu.h"
#include "processor.h"

class ProcessorMenu : public Menu {
public:
    ProcessorMenu(State* parent, std::vector<ProcessorTemplate>& templates);

    void init(WindowState* window_state) override;

    void render() override;

private:
    std::vector<ProcessorTemplate>& templates;

    State* parent;
};

#endif
