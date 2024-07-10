#ifndef PROCESSOR_MENU_H
#define PROCESSOR_MENU_H
#include "engine/game.h"
#include "engine/menu.h"

class ProcessorMenu : public Menu {
public:
    ProcessorMenu(State* parent);

    void init(WindowState* window_state) override;

    void render() override;

private:
    State* parent;
};

#endif
