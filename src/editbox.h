#ifndef PROCASM_EDITBOX_H
#define PROCASM_EDITBOX_H

#include "config.h"
#include "engine/game.h"
#include "engine/log.h"
#include "engine/ui.h"
#include <vector>

struct TextPosition {
    int row, col;

    bool operator<(const TextPosition& other) const;
    bool operator>(const TextPosition& other) const;
    bool operator<=(const TextPosition& other) const;
    bool operator>=(const TextPosition& other) const;
    bool operator==(const TextPosition& other) const;
    bool operator!=(const TextPosition& other) const;
};

struct EditAction {
    TextPosition start {};
    TextPosition end {};

    std::string text {};
};

class EditStack {
public:
    unsigned size() const {
        return data_size;
    }

    void clear() {
        data_size = 0;
        pos = 0;
    }

    void diff() {
        EditAction& action = top();
        std::string rep = action.text;
        for (int i = 0; i < rep.size(); ++i) {
            if (rep[i] == '\n') {
                rep.insert(rep.begin() + i, '\\');
                rep[++i] = 'n';
            }
        }
        LOG_DEBUG("STACK_EDIT(%p) (%u): (%d, %d) - (%d, %d): '%s'", this, data_size, action.start.row, action.start.col, action.end.row, action.end.col, rep.c_str());

    }

    void push(const EditAction& action) {
        std::string rep = action.text;
        for (int i = 0; i < rep.size(); ++i) {
            if (rep[i] == '\n') {
                rep.insert(rep.begin() + i, '\\');
                rep[++i] = 'n';
            }
        }
        LOG_DEBUG("STACK_APPEND(%p) (%u): (%d, %d) - (%d, %d): '%s'", this, data_size, action.start.row, action.start.col, action.end.row, action.end.col, rep.c_str());
        if (data_size < BOX_UNDO_BUFFER_SIZE) {
            ++data_size;
        }
        if (pos == data.size()) {
            data.push_back(action);
        } else {
            data[pos] = action;
        }
        pos = (pos + 1) % BOX_UNDO_BUFFER_SIZE;
    }

    EditAction& top() {
        return data[(pos + BOX_UNDO_BUFFER_SIZE - 1) % BOX_UNDO_BUFFER_SIZE];
    }

    EditAction pop() {
        --data_size;
        EditAction res = std::move(top());
        pos = (pos + BOX_UNDO_BUFFER_SIZE - 1) % BOX_UNDO_BUFFER_SIZE;
        return res;
    }
private:
    unsigned pos {0}, data_size {0};
    std::vector<EditAction> data {};
};

class Editbox {
public:
    Editbox(int x, int y, const WindowState &window_state);

    Editbox() = default;

    void render(WindowState &window_state);

    void set_dpi_scale(double dpi);

    void tick(Uint64 passed, bool mouse_down, const WindowState& window_state);

    [[nodiscard]] bool is_pressed(int mouse_x, int mouse_y) const;

    void handle_keypress(SDL_Keycode key, const WindowState &window_state);

    void select(const WindowState& window_state);

    void unselect();

    void input_char(char c, const WindowState& window_state);
private:
    int line_size(int row) const;

    TextPosition find_pos(int mouse_x, int mouse_y) const;

    void delete_region(TextPosition start, TextPosition end);

    void delete_selection();

    std::string extract_selection() const;

    bool insert_str(const std::string& str, const WindowState& window_state, bool undo, bool clear_redo = true);

    void clear_selection();

    void move_cursor(TextPosition pos, bool select);

    void update_selection(TextPosition pos);

    void reset_cursor_animation();

    int x{}, y{};

    bool box_selected = false;

    EditStack undo_stack {};
    EditStack redo_stack {};

    enum {
        NONE, WRITE, DELETE, BACKSPACE
    } edit_action = NONE;

    TextPosition selection_start {};
    TextPosition selection_end {};
    TextPosition selection_base {};

    bool show_cursor = false;
    Sint64 ticks_remaining = 0;

    TextPosition cursor_pos {};

    std::vector<TextBox> lines {};
};


#endif // PROCASM_EDITBOX_H
