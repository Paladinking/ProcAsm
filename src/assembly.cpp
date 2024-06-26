#include "assembly.h"
#include "event_id.h"
#include "engine/log.h"
#include "config.h"

GameState::GameState() : State() {}

void GameState::set_font_size() {
    // This is very cursed, but works on windows with the current font for sizes 1.0, 1.25, 1.5 and 1.75
    double new_dpi_scale = std::min(static_cast<double>(window_state->window_width) /
                                              window_state->screen_width,
                                          static_cast<double>(window_state->window_height) /
                                              window_state->screen_height);
    if (new_dpi_scale == dpi_scale) {
        return;
    }
    LOG_DEBUG("Recalculating font size");
    dpi_scale = new_dpi_scale;
    int hpdi = 72, vdpi = 72;
    TTF_SetFontSizeDPI(gFont, 20, hpdi, vdpi);
    int target_w, target_h;
    int base_w, base_h;
    int w, h;
    TTF_SizeUTF8(gFont, "a", &base_w, &base_h);
    target_w = static_cast<int>(base_w * dpi_scale);
    target_h = static_cast<int>(base_h * dpi_scale);
    TTF_SetFontSizeDPI(gFont, 20, hpdi, vdpi);
    TTF_SizeUTF8(gFont, "a", &w, &h);

    LOG_DEBUG("Finding font size for dpi %f", dpi_scale);
    while (true) {
        if (w == target_w && h == target_h) break;
        int old_hdpi = hpdi;
        int old_vdpi = vdpi;
        if (w > target_w) {
            --hpdi;
        } else if (w < target_w) {
            ++hpdi;
        }
        if (h > target_h) {
            --vdpi;
        } else if (h < target_h) {
            ++vdpi;
        }
        int new_w, new_h;
        TTF_SetFontSizeDPI(gFont, 20, hpdi, vdpi);
        TTF_SizeUTF8(gFont, "a", &new_w, &new_h);
        if (new_w <= target_w) {
            w = new_w;
        } else {
            hpdi = old_hdpi;
        }
        if (new_h <= target_h) {
            h = new_h;
        } else {
            vdpi = old_vdpi;
        }
        if ((w < new_w || w == target_w) && (h < new_h || h == target_h)) {
            break;
        }
    }
    LOG_DEBUG("Found font size");
    TTF_SetFontSizeDPI(gFont, 20, hpdi, vdpi);

    box.set_dpi_scale(dpi_scale);
    processor_gui.set_dpi(dpi_scale);
}

void GameState::handle_size_change() {
    SDL_RenderGetLogicalSize(gRenderer, &window_state->screen_width,
                             &window_state->screen_height);
    SDL_GetRendererOutputSize(gRenderer, &window_state->window_width,
                              &window_state->window_height);
    set_font_size();
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
    LOG_INFO("Logical size: %d, %d\n", window_state->screen_width, window_state->screen_height);

    box.~Editbox();
    new (&box)Editbox{BOX_X, BOX_Y, *window_state };

    processor_gui.~ProcessorGui();
    new (&processor_gui)ProcessorGui(&processor, &problem, BOX_X, BOX_Y, window_state);

    set_font_size();

    SDL_RWops* file = SDL_RWFromFile("program.txt", "r");
    if (file != nullptr) {
        Sint64 size = SDL_RWsize(file);
        char *data = new char[size];
        if (SDL_RWread(file, data, 1, size) == size) {
            std::string str{data, static_cast<std::size_t>(size)};
            if (str.back() == '\n') {
                str.pop_back();
            }
            box.set_text(str);
        }
        delete[] data;
        SDL_RWclose(file);
    }
}

void GameState::render() {
    processor_gui.render();

    box.render();
}
void GameState::tick(const Uint64 delta, StateStatus &res) {
    if (should_exit) {
        SDL_RWops* file = SDL_RWFromFile("program.txt", "w");
        if (file != nullptr) {
            const auto& lines = box.get_text();
            for (const std::string& s : lines) {
                SDL_RWwrite(file, s.c_str(), 1, s.size());
                SDL_RWwrite(file, "\n", 1, 1);
            }
            SDL_RWclose(file);
        }
        res.action = StateStatus::EXIT;
        return;
    }

    box.tick(delta, mouse_down);
    if (processor.is_running()) {
        ticks_passed += delta;
        while (ticks_passed > TICK_DELAY) {
            problem.clock_tick();
            processor.clock_tick();
            ticks_passed -= TICK_DELAY;
        }
    }
        //processor.clock_tick();
}

void GameState::handle_up(SDL_Keycode key, Uint8 mouse) {
    if (mouse == SDL_BUTTON_LEFT && mouse_down) {
        mouse_down = false;
        processor_gui.mouse_change(false);
        if (!box.is_pressed(window_state->mouseX, window_state->mouseY)) {
            box.unselect();
            SDL_StopTextInput();
            if (!processor.is_valid()) {
                std::vector<ErrorMsg> errors;
                const auto& text = box.get_text();
                processor.compile_program(text, errors);
                box.set_errors(errors);
            }
        }
    }
}

void GameState::handle_down(const SDL_Keycode key, const Uint8 mouse) {
    if (key == SDLK_ESCAPE) {
        should_exit = true;
    } else if (mouse == SDL_BUTTON_LEFT) {
        mouse_down = true;
        processor_gui.mouse_change(true);
        if (box.is_pressed(window_state->mouseX, window_state->mouseY)) {
            SDL_StartTextInput();
            box.select();
            processor.invalidate();
        } else {
            box.unselect();
            SDL_StopTextInput();
        }
    } else {
        box.handle_keypress(key);
    }
}

void GameState::handle_textinput(const SDL_TextInputEvent &e) {
    if (e.text[0] == '\0' || e.text[1] != '\0') {
        LOG_DEBUG("Invalid input received");
        return;
    }
    char c = e.text[0];
    box.input_char(c);
}

void GameState::handle_focus_change(bool focus) {
    if (!focus) {
        box.unselect();
        mouse_down = false;
        SDL_StopTextInput();
    }
}

