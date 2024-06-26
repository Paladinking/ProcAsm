#include "ui.h"
#include <utility>
#include "engine/log.h"

TTF_Font *TextBox::font;

constexpr int DEFAULT_DPI = 70;

void TextBox::init(TTF_Font *font_data) {
    TextBox::font = font_data;
    if (TextBox::font == nullptr) {
        throw game_exception(std::string(TTF_GetError()));
    }
}

TextBox::TextBox(const int x, const int y, const int w, const int h,
                 const std::string &text, const WindowState &window_state)
    : TextBox(x, y, w, h, text, 20, window_state) {}

TextBox::TextBox(const int x, const int y, const int w, const int h,
                 std::string text, const int font_size,
                 const WindowState &window_state)
    : x(x), y(y), w(w), h(h), text(std::move(text)), font_size(font_size),
      dpi_ratio(std::min(static_cast<double>(window_state.window_width) /
                             window_state.screen_width,
                         static_cast<double>(window_state.window_height) /
                             window_state.screen_height)),
      texture((SDL_Texture *)nullptr, 0, 0) {
    generate_texture();
}

void TextBox::generate_texture() {
    if (text.empty()) {
        texture.free();
        return;
    }

    SDL_Surface *text_surface =
        TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, 0);
    if (text_surface == nullptr) {
        throw image_load_exception(std::string(TTF_GetError()));
    }
    int width = text_surface->w;
    int height = text_surface->h;
    SDL_Texture *text_texture =
        SDL_CreateTextureFromSurface(gRenderer, text_surface);
    SDL_FreeSurface(text_surface);

    if (text_texture == nullptr) {
        throw image_load_exception(std::string(SDL_GetError()));
    }
    texture = Texture(text_texture, width, height);

    TTF_SizeUTF8(font, text.c_str(), &width, &height);
    if (alignment == Alignment::LEFT) {
        text_offset_x = 0;
    } else if (alignment == Alignment::CENTRE) {
        text_offset_x = static_cast<int>((w - width / dpi_ratio) / 2);
    } else {
        text_offset_x = static_cast<int>(w - width / dpi_ratio);
    }
    text_offset_y = static_cast<int>((h - height / dpi_ratio) / 2);
}

void TextBox::set_dpi_ratio(double dpi) {
    dpi_ratio = dpi;
    generate_texture();
}

void TextBox::set_position(const int new_x, const int new_y) {
    x = new_x;
    y = new_y;
}

void TextBox::set_text(const std::string &new_text) {
    text = new_text;
    generate_texture();
}

void TextBox::set_align(Alignment align) {
    alignment = align;
    if (align == Alignment::LEFT) {
        text_offset_x = 0;
    } else if (align == Alignment::CENTRE) {
        int width, height;
        TTF_SizeUTF8(font, text.c_str(), &width, &height);
        text_offset_x = static_cast<int>((w - width / dpi_ratio) / 2);
    } else {
        int width, height;
        TTF_SizeUTF8(font, text.c_str(), &width, &height);
        text_offset_x = static_cast<int>(w - width / dpi_ratio);
    }
}

void TextBox::set_text_color(const Uint8 r, const Uint8 g, const Uint8 b,
                             const Uint8 a) {
    color = {r, g, b, a};
    generate_texture();
}

const SDL_Color &TextBox::get_text_color() const { return color; }

void TextBox::set_font_size(const int new_font_size) {
    font_size = new_font_size;
    generate_texture();
}

const std::string &TextBox::get_text() const { return text; }

void TextBox::set_dimensions(const int new_w, const int new_h) {
    text_offset_x = (new_w - w) / 2 + text_offset_x;
    text_offset_y = (new_h - h) / 2 + text_offset_y;
    w = new_w;
    h = new_h;
}

void TextBox::render(const int x_offset, const int y_offset,
                     const WindowState &window_state) const {
    if (text.empty()) {
        return;
    }
    // When rendering text, set logical size to the size of the window to allow
    // better quality text. Because of this, need to manually adjust for DPI.
    SDL_RenderSetLogicalSize(gRenderer, dpi_ratio * window_state.screen_width,
                             dpi_ratio * window_state.screen_height);
    texture.render_corner(
        static_cast<int>(dpi_ratio * (x_offset + x + text_offset_x)),
        static_cast<int>(dpi_ratio * (y_offset + y + text_offset_y)));
    SDL_RenderSetLogicalSize(gRenderer, window_state.screen_width,
                             window_state.screen_height);
}

bool Button::is_pressed(int mouseX, int mouseY) const {
    return mouseX >= x && mouseX < x + w && mouseY >= y && mouseY < y + h;
}

void Button::set_hover(const bool new_hover) { hover = new_hover; }

void Button::set_border(const bool new_border) { border = new_border; }

bool Button::handle_press(int mouseX, int mouseY, bool press) {
    if (!is_pressed(mouseX, mouseY)) {
        down = false;
    } else if (press) {
        down = true;
    } else if (down){
        down = false;
        return true;
    } else {
        down = false;
    }
    return false;
}

