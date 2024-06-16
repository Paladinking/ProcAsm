#include "editbox.h"
#include "config.h"
#include "engine/log.h"
#include <sstream>

bool TextPosition::operator<(const TextPosition &other) const {
    if (other.row == row) {
        return col < other.col;
    }
    return row < other.row;
}
bool TextPosition::operator>(const TextPosition &other) const {
    return (other < *this);
}
bool TextPosition::operator<=(const TextPosition &other) const {
    return !(other < *this);
}
bool TextPosition::operator>=(const TextPosition &other) const {
    return !(*this < other);
}
bool TextPosition::operator==(const TextPosition &other) const {
    return col == other.col && row == other.row;
}
bool TextPosition::operator!=(const TextPosition &other) const {
    return !(*this == other);
}

bool valid_char(unsigned char c) { return c >= 0x20 && c <= 0xef; }

void add_line(std::vector<TextBox> &lines, const WindowState &window_state, int ix) {
    int i = static_cast<int>(lines.size());
    lines.emplace_back(0, 0 + BOX_LINE_HEIGHT * i, BOX_SIZE - 2 * BOX_TEXT_MARGIN, BOX_LINE_HEIGHT, "", window_state);
    lines.back().set_left_align(true);
    lines.back().set_text_color(0xf0, 0xf0, 0xf0, 0xff);
    for (int j = static_cast<int>(lines.size()) - 2; j >= ix; --j) {
        lines[j + 1].set_text(lines[j].get_text());
    }
    lines[ix].set_text("");
}

std::string Editbox::extract_region(TextPosition start, TextPosition end) const {
    if (start == end) {
        return "";
    }
    if (start.row == end.row) {
        return lines[start.row].get_text().substr(start.col,
                                                  end.col - start.col);
    } else {
        std::stringstream res{};
        res << lines[start.row].get_text().substr(start.col);
        for (int row = start.row + 1; row < end.row; ++row) {
            res << '\n' << lines[row].get_text();
        }
        res << '\n' << lines[end.row].get_text().substr(0, end.col);
        return res.str();
    }
}

Editbox::Editbox(int x, int y, const WindowState &window_state) : x(x), y(y) {
    add_line(lines, window_state, 0);
}

void Editbox::delete_region(TextPosition start, TextPosition end) {
    if (start >= end) {
        return;
    }
    std::string first = lines[start.row].get_text();
    first.erase(start.col);
    std::string last = lines[end.row].get_text();
    last.erase(0, end.col);
    lines[start.row].set_text(first + last);
    for (int ix = 0; ix < lines.size() - (end.row + 1); ++ix) {
        lines[start.row + ix + 1].set_text(lines[end.row + 1 + ix].get_text());
    }
    int rem_lines = static_cast<int>(lines.size()) - (end.row - start.row);
    lines.resize(rem_lines);
}

bool Editbox::insert_region(const std::string &str, const WindowState& window_state, TextPosition start, TextPosition end, EditAction& action) {
    int new_rows = static_cast<int>(std::count(str.begin(), str.end(), '\n'));
    if (lines.size() + new_rows - (end.row - start.row) > MAX_LINES) {
        // Would add too many lines
        return false;
    }
    if (new_rows == 0) {
        std::string old = extract_region(start, end);
        bool split_delete = false;
        if (start.row == end.row) {
            if (str.size() + start.col + line_size(end.row) - end.col > MAX_LINE_WIDTH) {
                // The first line would become too long
                return false;
            }
            delete_region(start, end);
        } else if (str.size() + start.col > MAX_LINE_WIDTH) {
            // The first line would become too long
            return false;
        }  else if (str.size() + start.col + line_size(end.row) - end.col > MAX_LINE_WIDTH) {
            delete_region(start, {start.row, line_size(start.row)});
            delete_region({start.row + 1, 0}, end);
            split_delete = true;
        } else {
            delete_region(start, end);
        }
        TextPosition start_pos = start;
        std::string s = lines[start.row].get_text();
        s.insert(start.col, str);
        lines[start.row].set_text(s);
        start.col += static_cast<int>(str.size());
        if (split_delete) {
            action = {start_pos, {start.row + 1, 0}, old};
        } else {
            action = {start_pos, start, old};
        }
    } else {
        size_t ix = str.find('\n');
        std::string first = str.substr(0, ix);
        if (start.col + first.size() > MAX_LINE_WIDTH) {
            return false;
        }
        size_t last_ix = str.rfind('\n');
        std::string last = str.substr(last_ix + 1);
        if (line_size(end.row) - end.col + last.size() > MAX_LINE_WIDTH) {
            return false;
        }
        std::string rem = str.substr(ix + 1, last_ix - ix);
        size_t offset = 0;
        for (int i = 0; i < new_rows - 1; ++i) {
            size_t pos = rem.find('\n', offset);
            if (pos - offset > MAX_LINE_WIDTH) {
                return false;
            }
            offset = pos + 1;
        }
        std::string old = extract_region(start, end);
        delete_region(start, end);
        std::string s = lines[start.row].get_text();
        std::string end_str = s.substr(start.col);
        s.erase(start.col);
        lines[start.row].set_text(s + first);
        offset = 0;
        for (int i = 1; i <= new_rows; ++i) {
            add_line(lines, window_state, start.row + i);
        }
        for (int i = 1; i < new_rows; ++i) {
            size_t pos = rem.find('\n', offset);
            lines[start.row + i].set_text(rem.substr(offset, pos - offset));
            offset = pos + 1;
        }
        TextPosition start_pos = start;
        lines[start.row + new_rows].set_text(last + end_str);
        start.row += new_rows;
        start.col = static_cast<int>(last.size());
        action = {start_pos, start, old};
    }
    return true;
}

