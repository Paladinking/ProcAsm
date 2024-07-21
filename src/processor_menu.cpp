#include "processor_menu.h"
#include "config.h"

ProcessorMenu::ProcessorMenu(State *parent,
                             std::vector<ProcessorTemplate> &templates,
                             std::size_t selected_ix)
    : parent{parent}, templates{templates}, selected_ix{selected_ix} {}

void ProcessorMenu::init(WindowState *window_state) {
    Menu::init(window_state);
    comps.set_window_state(window_state);

    comps.add(Box(WIDTH / 2 - 316, HEIGHT / 2 - 450, 632, 900, 6));

    void (*proc_btn)(ProcessorMenu *, std::size_t) = [](ProcessorMenu *self,
                                                        std::size_t ix) {
        self->proc_buttons[self->selected_ix]->enable_button(true);
        self->selected_ix = ix;
        self->proc_buttons[ix]->enable_button(false);

        self->name->set_text("Name: " + self->templates[ix].name);

        std::size_t reg_ix = 0;

        auto reg_text = [&ix, &self, &reg_ix](DataSize size, std::string post) {
            uint32_t count = 0;
            for (const auto &name : self->templates[ix].register_names) {
                count += name.second == size;
            }
            if (count == 0) {
                return;
            }
            post += (count > 1 ? " registers" : " register");
            std::string s = std::to_string(count) + post;
            self->register_types[reg_ix++]->set_text(std::move(s));
        };

        reg_text(DataSize::BYTE, " general purpose 8 bit");
        reg_text(DataSize::WORD, " general purpose 16 bit");
        reg_text(DataSize::DWORD, " general purpose 32 bit");
        reg_text(DataSize::QWORD, " general purpose 64 bit");

        if (reg_ix == 0) {
            self->register_types[reg_ix++]->set_text("None");
        }

        for (; reg_ix < self->register_types.size(); ++reg_ix) {
            self->register_types[reg_ix]->set_text("");
        }
        std::string flags = "";
        bool first = true;
        auto add_flag = [&ix, &flags, &first, &self](std::string name, flag_t mask) {
            if (self->templates[ix].supported_flags & mask) {
                if (!first) {
                    flags += ", " + name;
                } else {
                    flags += name;
                    first = false;
                }
            }
        };
        add_flag("Zero", FLAG_ZERO_MASK);
        add_flag("Carry", FLAG_CARRY_MASK);
        self->flags->set_text(flags);
    };

    for (std::size_t i = 0; i < templates.size(); ++i) {
        const std::string &name = templates[i].name;
        auto btn =
            comps.add(Button(WIDTH / 2 - 316 + 70 + 166 * i, HEIGHT / 2 - 406,
                             160, 100, name, *window_state),
                      proc_btn, this, i);
        if (i == selected_ix) {
            btn->enable_button(false);
        }
        proc_buttons.push_back(btn);
    }

    // Line
    comps.add(Box(WIDTH / 2 - 316, HEIGHT / 2 - 266, 632, 6, 3));

    name = comps.add(TextBox(WIDTH / 2 - 316 + 20, HEIGHT / 2 - 236, 10, 5, "",
                             *window_state));
    name->set_align(Alignment::LEFT);
    auto regs = comps.add(TextBox(WIDTH / 2 - 316 + 20, HEIGHT / 2 - 196, 10, 5,
                                  "Registers: ", *window_state));
    regs->set_align(Alignment::LEFT);

    for (int i = 0; i < 4; ++i) {
        register_types[i] =
            comps.add(TextBox(WIDTH / 2 - 316 + 30, HEIGHT / 2 - 176 + 20 * i,
                              10, 5, "", *window_state));
        register_types[i]->set_align(Alignment::LEFT);
    }

    auto flags_title = comps.add(TextBox(WIDTH / 2 - 316 + 20, HEIGHT / 2 - 86,
        10, 5, "Flags: ", *window_state));
    flags_title->set_align(Alignment::LEFT);

    flags = comps.add(TextBox(WIDTH / 2 - 316 + 30, HEIGHT / 2 - 66, 10, 5, "", *window_state));
    flags->set_align(Alignment::LEFT);

    proc_btn(this, selected_ix);
}

void ProcessorMenu::render() {
    parent->render();
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0x5f);
    SDL_Rect r = {0, 0, WIDTH, HEIGHT};
    SDL_RenderFillRect(gRenderer, &r);
    Menu::render();
}
