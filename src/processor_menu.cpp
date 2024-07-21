#include "processor_menu.h"
#include "config.h"

constexpr int MENU_WIDTH = 632;
constexpr int MENU_HEIGHT = 900;
constexpr int BASE_X = WIDTH / 2 - MENU_WIDTH;
constexpr int BASE_Y = HEIGHT / 2 - MENU_HEIGHT / 2;
constexpr int CHOICE_BOX_HEIGHT = 184;
constexpr int FIELDS_BASE = BASE_Y + CHOICE_BOX_HEIGHT;

ProcessorMenu::ProcessorMenu(State *parent,
                             std::vector<ProcessorTemplate> &templates,
                             std::size_t selected_ix)
    : parent{parent}, templates{templates}, selected_ix{selected_ix} {}

void ProcessorMenu::init(WindowState *window_state) {
    Menu::init(window_state);
    comps.set_window_state(window_state);

    comps.add(Box(BASE_X, BASE_Y, MENU_WIDTH, MENU_HEIGHT, 6));
    comps.add(Box(BASE_X + MENU_WIDTH, BASE_Y, MENU_WIDTH, MENU_HEIGHT, 6));

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
        std::string features = "";
        first = true;
        auto add_feature = [&ix, &features, &first, &self](std::string name, feature_t mask) {
            if (self->templates[ix].features & mask) {
                features += first ? name : ", " + name;
                first = false;
            }
        };

        add_feature("ALU", ProcessorFeature::ALU);
        add_feature("Register file", ProcessorFeature::REGISTER_FILE);
        self->features->set_text(features);

        WindowState* window_state = self->window_state;
        Components& comps = self->instr_comps;
        comps.clear();
        comps.set_window_state(window_state);

        std::size_t i = 0;
        for (const auto& instr: self->templates[ix].instruction_set) {
            auto mnemonic = instr.first + " " + instruction_mnemonic(instr.second, self->templates[ix].features);
            auto area = std::to_string(area_cost(instr.second, self->templates[ix].features));
            const int y = BASE_Y + 90 + 60 * i;
            comps.add(Box(BASE_X + MENU_WIDTH + 30, y, MENU_WIDTH - 60, 50, 2));
            comps.add(TextBox(BASE_X + MENU_WIDTH + 30, y, 80, 50, instr.first, *window_state));
            comps.add(Box(BASE_X + MENU_WIDTH + 30, y, 80, 50, 2));
            comps.add(TextBox(BASE_X + MENU_WIDTH + 125, y, 200, 50, mnemonic, *window_state))->set_align(Alignment::LEFT);
            comps.add(Box(BASE_X + MENU_WIDTH + 425, y, 100, 50, 2));
            comps.add(TextBox(BASE_X + MENU_WIDTH + 425, y, 100, 50, area, *window_state));
            ++i;
        }

        self->instr_slots->set_text("Instruction slots: " + std::to_string(i) + "/" + std::to_string(self->templates[ix].instruction_slots));
        if (i >= self->templates[ix].instruction_slots) {
            self->instr_slots->set_text_color(0xf0, 0x0, 0x0, 0xff);
            self->add_instruction->enable_button(false);
        } else {
            self->instr_slots->set_text_color(TEXT_COLOR);
            self->add_instruction->enable_button(true);
        }
    };

    for (std::size_t i = 0; i < templates.size(); ++i) {
        const std::string &name = templates[i].name;
        auto btn =
            comps.add(Button(BASE_X + 70 + 166 * i, BASE_Y + CHOICE_BOX_HEIGHT / 2 - 50,
                             160, 100, name, *window_state),
                      proc_btn, this, i);
        if (i == selected_ix) {
            btn->enable_button(false);
        }
        proc_buttons.push_back(btn);


    }

    // Line
    comps.add(Box(BASE_X, FIELDS_BASE, MENU_WIDTH, 6, 3));

    name = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 30, 10, 5, "",
                             *window_state));
    name->set_align(Alignment::LEFT);
    auto regs = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 60, 10, 5,
                                  "Registers: ", *window_state));
    regs->set_align(Alignment::LEFT);

    for (int i = 0; i < 4; ++i) {
        register_types[i] =
            comps.add(TextBox(BASE_X + 30, FIELDS_BASE + 80 + 20 * i,
                              10, 5, "", *window_state));
        register_types[i]->set_align(Alignment::LEFT);
    }

    auto flags_title = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 170,
        10, 5, "Flags: ", *window_state));
    flags_title->set_align(Alignment::LEFT);

    flags = comps.add(TextBox(BASE_X + 30, FIELDS_BASE + 190, 10, 5, "", *window_state));
    flags->set_align(Alignment::LEFT);

    auto features_title = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 220, 10, 5, "Features: ", *window_state));
    features_title->set_align(Alignment::LEFT);

    features = comps.add(TextBox(BASE_X + 30, FIELDS_BASE + 240, 10, 5, "", *window_state));
    features->set_align(Alignment::LEFT);

    auto inst_title = comps.add(TextBox(BASE_X + MENU_WIDTH + 20, BASE_Y + 30, 10, 5, "Instructions: ", *window_state));
    inst_title->set_align(Alignment::LEFT);

    comps.add(TextBox(BASE_X + MENU_WIDTH + 30, BASE_Y + 45, 80, 50, "Name", *window_state));
    comps.add(TextBox(BASE_X + MENU_WIDTH + 110, BASE_Y + 45, 315, 50, "Mnemonic", *window_state));
    comps.add(TextBox(BASE_X + MENU_WIDTH + 425, BASE_Y + 45, 100, 50, "Area", *window_state));
    comps.add(TextBox(BASE_X + MENU_WIDTH + 525, BASE_Y + 45, 60, 50, "Details", *window_state));

    add_instruction = comps.add(Button(BASE_X + MENU_WIDTH + 30, BASE_Y + MENU_HEIGHT - 80, 80, 60, "+", *window_state));
    instr_slots = comps.add(TextBox(BASE_X + MENU_WIDTH + 120, BASE_Y + MENU_HEIGHT - 80, 80, 60, "", *window_state));
    instr_slots->set_align(Alignment::LEFT);

    proc_btn(this, selected_ix);
}

void ProcessorMenu::render() {
    parent->render();
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0x5f);
    SDL_Rect r = {0, 0, WIDTH, HEIGHT};
    SDL_RenderFillRect(gRenderer, &r);
    Menu::render();
    instr_comps.render(0, 0);
}