bool Editbox::insert_str(const std::string &str, const WindowState& window_state, EditType edit) {
    EditAction action;
    if (selection_start == selection_end) {
        if (!insert_region(str, window_state, cursor_pos, cursor_pos, action)) {
            return false;
        }
    } else if (!insert_region(str, window_state, selection_start, selection_end, action)) {
        return false;
    }
    cursor_pos = action.end;
    clear_selection();
    reset_cursor_animation();
    if (edit == UNDO) {
        redo_stack.push(action);
    } else {
        if (edit != REDO) {
            redo_stack.clear();
        }
        undo_stack.push(action);
    }
    edit_action = edit;
    return true;
}

void Editbox::clear_selection() {
    selection_start = cursor_pos;
    selection_end = cursor_pos;
    selection_base = cursor_pos;
}

TextPosition Editbox::find_pos(int mouse_x, int mouse_y) const {
    int row = (mouse_y - (y + BOX_TEXT_MARGIN)) / BOX_LINE_HEIGHT;
    if (row < 0) {
        row = 0;
    } else if (row >= lines.size()) {
        row = static_cast<int>(lines.size() - 1);
    }
    int col = (mouse_x - (x + BOX_TEXT_MARGIN)) / BOX_CHAR_WIDTH;
    if (col < 0) {
        col = 0;
    } else if (col > line_size(row)) {
        col = line_size(row);
    }
    return {row, col};
}

void Editbox::update_selection(TextPosition pos) {
    if (!(pos >= selection_base)) {
        selection_start = pos;
        selection_end = selection_base;
    } else {
        selection_start = selection_base;
        selection_end = pos;
    }
    cursor_pos = pos;
}

void Editbox::move_cursor(TextPosition pos, bool select) {
    if (select) {
        update_selection(pos);
    } else {
        cursor_pos = pos;
        clear_selection();
    }
    edit_action = NONE;
    reset_cursor_animation();
}

void Editbox::input_char(char c, const WindowState& window_state) {
    if (!box_selected) {
        return;
    }

    if (valid_char(c)) {
        EditAction action;
        if (insert_region(std::string(1, c), window_state, selection_start, selection_end, action)) {
            if (edit_action == WRITE) {
                undo_stack.top().end = action.end;
                LOG_DEBUG("CHANGE: (%d, %d) - (%d, %d)", undo_stack.top().start.row, undo_stack.top().start.col, undo_stack.top().end.row, undo_stack.top().end.col);
            } else {
                edit_action = WRITE;
                redo_stack.clear();
                undo_stack.push(action);
            }
            cursor_pos = action.end;
            clear_selection();
        }
        reset_cursor_animation();
    }
}

void Editbox::tick(Uint64 passed, bool mouse_down, const WindowState& window_state) {
    if (mouse_down && box_selected) {
        TextPosition pos = find_pos(window_state.mouseX, window_state.mouseY);
        update_selection(pos);
    }

    ticks_remaining -= static_cast<Sint64>(passed);
    if (box_selected && ticks_remaining < 0) {
        ticks_remaining = 500;
        show_cursor = !show_cursor;
    }
}

void Editbox::select(const WindowState& window_state) {
    TextPosition pos = find_pos(window_state.mouseX, window_state.mouseY);
    box_selected = true;
    bool shift_pressed = window_state.keyboard_state[SDL_SCANCODE_RSHIFT] ||
                         window_state.keyboard_state[SDL_SCANCODE_LSHIFT];

    if (shift_pressed) {
        update_selection(pos);
    } else {
        cursor_pos = pos;
        clear_selection();
    }
    edit_action = NONE;

    show_cursor = true;
    ticks_remaining = 800;
}

