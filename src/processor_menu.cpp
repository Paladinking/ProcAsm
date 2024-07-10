#include "processor_menu.h"
#include "config.h"

ProcessorMenu::ProcessorMenu(State* parent) : parent{parent} {}

void ProcessorMenu::init(WindowState* window_state) {
    Menu::init(window_state);

    comps.add(Box(WIDTH / 2 - 300, HEIGHT / 2 - 400, 600, 800, 5));
} 

void ProcessorMenu::render() {
    parent->render();
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0x5f);
    SDL_Rect r = {0, 0, WIDTH, HEIGHT};
    SDL_RenderFillRect(gRenderer, &r);
    Menu::render();
}
