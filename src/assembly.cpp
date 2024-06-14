#include "assembly.h"
#include "engine/log.h"

#include <sstream>

constexpr int WIDTH = 800, HEIGHT = 800;

constexpr int MAX_LINE_WIDTH = 46;

// TODO: Selection is weird when moving between lines

TextPosition& TextPosition::operator++() {
    ++col;
    ++ix;
    return *this;
}

TextPosition& TextPosition::operator--() {
    --col;
    --ix;
    return *this;
}

GameState::GameState() : State() {}

void add_line(std::vector<TextBox> &lines, WindowState* window_state, int ix) {
    int i = static_cast<int>(lines.size());
    lines.emplace_back(216, 280 + 20 * i, 200, 20, "", *window_state);
    lines.back().set_left_align(true);
    lines.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
    for (int j = ix; j < lines.size() - 1; ++j) {
        lines[j + 1].set_text(lines[j].get_text());
    }
    lines[ix].set_text("");
}

void GameState::init(WindowState *window_state) {
    SDL_ShowWindow(gWindow);
    SDL_RenderSetVSync(gRenderer, 1);
    SDL_RenderSetLogicalSize(gRenderer, WIDTH, HEIGHT);
    State::init(window_state);

    SDL_RenderGetLogicalSize(gRenderer, &window_state->screen_width,
                             &window_state->screen_height);
    SDL_GetRendererOutputSize(gRenderer, &window_state->window_width,
                              &window_state->window_height);

    LOG_INFO("Physical size: %d, %d\n", window_state->window_width, window_state->window_height);
    LOG_INFO("Logical size: %d, %d\n", window_state->screen_width, window_state->screen_width);

    add_line(lines, window_state, 0);

    cursor_text = TextBox(100, 100, 100, 100, "", *window_state);
    cursor_text.set_text_color(0xf0, 0xf0, 0xf0, 0xff);
    cursor_text.set_left_align(true);
}

void GameState::render() {
    SDL_SetRenderDrawColor(gRenderer, 0x20, 0x20, 0x20, 0xff);
    SDL_RenderClear(gRenderer);
    //SDL_RenderSetScale(gRenderer, 10.0, 10.0);

    SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
    SDL_Rect rect = {WIDTH / 2 - 200, HEIGHT / 2 - 200, 400, 400};
    SDL_RenderDrawRect(gRenderer, &rect);

    //SDL_RenderSetScale(gRenderer, 1.0, 1.0);
    if (selection_start.ix != selection_end.ix) {
        SDL_SetRenderDrawColor(gRenderer, 0x50, 0x50, 0x50, 0xff);
        int start = selection_start.col;
        for (int row = selection_start.row; row < selection_end.row; ++row) {
            int size = static_cast<int>(lines[row].get_text().size());
            SDL_Rect r = {216 + 8 * start, 280 + 20 * row, 8 * (size - start), 20};
            SDL_RenderFillRect(gRenderer, &r);
            start = 0;
        }
        int col = selection_end.col;
        SDL_Rect r = {216 + 8 * start, 280 + 20 * selection_end.row, 8 * (col - start), 20};
        SDL_RenderFillRect(gRenderer, &r);
    }

    SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);

    cursor_text.render(0, 0, *window_state);

    for (auto& box: lines) {
        box.render(0, 0, *window_state);
    }

    if (show_cursor) {
        SDL_RenderDrawLine(gRenderer, 216 + 8 * cursor_pos.col,
                           280 + 20 * cursor_pos.row,
                           216 + 8 * cursor_pos.col,
                           300 + 20 * cursor_pos.row);
    }



    SDL_RenderPresent(gRenderer);
}

bool shift_pressed() {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    return keys[SDL_SCANCODE_RSHIFT] || keys[SDL_SCANCODE_LSHIFT];
}

