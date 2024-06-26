#ifndef UI_00_H
#define UI_00_H
#include "game.h"
#include "input.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <string>
#include <vector>

enum class Alignment {
    LEFT, CENTRE, RIGHT
};

class Dropdown;

class TextBox {
public:
    TextBox() = default;

    TextBox(int x, int y, int w, int h, const std::string &text,
            const WindowState &window_state);

    TextBox(int x, int y, int w, int h, std::string text, int font_size,
            const WindowState &window_state);

    /**
     * Sets the text of the textbox.
     */
    void set_text(const std::string &text);

    /**
     * Sets the font size of the textbox.
     */
    void set_font_size(int font_size);

    /**
     * Gets the text of the textbox.
     */
    const std::string &get_text() const;

    /**
     * Sets the position of the textbox (upper corner) to (x, y).
     */
    void set_position(int x, int y);

    /**
     * Sets the dimensions of the textbox to (w, h).
     */
    void set_dimensions(int w, int h);

    /**
     * Sets the color of the text.
     */
    void set_text_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a);

    /**
     * Sets horizontal alignment of the text.
     */
     void set_align(Alignment alignment);

    /**
     * Gets the color of the text;
     */
    [[nodiscard]] const SDL_Color &get_text_color() const;

    /**
     * Renders the textbox.
     */
    virtual void render(int x_offset, int y_offset,
                        const WindowState &window_state) const;

    /**
     * Initializes the button class, loading the font used for the button text.
     */
    static void init(TTF_Font *font_data);

    void set_dpi_ratio(double dpi);

protected:
    friend class Dropdown;

    int x{}, y{}, w{}, h{};

    int font_size{};

    int text_offset_x{};

    int text_offset_y{};

    std::string text;

    double dpi_ratio{};

private:

    Texture texture;

    SDL_Color color = {0, 0, 0, 0};

    Alignment alignment = Alignment::CENTRE;

    /**
     * Generates the texture containing the text of the button. Called by
     * constructor and set_text.
     */
    void generate_texture();

    static TTF_Font *font;
};

class Button : public TextBox {
public:
    /**
     * Default initialization
     */
    Button() = default;

    /**
     * Constructs a button with given size and text positioned at given
     * location.
     */
    Button(const int x, const int y, const int w, const int h,
           const std::string &text, const WindowState &window_state)
        : TextBox(x, y, w, h, text, window_state){};
    Button(const int x, const int y, const int w, const int h,
           const std::string &text, const int font_size,
           const WindowState &window_state)
        : TextBox(x, y, w, h, text, font_size, window_state){};

    /**
     * Returns true if the button contains the point (mouseX, mousey).
     */
    [[nodiscard]] bool is_pressed(int mouseX, int mouseY) const;

    /**
     * Sets the hover state of this button.
     */
    void set_hover(bool hover);

    /**
     * Hides or shows the border of this button.
     */
    void set_border(bool border);

    /**
     * Handles press logic on a mouse event. Returns true when pressed.
     */
    bool handle_press(int mouseX, int mouseY, bool down);

    /**
     * Renders the button.
     */
    void render(int x_offset, int y_offset,
                const WindowState &window_state) const override;

private:
    bool border = true;
    bool hover = false;
    bool down = false;
};

class Dropdown {
public:
    Dropdown() = default;

    Dropdown(int x, int y, int w, int h, const std::string& text, const std::vector<std::string>& choices, const WindowState& window_state);

    void clear_choice();

    int get_choice() const;

    void render(int x_offset, int y_offset, const WindowState& window_state) const;

    int handle_press(int mouseX, int mouseY, bool down);

    void set_text_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    void set_dpi_ratio(double dpi);

    void set_choices(const std::vector<std::string>& choices, const WindowState& window_state);

private:
    bool show_list = false;

    int max_w, max_h;

    int ix;

    std::string default_value;
    
    Button base;

    std::vector<Button> choices {};
};

class Menu : public State {
public:
    Menu();

    explicit Menu(const std::string &exit_input);

    /**
     * Handles a down-event of keyboard or mouse.
     */
    void handle_down(SDL_Keycode key, Uint8 mouse) override;

    /**
     * Handles an up-event of keyboard or mouse.
     */
    void handle_up(SDL_Keycode key, Uint8 mouse) override;

    /**
     * Renders the full menu.
     */
    void render() override;

    /**
     * Ticks the menu, deciding if to switch state.
     */
    void tick(Uint64 delta, StateStatus &res) override;

protected:
    std::vector<Button> buttons;
    std::vector<TextBox> text;

    // Set by subclasses to swap state
    StateStatus next_res;

    /**
     * Called when a button is pressed.
     * The int btn will contain the index of the button in the buttons vector.
     */
    virtual void button_press(int btn) = 0;

    /**
     * Called when the Menu_exit input is recieved (Typicly Escape).
     * This function allows most menu subclasses not to override handle_down or
     * handle_up.
     */
    virtual void menu_exit();

private:
    int targeted_button = 0;

    std::unique_ptr<PressInput> exit_input;
};

#endif
