#pragma once
#include "editbox.h"
#include "problem.h"
#include "processor.h"
#include "processor_gui.h"
#include <engine/game.h>
#include <engine/ui.h>

class GameState : public State {
public:
    GameState();

    void init(WindowState* state) override;

    void render() override;

    void tick(Uint64 delta, StateStatus &res) override;

    void handle_down(SDL_Keycode key, Uint8 mouse) override;

    void handle_up(SDL_Keycode key, Uint8 mouse) override;

    void handle_textinput(const SDL_TextInputEvent &e) override;

    void handle_size_change() override;

    void handle_focus_change(bool focus) override;
private:
    void set_font_size();

    bool should_exit = false;

    bool mouse_down = false;

    Editbox box;

    Processor processor {4};
    ProcessorGui processor_gui{};
    ByteProblem problem {};

    Uint64 ticks_passed = 0;

    double dpi_scale = 0.0;
};