void Button::render(const int x_offset, const int y_offset,
                    const WindowState &window_state) const {
    SDL_Rect r = {x + x_offset, y + y_offset, w, h};
    if (border) {
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        SDL_RenderFillRect(gRenderer, &r);
        r.x += 2;
        r.y += 2;
        r.w -= 4;
        r.h -= 4;
    }
    if (down) {
        SDL_SetRenderDrawColor(gRenderer, 50, 50, 50, 0xFF);
    } else if (hover) {
        SDL_SetRenderDrawColor(gRenderer, 80, 80, 80, 0xFF);
    } else {
        SDL_SetRenderDrawColor(gRenderer, 30, 30, 30, 0xFF);
    }
    SDL_RenderFillRect(gRenderer, &r);

    TextBox::render(x_offset, y_offset, window_state);
}

Dropdown::Dropdown(int x, int y, int w, int h, const std::string& text, const std::vector<std::string>& choices, const WindowState& window_state) :
    base(x, y, w, h, text, window_state), default_value(text), ix{-1} {
    set_choices(choices, window_state);
}

void Dropdown::render(int x_offset, int y_offset, const WindowState& window_state) const {
    const_cast<Button*>(&base)->set_hover(base.is_pressed(window_state.mouseX - x_offset, window_state.mouseY - y_offset));
    base.render(x_offset, y_offset, window_state);
    float x_base = base.x + x_offset + base.w - 20.0f;
    float y_base = base.y + y_offset + base.h / 2.0f - 4;
    if (show_list) {
        for (auto & btn: choices) {
            const_cast<Button*>(&btn)->set_hover(btn.is_pressed(window_state.mouseX - x_offset, window_state.mouseY - y_offset));
            btn.render(x_offset, y_offset, window_state);
        }
        SDL_Rect r = {x_offset + base.x, y_offset + base.y + base.h, max_w + 8, static_cast<int>(choices.size()) * (max_h + 16)};
        SDL_SetRenderDrawColor(gRenderer, 0xf0, 0xf0, 0xf0, 0xff);
        SDL_RenderDrawRect(gRenderer, &r);
        SDL_Vertex verticies[3] = {
            {{x_base, y_base + 10}, {0xf0, 0xf0, 0xf0, 0xff}, {0.0f, 0.0f}},
            {{x_base + 10, y_base + 10}, {0xf0, 0xf0, 0xf0, 0xff}, {0.0f, 0.0f}},
            {{x_base + 5, y_base}, {0xf0, 0xf0, 0xf0, 0xff}, {0.0f, 0.0f}}
        };
        SDL_RenderGeometry(gRenderer, nullptr, verticies, 3, nullptr, 0);
    } else {
        SDL_Vertex verticies[3] = {
            {{x_base, y_base}, {0xf0, 0xf0, 0xf0, 0xff}, {0.0f, 0.0f}},
            {{x_base + 10, y_base}, {0xf0, 0xf0, 0xf0, 0xff}, {0.0f, 0.0f}},
            {{x_base + 5, y_base + 10}, {0xf0, 0xf0, 0xf0, 0xff}, {0.0f, 0.0f}}
        };
        SDL_RenderGeometry(gRenderer, nullptr, verticies, 3, nullptr, 0);
    }
}

void Dropdown::set_choices(const std::vector<std::string>& choices, const WindowState& window_state) {
    this->choices.clear();
    this->clear_choice();
    max_w = base.w - 8, max_h = 0;
    for (int i = 0; i < choices.size(); ++i) {
        const std::string str = " " + choices[i];
        int tw, th;
        TTF_SizeUTF8(gFont, str.c_str(), &tw, &th);
        if (tw > max_w) {
            max_w = tw;
        }
        if (th > max_h) {
            max_h = th;
        }
    }
    for (int i = 0; i < choices.size(); ++i) {
        const std::string& str = choices[i];
        this->choices.emplace_back(base.x, base.y + base.h + (max_h + 16) * i,
                                   max_w + 8, max_h + 16, " " + choices[i], window_state);
        this->choices.back().set_align(Alignment::LEFT);
        this->choices.back().set_border(false);
    }
}

void Dropdown::set_dpi_ratio(double dpi) {
    base.set_dpi_ratio(dpi);
    for (auto& btn: choices) {
        btn.set_dpi_ratio(dpi);
    }
}

int Dropdown::handle_press(int mouseX, int mouseY, bool press) {
    if (base.handle_press(mouseX, mouseY, press)) {
        show_list = !show_list;
    }
    if (show_list) {
        int pressed = -1;
        for (int i = 0; i < choices.size(); ++i) {
            auto& btn = choices[i];
            if (btn.handle_press(mouseX, mouseY, press)) {
                pressed = i;
                show_list = false;
            }
        }
        if (pressed != -1) {
            ix = pressed;
            base.set_text(choices[pressed].get_text().substr(1));
        }
        return pressed;
    }
    return -1;
}

void Dropdown::clear_choice() {
    ix = -1;
    base.set_text(default_value);
}

int Dropdown::get_choice() const {
    return ix;
}

void Dropdown::set_text_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    base.set_text_color(r, g, b, a);

    for (auto& btn: choices) {
        btn.set_text_color(r, g, b, a);
    }
}

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