void Editbox::unselect() {
    box_selected = false;
    show_cursor = false;
    edit_action = NONE;
}

void Editbox::reset_cursor_animation() {
    ticks_remaining = 500;
    show_cursor = true;
}

int Editbox::line_size(int row) const {
    return static_cast<int>(lines[row].get_text().size());
}

bool Editbox::move_left(TextPosition &pos, int off) const {
    TextPosition old = pos;
    while (pos.col - off < 0) {
        if (pos.row == 0) {
            pos = old;
            return false;
        }
        off -= 1 + pos.col;
        --pos.row;
        pos.col = line_size(pos.row);
    }
    pos.col -= off;
    return true;
}

bool Editbox::move_right(TextPosition &pos, int off) const {
    TextPosition old = pos;
    while (pos.col + off > line_size(pos.row)) {
        if (pos.row == lines.size() - 1) {
            pos = old;
            return false;
        }
        off -= 1 + line_size(pos.row) - pos.col;
        ++pos.row;
        pos.col = 0;
    }
    pos.col += off;
    return true;
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

void Editbox::handle_keypress(SDL_Keycode key, const WindowState &window_state) {
    bool shift_pressed = window_state.keyboard_state[SDL_SCANCODE_RSHIFT] ||
                         window_state.keyboard_state[SDL_SCANCODE_LSHIFT];
    bool ctrl_pressed = window_state.keyboard_state[SDL_SCANCODE_LCTRL] ||
                        window_state.keyboard_state[SDL_SCANCODE_RCTRL];

    if (key == SDLK_HOME) {
        move_cursor({cursor_pos.row, 0}, shift_pressed);
    } else if (key == SDLK_END) {
        move_cursor({cursor_pos.row, line_size(cursor_pos.row)}, shift_pressed);
    } else if (key == SDLK_TAB || key == SDLK_KP_TAB) {
        insert_str(std::string(SPACES_PER_TAB, ' '), window_state);
    } else if (key == 'a' && ctrl_pressed) {
        selection_start = selection_base = {0, 0};
        selection_end = {static_cast<int>(lines.size() - 1),
                         line_size(static_cast<int>(lines.size() - 1))};
        cursor_pos = selection_end;
        reset_cursor_animation();
    } else if ((key == 'z' && ctrl_pressed && shift_pressed) || key == 'y' && ctrl_pressed) {
        if (selection_start != selection_end) {
            clear_selection();
        } else if (redo_stack.size() > 0) {
            EditAction action = redo_stack.pop();
            selection_start = action.start;
            selection_end = action.end;
            cursor_pos = selection_start;
            insert_str(action.text, window_state, REDO);
            edit_action = NONE;
        }
    } else if (key == 'z' && ctrl_pressed) {
        if (selection_start != selection_end) {
            clear_selection();
        } else if (undo_stack.size() > 0) {
            EditAction action = undo_stack.pop();
            selection_start = action.start;
            selection_end = action.end;
            cursor_pos = selection_start;
            insert_str(action.text, window_state, UNDO);
            edit_action = NONE;
        }
    } else if (key == 'v' && ctrl_pressed) {
        char* clip = SDL_GetClipboardText();
        std::string s{clip};
        validate_string(s);
        SDL_free(clip);
        insert_str(s, window_state);
    } else if ((key == 'c' || key == 'x') && ctrl_pressed) {
        if (selection_start == selection_end) {
            return;
        }
        std::string s = extract_region(selection_start, selection_end);
        LOG_DEBUG("Copying '%s' to clipboard", s.c_str());
        SDL_SetClipboardText(s.c_str());
        if (key == 'x') {
            insert_str("", window_state);
        }
    } else if (key == SDLK_DOWN) {
        if (!shift_pressed && selection_start != selection_end) {
            clear_selection();
        }
        if (cursor_pos.row == lines.size() - 1) {
            int size = line_size(cursor_pos.row);
            cursor_pos.col = size;
            move_cursor(cursor_pos, shift_pressed);
        } else {
            ++cursor_pos.row;
            int size = line_size(cursor_pos.row);
            if (cursor_pos.col > size) {
                cursor_pos.col = size;
            }
            move_cursor(cursor_pos, shift_pressed);
        }
    } else if (key == SDLK_UP) {
        if (!shift_pressed && selection_start != selection_end) {
            clear_selection();
        }
        if (cursor_pos.row == 0) {
            move_cursor({0, 0}, shift_pressed);
        } else {
            --cursor_pos.row;
            int size = line_size(cursor_pos.row);
            if (cursor_pos.col > size) {
                cursor_pos.col = size;
            }
            move_cursor(cursor_pos, shift_pressed);
        }
    } else if (key == SDLK_LEFT) {
        if (!shift_pressed && selection_start != selection_end) {
            cursor_pos = selection_start;
            move_cursor(cursor_pos, false);
        } else {
            if (!move_left(cursor_pos, 1)) {
                return;
            }
            move_cursor(cursor_pos, shift_pressed);
        }
    } else if (key == SDLK_RIGHT) {
        if (!shift_pressed && selection_start != selection_end) {
            cursor_pos = selection_end;
            move_cursor(cursor_pos, false);
        } else {
            if (!move_right(cursor_pos, 1)) {
                return;
            }
            move_cursor(cursor_pos, shift_pressed);
        }
    } else if (key == SDLK_DELETE) {
        reset_cursor_animation();
        if (selection_start == selection_end) {
            if (!move_right(selection_end, 1)) {
                return;
            }
        }
        EditAction action;
        if (insert_region("", window_state, selection_start, selection_end, action)) {
            if (edit_action == DELETE) {
                EditAction& old = undo_stack.top();
                old.start = old.end = action.start;
                old.text = old.text + action.text;
            } else {
                redo_stack.clear();
                undo_stack.push(action);
                edit_action = DELETE;
            }
            cursor_pos = action.start;
            clear_selection();
        }
    } else if (key == SDLK_BACKSPACE) {
        reset_cursor_animation();
        if (selection_start == selection_end) {
            if (!move_left(selection_start, 1)) {
                return;
            }
        }
        EditAction action;
        if (insert_region("", window_state, selection_start, selection_end, action)) {
            if (edit_action == BACKSPACE) {
                EditAction& old = undo_stack.top();
                old.start = old.end = action.start;
                old.text = action.text + old.text;
            } else {
                redo_stack.clear();
                undo_stack.push(action);
                edit_action = BACKSPACE;
            }
            cursor_pos = action.start;
            clear_selection();
        }
    } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
        insert_str("\n", window_state);
    }
}

