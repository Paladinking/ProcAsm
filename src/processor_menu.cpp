#include "processor_menu.h"
#include "config.h"

ProcessorMenu::ProcessorMenu(State *parent,
                             std::vector<ProcessorTemplate> &templates)
    : parent{parent}, templates{templates} {}

void ProcessorMenu::init(WindowState *window_state) {
    Menu::init(window_state);
    comps.set_window_state(window_state);

    comps.add(Box(WIDTH / 2 - 316, HEIGHT / 2 - 400, 632, 800, 6));

    for (uint32_t i = 0; i < templates.size(); ++i) {
        const std::string& name = templates[i].name;
        auto btn = comps.add(Button(WIDTH / 2 - 316 + 70 + 166 * i, HEIGHT / 2 - 356, 160, 100, name, *window_state));
    }

    comps.add(Box(WIDTH / 2 - 316, HEIGHT / 2 - 216, 632, 6, 3));
}

void ProcessorMenu::render() {
    parent->render();
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0x5f);
    SDL_Rect r = {0, 0, WIDTH, HEIGHT};
    SDL_RenderFillRect(gRenderer, &r);
    Menu::render();
}
