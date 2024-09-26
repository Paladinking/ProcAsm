#include "assembly.h"
#include "event_id.h"
#include "engine/log.h"
#include "config.h"
#include "processor_menu.h"

GameState::GameState(std::vector<ProcessorTemplate> temp) : State(), processor{temp[0].instantiate()}, templates{std::move(temp)} {}

void GameState::set_font_size() {
    // This is very cursed, but works on windows with the current font for sizes 1.0, 1.25, 1.5 and 1.75
    double new_dpi_scale = std::min(static_cast<double>(window_state->window_width) /
                                              window_state->screen_width,
                                          static_cast<double>(window_state->window_height) /
                                              window_state->screen_height);
    if (new_dpi_scale != dpi_scale) {
        return;
    }
    LOG_DEBUG("Recalculating font size");
    dpi_scale = new_dpi_scale;
    int hpdi = 72, vdpi = 72;
    TTF_SetFontSizeDPI(gFont, 20, hpdi, vdpi);
    return;
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

    processor_gui.~ProcessorGui();
    new (&processor_gui)ProcessorGui(&processor, &problem, BOX_X, BOX_Y, window_state);

    void(*proc_cb)(GameState*) = [](GameState* self) {
        self->processor_gui.menu_change(true);
        self->top_comps.enable_hover(false);
        self->next_state.action = StateStatus::PUSH;
        LOG_DEBUG("Template: %s", self->templates[0].name.c_str());
        self->next_state.new_state = new ProcessorMenu(self, self->templates, 0);
    };

    top_comps.set_window_state(window_state);

    top_comps.add(Button(BOX_X + BOX_SIZE + 20, BOX_Y + 20, 40, 40, "i", *window_state),
                  proc_cb, this);

    void(*run_pressed)(GameState*) = [](GameState* self) {
        if (self->processor.is_valid()) {
            if (self->processor.is_running()) {
                self->run_button->set_text("Run");
                self->processor.stop();
            } else {
                self->run_button->set_text("Stop");
                self->processor.start();
            }
        }
    };

    run_button = top_comps.add(Button(BOX_X + BOX_SIZE + 20,
                                      BOX_Y + BOX_SIZE + 200,
                                      80, 80, "Run", *window_state), run_pressed, this);

    void(*step_pressed)(GameState*) = [](GameState* self) {
        LOG_DEBUG("Step pressed");
        if (self->processor.is_valid()) {
            self->clock_tick();
            self->processor_gui.update();
        }
    };

    top_comps.add(Button(BOX_X + BOX_SIZE + 120, BOX_Y + BOX_SIZE + 200, 80, 80, "Step", *window_state), step_pressed, this);
    next_state.action = StateStatus::NONE;

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
            processor_gui.set_edit_text(str);
        }
        delete[] data;
        SDL_RWclose(file);
    }
}

void GameState::resume() {
    next_state.action = StateStatus::NONE;
    processor_gui.menu_change(false);
    top_comps.enable_hover(true);

    processor = templates[0].instantiate();

    processor_gui.set_processor(&processor);
    problem.reset();
}

void GameState::clock_tick() {
    problem.in_tick();
    processor.in_tick();

    problem.out_tick();
    processor.out_tick();

    processor.clock_tick();
}

void GameState::render() {
    processor_gui.render();
    top_comps.render(0, 0);
}
void GameState::tick(const Uint64 delta, StateStatus &res) {
    res = next_state;
    if (next_state.will_leave()) {
        LOG_DEBUG("Saving...");
        SDL_RWops* file = SDL_RWFromFile("program.txt", "w");
        if (file != nullptr) {
            const auto& lines = processor_gui.get_edit_text();
            for (const std::string& s : lines) {
                SDL_RWwrite(file, s.c_str(), 1, s.size());
                SDL_RWwrite(file, "\n", 1, 1);
            }
            SDL_RWclose(file);
        }
        return;
    }

    processor_gui.tick(delta);

    if (processor.is_running()) {
        ticks_passed += delta;
        while (ticks_passed > TICK_DELAY) {
            clock_tick();
            ticks_passed -= TICK_DELAY;
        }
    }

    if (processor.is_running()) {
        if (run_button->get_text()[0] != 'S') {
            run_button->set_text("Stop");
        }
        processor_gui.update();
    } else {
        if (run_button->get_text()[0] != 'R') {
            run_button->set_text("Run");
        }
    }
}

void GameState::handle_up(SDL_Keycode key, Uint8 mouse) {
    if (mouse == SDL_BUTTON_LEFT) {
        top_comps.handle_press(0, 0, false);
        if (!processor_gui.is_pressed(window_state->mouseX, window_state->mouseY) && mouse_down) {
            processor_gui.set_selected(false);
            SDL_StopTextInput();
        }
        mouse_down = false;
    }
}

void GameState::handle_down(const SDL_Keycode key, const Uint8 mouse) {
    if (key == SDLK_ESCAPE) {
        next_state.action = StateStatus::POP;
    } else if (mouse == SDL_BUTTON_LEFT) {
        if (processor_gui.is_pressed(window_state->mouseX, window_state->mouseY)) {
            SDL_StartTextInput();
            processor_gui.set_selected(true);
            processor.invalidate();
            problem.reset();
        } else {
            mouse_down = true;
            processor_gui.set_selected(false);
            SDL_StopTextInput();
        }
        top_comps.handle_press(0, 0, true);
    } else {
        processor_gui.handle_keypress(key);
    }
}

void GameState::handle_textinput(const SDL_TextInputEvent &e) {
    if (e.text[0] == '\0' || e.text[1] != '\0') {
        LOG_DEBUG("Invalid input received");
        return;
    }
    char c = e.text[0];

    processor_gui.input_char(c);
}

void GameState::handle_focus_change(bool focus) {
    if (!focus) {
        processor_gui.set_selected(false);
        mouse_down = false;
        SDL_StopTextInput();
    }
}

