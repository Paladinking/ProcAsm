#include "editbox.h"
#include "config.h"
#include "engine/log.h"
#include <sstream>

bool valid_char(unsigned char c) { return c >= 0x20 && c <= 0xef; }

Editbox::Editbox(int x, int y, const WindowState &window_state) : x(x), y(y), window_state(&window_state) {}


TextPosition Editbox::find_pos(int mouse_x, int mouse_y) const {
    int row = (mouse_y - (y + BOX_TEXT_MARGIN)) / BOX_LINE_HEIGHT;
    const std::vector<std::string>& text = lines.get_lines();
    if (row < 0) {
        row = 0;
    } else if (row >= text.size()) {
        row = static_cast<int>(text.size() - 1);
    }
    int col = (mouse_x - (x + BOX_TEXT_MARGIN)) / BOX_CHAR_WIDTH;
    if (col < 0) {
        col = 0;
    } else if (col > text[row].size()) {
        col = static_cast<int>(text[row].size());
    }
    return {row, col};
}


void Editbox::input_char(char c) {
    if (!box_selected) {
        return;
    }

    if (valid_char(c)) {
        lines.insert_str(std::string(1, c), EditType::WRITE);
    }
}

void Editbox::tick(Uint64 passed, bool mouse_down) {
    if (mouse_down && box_selected) {
        TextPosition pos = find_pos(window_state->mouseX, window_state->mouseY);
        lines.move_cursor(pos, true);
    }

    ticks_remaining -= static_cast<Sint64>(passed);
    if (box_selected && ticks_remaining < 0) {
        ticks_remaining = 500;
        show_cursor = !show_cursor;
    }
}

void Editbox::select() {
    TextPosition pos = find_pos(window_state->mouseX, window_state->mouseY);
    box_selected = true;
    bool shift_pressed = window_state->keyboard_state[SDL_SCANCODE_RSHIFT] ||
                         window_state->keyboard_state[SDL_SCANCODE_LSHIFT];

    lines.move_cursor(pos, shift_pressed);

    show_cursor = true;
    ticks_remaining = 800;
}

void Editbox::unselect() {
    box_selected = false;
    show_cursor = false;
    lines.clear_action();
}

void Editbox::reset_cursor_animation() {
    ticks_remaining = 500;
    show_cursor = true;
}

void validate_string(std::string& s) {
    while (true) {
        size_t pos = s.find("\r\n");
        if (pos == std::string::npos) {
            break;
        }
        s.erase(pos, 1);
    }

    for (int i = 0; i < s.size();) {
        if (s[i] == '\t') {
            s[i] = ' ';
            for (int j = 0; j < SPACES_PER_TAB - 1; ++j) {
                s.insert(s.begin() + i + j, ' ');
            }
            i += SPACES_PER_TAB;
        } else if (s[i] == '\r') {
            s[i] = '\n';
            ++i;
        } else if(s[i] != '\n' && !valid_char(s[i])) {
            s.erase(i, 1);
        } else {
            ++i;
        }
    }
}

void Editbox::handle_keypress(SDL_Keycode key) {
    bool shift_pressed = window_state->keyboard_state[SDL_SCANCODE_RSHIFT] ||
                         window_state->keyboard_state[SDL_SCANCODE_LSHIFT];
    bool ctrl_pressed = window_state->keyboard_state[SDL_SCANCODE_LCTRL] ||
                        window_state->keyboard_state[SDL_SCANCODE_RCTRL];

    TextPosition cursor = lines.get_cursor_pos();
    if (key == SDLK_HOME) {
        lines.move_cursor({cursor.row, 0}, shift_pressed);
    } else if (key == SDLK_END) {
        lines.move_cursor({cursor.row, lines.line_size(cursor.row)}, shift_pressed);
    } else if (key == SDLK_TAB || key == SDLK_KP_TAB) {
        lines.insert_str(std::string(SPACES_PER_TAB, ' '));
    } else if (key == 'a' && ctrl_pressed) {
        lines.set_selection({0, 0},
                            {static_cast<int>(lines.line_count() - 1),
                             lines.line_size(lines.line_count() - 1)},
                            true);
    } else if ((key == 'z' && ctrl_pressed && shift_pressed) || key == 'y' && ctrl_pressed) {
        if (lines.has_selection()) {
            lines.clear_selection();
        } else {
            lines.undo_action(true);
        }
    } else if (key == 'z' && ctrl_pressed) {
        if (lines.has_selection()) {
            lines.clear_selection();
        } else {
            lines.undo_action(false);
        }
    } else if (key == 'v' && ctrl_pressed) {
        char* clip = SDL_GetClipboardText();
        std::string s{clip};
        validate_string(s);
        SDL_free(clip);
        lines.insert_str(s);
    } else if ((key == 'c' || key == 'x') && ctrl_pressed) {
        if (!lines.has_selection()) {
            return;
        }
        std::string s = lines.extract_selection();
        LOG_DEBUG("Copying '%s' to clipboard", s.c_str());
        SDL_SetClipboardText(s.c_str());
        if (key == 'x') {
            lines.insert_str("");
        }
    } else if (key == SDLK_DOWN) {
        if (!shift_pressed && lines.has_selection()) {
            lines.clear_selection();
        }
        lines.move_cursor({cursor.row + 1, cursor.col}, shift_pressed);
    } else if (key == SDLK_UP) {
        if (!shift_pressed && lines.has_selection()) {
            lines.clear_selection();
        }
        lines.move_cursor({cursor.row - 1, cursor.col}, shift_pressed);
    } else if (key == SDLK_LEFT) {
        if (!shift_pressed && lines.has_selection()) {
            lines.clear_selection();
        } else {
            if (!lines.move_left(cursor, 1)) {
                return;
            }
            lines.move_cursor(cursor, shift_pressed);
        }
    } else if (key == SDLK_RIGHT) {
        if (!shift_pressed && lines.has_selection()) {
            lines.clear_selection();
        } else {
            if (!lines.move_right(cursor, 1)) {
                return;
            }
            lines.move_cursor(cursor, shift_pressed);
        }
    } else if (key == SDLK_DELETE) {
        if (!lines.has_selection()) {
            if (!lines.move_right(cursor, 1)) {
                return;
            }
            lines.move_cursor(cursor, true);
        }
        lines.insert_str("", EditType::DELETE);
    } else if (key == SDLK_BACKSPACE) {
        if (!lines.has_selection()) {
            if (!lines.move_left(cursor, 1)) {
                return;
            }
            lines.move_cursor(cursor, true);
        }
        lines.insert_str("", EditType::BACKSPACE);
    } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        lines.insert_str("\n");
    }
}