void GameState::delete_selection() {
    cursor_pos = selection_start;
    if (selection_start.row == selection_end.row) {
        std::string s = lines[selection_start.row].get_text();
        s.erase(selection_start.col, selection_end.col - selection_start.col);
        lines[selection_start.row].set_text(s);
    } else {
        std::string first = lines[selection_start.row].get_text();
        first.erase(selection_start.col);
        std::string last = lines[selection_end.row].get_text();
        last.erase(0, selection_end.col + 1);
        if (last.size() + first.size() > MAX_LINE_WIDTH) {
            lines[selection_start.row + 1].set_text(last);
            ++selection_start.row;
        } else {
            lines[selection_start.row].set_text(first + last);
        }
        for (int ix = 0; ix < lines.size() - (selection_end.row + 1); ++ix) {
            lines[selection_start.row + ix + 1].set_text(lines[selection_end.row + 1 + ix].get_text());
        }
        int rem_lines = static_cast<int>(lines.size()) - (selection_end.row - selection_start.row);
        lines.resize(rem_lines);
    }

    selection_base = cursor_pos;
    selection_end = cursor_pos;
}

void GameState::clear_selection() {
    selection_start = cursor_pos;
    selection_end = cursor_pos;
    selection_base = cursor_pos;
}

void GameState::tick(const Uint64 delta, StateStatus &res) {
    if (should_exit) {
        res.action = StateStatus::EXIT;
        return;
    }
    if (mouse_down) {
        TextPosition pos = find_pos(window_state->mouseX, window_state->mouseY);
        if (pos.ix < selection_base.ix) {
            selection_start = pos;
            selection_end = selection_base;
        } else {
            selection_start = selection_base;
            selection_end = pos;
        }
        cursor_pos = pos;
    }

    ticks_remaining -= static_cast<Sint64>(delta);
    if (box_selected && ticks_remaining < 0) {
        ticks_remaining = 500;
        show_cursor = !show_cursor;
    }

    std::stringstream s;
    s << "Ix: " << cursor_pos.ix << ", Row: " << cursor_pos.row << " Col: " << cursor_pos.col
      << " | Sel start(" << selection_start.ix << "," << selection_start.row << ","  << selection_start.col << ")"
      << " | Sel end(" << selection_end.ix << "," << selection_end.row << ","  << selection_end.col << ")"
      << " | Sel base(" << selection_base.ix << "," << selection_base.row << ","  << selection_base.col << ")";
    cursor_text.set_text(s.str());
}

void GameState::handle_up(SDL_Keycode key, Uint8 mouse) {
    if (!box_selected) {
        return;
    }

    if (mouse == SDL_BUTTON_LEFT && mouse_down) {
        mouse_down = false;
    }
}

TextPosition GameState::find_pos(int mouse_x, int mouse_y) const {
    int row = (mouse_y - 270) / 20;
    if (row < 0) {
        row = 0;
    } else if (row >= lines.size()) {
        row = static_cast<int>(lines.size() - 1);
    }
    int col = (mouse_x - 212) / 8;
    if (col < 0) {
        col = 0;
    } else if (col > lines[row].get_text().size()) {
        col = static_cast<int>(lines[row].get_text().size());
    }

    int ix = 0;
    for (int i = 0; i < row; ++i) {
        ix += static_cast<int>(lines[i].get_text().size());
    }
    return {row, col, ix + col};
}