void Editbox::render(WindowState &window_state) {
    SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
    SDL_Rect rect = {x, y, BOX_SIZE, BOX_SIZE};
    SDL_RenderDrawRect(gRenderer, &rect);

    if (selection_start != selection_end) {
        SDL_SetRenderDrawColor(gRenderer, 0x50, 0x50, 0x50, 0xff);
        int start = selection_start.col;
        for (int row = selection_start.row; row < selection_end.row; ++row) {
            int size = line_size(row);
            SDL_Rect r = {x + BOX_TEXT_MARGIN + BOX_CHAR_WIDTH * start,
                          y + BOX_LINE_HEIGHT * row + BOX_TEXT_MARGIN,
                          BOX_CHAR_WIDTH * (size + 1 - start),
                          BOX_LINE_HEIGHT};
            SDL_RenderFillRect(gRenderer, &r);
            start = 0;
        }
        int col = selection_end.col;
        SDL_Rect r = {x + BOX_TEXT_MARGIN + BOX_CHAR_WIDTH * start,
                      y + BOX_LINE_HEIGHT * selection_end.row + BOX_TEXT_MARGIN,
                      BOX_CHAR_WIDTH * (col - start),
                      BOX_LINE_HEIGHT};
        SDL_RenderFillRect(gRenderer, &r);
    }

    for (auto& box: lines) {
        box.render(x + BOX_TEXT_MARGIN, y + BOX_TEXT_MARGIN, window_state);
    }

    if (show_cursor) {
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        SDL_RenderDrawLine(gRenderer, x + BOX_TEXT_MARGIN + BOX_CHAR_WIDTH * cursor_pos.col,
                           y + BOX_LINE_HEIGHT  * cursor_pos.row + BOX_TEXT_MARGIN,
                           x + BOX_TEXT_MARGIN + BOX_CHAR_WIDTH * cursor_pos.col,
                           y + BOX_LINE_HEIGHT * (cursor_pos.row + 1) + BOX_TEXT_MARGIN);
    }
}
void Editbox::set_dpi_scale(double dpi) {
    for (auto& line: lines) {
        line.set_dpi_ratio(dpi);
    }
}
bool Editbox::is_pressed(int mouse_x, int mouse_y) const {
    return mouse_x > x && mouse_x < x + BOX_SIZE && mouse_y > y && mouse_y < y + BOX_SIZE;
}