void Editbox::render() {
    SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
    SDL_Rect rect = {x, y, BOX_SIZE, BOX_SIZE};
    SDL_RenderDrawRect(gRenderer, &rect);

    if (lines.has_selection()) {
        SDL_SetRenderDrawColor(gRenderer, 0x50, 0x50, 0x50, 0xff);
        int start = lines.get_selection_start().col;
        for (int row = lines.get_selection_start().row; row < lines.get_selection_end().row; ++row) {
            int size = lines.line_size(row);
            SDL_Rect r = {x + BOX_TEXT_MARGIN + BOX_CHAR_WIDTH * start,
                          y + BOX_LINE_HEIGHT * row + BOX_TEXT_MARGIN,
                          BOX_CHAR_WIDTH * (size + 1 - start),
                          BOX_LINE_HEIGHT};
            SDL_RenderFillRect(gRenderer, &r);
            start = 0;
        }
        int col = lines.get_selection_end().col;
        SDL_Rect r = {x + BOX_TEXT_MARGIN + BOX_CHAR_WIDTH * start,
                      y + BOX_LINE_HEIGHT * lines.get_selection_end().row + BOX_TEXT_MARGIN,
                      BOX_CHAR_WIDTH * (col - start),
                      BOX_LINE_HEIGHT};
        SDL_RenderFillRect(gRenderer, &r);
    }

    for (auto& box: boxes) {
        box.render(x + BOX_TEXT_MARGIN, y + BOX_TEXT_MARGIN, *window_state);
    }

    if (show_cursor) {
        TextPosition cursor_pos = lines.get_cursor_pos();
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        SDL_RenderDrawLine(gRenderer, x + BOX_TEXT_MARGIN + BOX_CHAR_WIDTH * cursor_pos.col,
                           y + BOX_LINE_HEIGHT  * cursor_pos.row + BOX_TEXT_MARGIN,
                           x + BOX_TEXT_MARGIN + BOX_CHAR_WIDTH * cursor_pos.col,
                           y + BOX_LINE_HEIGHT * (cursor_pos.row + 1) + BOX_TEXT_MARGIN);
    }
}
void Editbox::set_dpi_scale(double dpi) {
    for (auto& line: boxes) {
        line.set_dpi_ratio(dpi);
    }
}
bool Editbox::is_pressed(int mouse_x, int mouse_y) const {
    return mouse_x > x && mouse_x < x + BOX_SIZE && mouse_y > y && mouse_y < y + BOX_SIZE;
}
void Editbox::change_callback(TextPosition start, TextPosition end,
                              int removed) {
    if (boxes.size() < lines.line_count()) {
        for (int i = boxes.size(); i < lines.line_count(); ++i) {
            boxes.emplace_back(0, 0 + BOX_LINE_HEIGHT * i, BOX_SIZE - 2 * BOX_TEXT_MARGIN, BOX_LINE_HEIGHT, "", *window_state);
            boxes.back().set_left_align(true);
            boxes.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
        }
    } else if (boxes.size() > lines.line_count()) {
        boxes.resize(lines.line_count());
    }
    for (int i = 0; i < lines.line_count(); ++i) {
        if (lines.get_lines()[i] != boxes[i].get_text()) {
            boxes[i].set_text(lines.get_lines()[i]);
        }
    }
    LOG_INFO("Change!");
    reset_cursor_animation();
    //lines.emplace_back(0, 0 + BOX_LINE_HEIGHT * i, BOX_SIZE - 2 * BOX_TEXT_MARGIN, BOX_LINE_HEIGHT, "", window_state);
}

void change_callback(TextPosition start, TextPosition end, int removed, void* aux) {
    static_cast<Editbox*>(aux)->change_callback(start, end, removed);

}
