#include "menu.h"

Menu::Menu() : State() {
    // Since get_press_input can throw exceptions...
    exit_input = std::unique_ptr<PressInput>(new KeyPressInput(SDLK_ESCAPE));
}

Menu::Menu(const std::string &exit_input) : State() {
    this->exit_input = get_press_input(exit_input, "Escape");
}

void Menu::handle_down(const SDL_Keycode key, const Uint8 mouse) {
    if (mouse == SDL_BUTTON_LEFT) {
        targeted_button = -1;
        for (int i = 0; i < buttons.size(); ++i) {
            if (buttons[i].is_pressed(window_state->mouseX,
                                      window_state->mouseY)) {
                targeted_button = i;
                break;
            }
        }
    }
    if (exit_input->is_targeted(key, mouse)) {
        menu_exit();
    }
}

void Menu::handle_up(const SDL_Keycode key, const Uint8 mouse) {
    if (mouse == SDL_BUTTON_LEFT) {
        if (targeted_button >= 0 &&
            buttons[targeted_button].is_pressed(window_state->mouseX,
                                                window_state->mouseY)) {
            button_press(targeted_button);
        }
    }
}

void Menu::render() {
    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(gRenderer);

    for (auto &b : buttons) {
        b.set_hover(b.is_pressed(window_state->mouseX, window_state->mouseY));
        b.render(0, 0, *window_state);
    }
    for (auto &t : text) {
        t.render(0, 0, *window_state);
    }
    SDL_RenderPresent(gRenderer);
}

void Menu::tick(const Uint64 delta, StateStatus &res) {
    res = next_res;
    next_res.action = StateStatus::NONE;
    next_res.new_state = nullptr;
}

void Menu::menu_exit() { next_res.action = StateStatus::POP; }
