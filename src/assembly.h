#pragma once
#include <engine/game.h>
#include <engine/ui.h>


struct TextPosition {
    int row, col, ix;

    TextPosition& operator++();

    TextPosition& operator--();
};


class GameState : public State {
public:
    GameState();

    void init(WindowState* state) override;

    void render() override;

    void tick(Uint64 delta, StateStatus &res) override;

    void handle_down(SDL_Keycode key, Uint8 mouse) override;

    void handle_up(SDL_Keycode key, Uint8 mouse) override;

    void handle_textinput(const SDL_TextInputEvent &e) override;
private:
    void reset_cursor_animation();

    void delete_selection();

    void clear_selection();

    TextPosition find_pos(int mouse_x, int mouse_y) const;

    bool should_exit = false;

    bool box_selected = false;

    TextPosition selection_start {};
    TextPosition selection_end {};
    TextPosition selection_base {};

    bool mouse_down = false;

    bool show_cursor = false;
    Sint64 ticks_remaining = 0;

    TextPosition cursor_pos {};

    TextBox cursor_text;

    std::vector<TextBox> lines {};
};
