#ifndef PROCASM_EDITLINES_H
#define PROCASM_EDITLINES_H
#include <string>
#include <vector>
#include "config.h"

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
    bool chain = false;
};

class EditStack {
public:
    unsigned size() const;

    void clear();

    void push(const EditAction& action);

    EditAction& top();

    EditAction pop();
private:
    unsigned pos {0}, data_size {0};
    std::vector<EditAction> data {};
};

enum class EditType {
    NONE, UNDO, REDO, WRITE, DELETE, BACKSPACE, INSERT
};

class EditLines {
public:
    EditLines(int max_rows, int max_cols, void (*change_callback)(TextPosition start, TextPosition end, int removed, void*), void* aux);

    bool move_left(TextPosition &pos, int off) const;

    bool move_right(TextPosition &pos, int off) const;

    bool has_selection() const;

    void move_cursor(TextPosition pos, bool select);

    void clear_selection();

    bool insert_str(const std::string& str, EditType type = EditType::NONE);

    const TextPosition& get_cursor_pos() const;

    const TextPosition& get_selection_start() const;

    const TextPosition& get_selection_end() const;

    void set_selection(TextPosition start, TextPosition end, bool cursor_at_end);

    const std::vector<std::string> &get_lines() const;

    std::string extract_selection() const;

    void clear_action();

    void undo_action(bool redo);

    int line_count() const;

    int line_size(int row) const;

    char char_at_pos(TextPosition pos) const;
private:
    void delete_region(TextPosition start, TextPosition end);

    std::string extract_region(TextPosition start, TextPosition end) const;

    bool insert_region(const std::string &str, TextPosition start, TextPosition end, EditAction& action);

    EditType edit_action {EditType::NONE};

    EditStack undo_stack {};
    EditStack redo_stack {};

    TextPosition cursor_pos {};

    TextPosition selection_start {};
    TextPosition selection_end {};
    TextPosition selection_base {};

    std::vector<std::string> lines {};

    void (*change_callback)(TextPosition start, TextPosition end, int removed, void*);
    void* aux_data;

    std::size_t max_rows, max_cols;
};

#endif // PROCASM_EDITLINES_H