void GameState::handle_down(const SDL_Keycode key, const Uint8 mouse) {
    if (key == SDLK_ESCAPE) {
        should_exit = true;
    } else if (key == SDLK_DOWN) {
        if (cursor_pos.row == lines.size() - 1) {
            return;
        }
        if (shift_pressed()) {
            // TODO
        } else {
            if (selection_start.ix != selection_end.ix) {
                clear_selection();
            }
            cursor_pos.ix += static_cast<int>(lines[cursor_pos.row].get_text().size()) - cursor_pos.col;
            ++cursor_pos.row;
            int size = static_cast<int>(lines[cursor_pos.row].get_text().size());
            if (cursor_pos.col > size) {
                cursor_pos.col = size;
            }
            cursor_pos.ix += cursor_pos.col;
        }
    } else if (key == SDLK_UP) {
        if (cursor_pos.row == 0) {
            return;
        }
        if (shift_pressed()) {
            // TODO
        } else {
            if (selection_start.ix != selection_end.ix) {
                clear_selection();
            }
            --cursor_pos.row;
            cursor_pos.ix -= cursor_pos.col;
            int size = static_cast<int>(lines[cursor_pos.row].get_text().size());
            if (cursor_pos.col > size) {
                cursor_pos.col = size;
            }
            cursor_pos.ix -= size - cursor_pos.col;
        }

    } else if (key == SDLK_LEFT && cursor_pos.col > 0) {
        if (shift_pressed()) {
            if (selection_start.ix == selection_end.ix) {
                clear_selection();
            }
            if (cursor_pos.ix <= selection_base.ix) {
                --selection_start;
            } else {
                --selection_end;
            }
            --cursor_pos;
            reset_cursor_animation();
        } else if (selection_start.ix != selection_end.ix) {
            clear_selection();
        } else {
            --cursor_pos;
            reset_cursor_animation();
        }
    } else if (key == SDLK_RIGHT && cursor_pos.col < lines[cursor_pos.row].get_text().size()) {
        if (shift_pressed()) {
            if (selection_start.ix == selection_end.ix) {
                clear_selection();
            }

            if (selection_start.ix < selection_base.ix) {
                ++selection_start;
            } else {
                ++selection_end;
            }
            ++cursor_pos;
            reset_cursor_animation();
        } else if (selection_start.ix != selection_end.ix) {
            clear_selection();
        } else {
            ++cursor_pos;
            reset_cursor_animation();
        }
    } else if (key == SDLK_BACKSPACE) {
        if (selection_start.ix != selection_end.ix) {
            delete_selection();
        } else {
            if (cursor_pos.col == 0) {
                if (lines.size() == 1) {
                    return;
                }
                for (int i = cursor_pos.row; i < lines.size() - 1; ++i) {
                    lines[i].set_text(lines[i + 1].get_text());
                }
                lines.pop_back();
                if (cursor_pos.row > 0) {
                    --cursor_pos.row;
                }
                cursor_pos.col = static_cast<int>(lines[cursor_pos.row].get_text().size());
            } else {
                std::string s = lines[cursor_pos.row].get_text();
                s.erase(cursor_pos.col - 1, 1);
                --cursor_pos;
                lines[cursor_pos.row].set_text(s);
            }
        }
        reset_cursor_animation();
    } else if (key == SDLK_RETURN) {
        if (selection_start.ix != selection_end.ix) {
            delete_selection();
        }
        std::string s = lines[cursor_pos.row].get_text();
        std::string rem = s.substr(cursor_pos.col);
        s.erase(cursor_pos.col);
        lines[cursor_pos.row].set_text(s);
        ++cursor_pos.row;
        add_line(lines, window_state, cursor_pos.row);
        cursor_pos.col = 0;
        lines[cursor_pos.row].set_text(rem);
    } else if (mouse == SDL_BUTTON_LEFT) {
        if (window_state->mouseX > WIDTH / 2 - 200 &&
            window_state->mouseX < WIDTH / 2 + 200 &&
            window_state->mouseY > HEIGHT / 2 - 200 &&
            window_state->mouseY < HEIGHT / 2 + 200) {

            LOG_DEBUG("Box pressed");
            TextPosition pos = find_pos(window_state->mouseX, window_state->mouseY);
            box_selected = true;
            SDL_StartTextInput();

            mouse_down = true;
            if (shift_pressed() && selection_start.ix != selection_end.ix) {
                if (pos.ix < selection_base.ix) {
                    selection_start = pos;
                    selection_end = selection_base;
                } else {
                    selection_start = selection_base;
                    selection_end = pos;
                }
                cursor_pos = pos;
            } else {
                selection_start = pos;
                selection_base = pos;
                cursor_pos = pos;
            }

            show_cursor = true;
            ticks_remaining = 800;
        } else {
            box_selected = false;
            show_cursor = false;
            SDL_StopTextInput();
        }
    }
}

void GameState::handle_textinput(const SDL_TextInputEvent &e) {
    if (!box_selected) {
        return;
    }
    if (e.text[0] == '\0' || e.text[1] != '\0') {
        LOG_DEBUG("Invalid input received");
        return;
    }
    char c = e.text[0];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == ',' || c == '.' || c == ';' || c == ':' || c == '!' || c == '?') {
        if (selection_start.ix != selection_end.ix) {
            delete_selection();
        }

        std::string s = lines[cursor_pos.row].get_text();
        if (s.size() == MAX_LINE_WIDTH) {
            return;
        }

        s.insert(s.begin() + cursor_pos.col, c);
        reset_cursor_animation();
        ++cursor_pos;
        lines[cursor_pos.row].set_text(s);
        selection_base = cursor_pos;
    }

}

void GameState::reset_cursor_animation() {
    ticks_remaining = 500;
    show_cursor = true;
}