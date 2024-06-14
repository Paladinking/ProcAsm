#include "ui.h"
#include "log.h"
#include <utility>

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
    //TTF_SetFontSizeDPI(font, 20, 114, 123);
    SDL_Surface *text_surface =
        TTF_RenderUTF8_Blended(font, text.c_str(), color);
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
    if (width < 40) {
        LOG_DEBUG("Size: %d, %d", width, height);
    }
    if (left_align) {
        text_offset_x = 0;
    } else {
        text_offset_x = static_cast<int>((w - width / dpi_ratio) / 2);
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

void TextBox::set_left_align(bool enabled) {
    left_align = enabled;
    if (enabled) {
        text_offset_x = 0;
    } else {
        int width, height;
        //TTF_SetFontSize(font, static_cast<int>(dpi_ratio * font_size));
        TTF_SizeUTF8(font, text.c_str(), &width, &height);
        text_offset_x = static_cast<int>((w - width / dpi_ratio) / 2);
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
                     const WindowState &window_state) {
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

void Button::render(const int x_offset, const int y_offset,
                    const WindowState &window_state) {
    SDL_Rect r = {x + x_offset, y + y_offset, w, h};
    if (background != nullptr) {
        if (hover) {
            background->set_color_mod(200, 200, 200);
        } else {
            background->set_color_mod(255, 255, 255);
        }
        background->set_dimensions(r.w, r.h);
        background->render_corner(r.x, r.y);
    } else {
        if (hover) {
            SDL_SetRenderDrawColor(gRenderer, 200, 200, 240, 0xFF);
        } else {
            SDL_SetRenderDrawColor(gRenderer, 100, 100, 220, 0xFF);
        }
        SDL_RenderFillRect(gRenderer, &r);
    }

    TextBox::render(x_offset, y_offset, window_state);
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
