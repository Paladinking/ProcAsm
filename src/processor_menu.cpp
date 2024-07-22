#include "processor_menu.h"
#include "config.h"
#include "engine/style.h"
#include <algorithm>
#include <bitset>

constexpr int MENU_WIDTH = 632;
constexpr int MENU_HEIGHT = 900;
constexpr int BASE_X = WIDTH / 2 - MENU_WIDTH;
constexpr int BASE_Y = HEIGHT / 2 - MENU_HEIGHT / 2;
constexpr int CHOICE_BOX_HEIGHT = 184;
constexpr int FIELDS_BASE = BASE_Y + CHOICE_BOX_HEIGHT;

int bit_count(feature_t map) {
#if __has_cpp_attribute(cpp_lib_bitops)
    return std::popcount(map);
#else
    return std::bitset<64>(map).count();
#endif
}

class InstructionInfo : public Menu {
public:
    InstructionInfo(State *parent, std::string name, InstructionSlotType slot)
        : parent{parent}, name{std::move(name)}, slot{slot} {}

    void init(WindowState *window_state) override {
        Menu::init(window_state);
        comps.set_window_state(window_state);
        
        comps.add(
            Box(BASE_X + MENU_WIDTH / 2, BASE_Y, MENU_WIDTH, MENU_HEIGHT, 6));
    }

    void render() override {
        parent->render();

        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0x5f);
        SDL_Rect r = {0, 0, WIDTH, HEIGHT};
        SDL_RenderFillRect(gRenderer, &r);

        Menu::render();
    }

private:
    State *parent;

    std::string name;
    InstructionSlotType slot;
};

class AddInstruction : public Menu {
public:
    AddInstruction(State *parent, ProcessorTemplate &proc_template)
        : proc_template{proc_template}, parent{parent} {}

    void init(WindowState *window_state) override {
        Menu::init(window_state);
        comps.set_window_state(window_state);

        comps.add(
            Box(BASE_X + MENU_WIDTH / 2, BASE_Y, MENU_WIDTH, MENU_HEIGHT, 6));

        comps.add(TextBox(BASE_X + MENU_WIDTH / 2, BASE_Y + 20, MENU_WIDTH, 20,
                          "Add instruction", *window_state));

        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 30, BASE_Y + 45, 80, 50,
                          "Name", *window_state));
        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 110, BASE_Y + 45, 315, 50,
                          "Mnemonic", *window_state));
        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 405, BASE_Y + 45, 60, 50,
                          "Area", *window_state));

        std::array<bool, INSTRUCTION_SLOT_COUNT> found{};
        found.fill(false);
        for (auto &i : proc_template.instruction_set) {
            found[static_cast<std::size_t>(i.second)] = true;
        }

        using Slot = std::pair<const std::string *, InstructionSlotType>;

        std::array<Slot, INSTRUCTION_SLOT_COUNT> to_show;
        std::size_t count = 0;
        for (auto &i : ALL_INSTRUCTIONS) {
            if (!found[static_cast<std::size_t>(i.second)]) {
                to_show[count].first = &i.first;
                to_show[count].second = i.second;
                ++count;
            }
        }

        std::stable_sort(
            to_show.begin(), to_show.begin() + count, [this](Slot a, Slot b) {
                feature_t required_a = required_features(a.second);
                feature_t required_b = required_features(b.second);

                return bit_count(~proc_template.features & required_a) <
                       bit_count(~proc_template.features & required_b);
            });

        void (*exit)(AddInstruction *, InstructionSlotType) =
            [](AddInstruction *self, InstructionSlotType slot) {
                if (self->next_res.will_leave()) {
                    return;
                }
                for (auto &v : ALL_INSTRUCTIONS) {
                    if (v.second == slot) {
                        self->proc_template.instruction_set.insert(v);
                        self->menu_exit();
                        return;
                    }
                }
            };

        void (*info)(AddInstruction *, std::string, InstructionSlotType) =
            [](AddInstruction *self, std::string name,
               InstructionSlotType slot) {
                self->next_res.action = StateStatus::PUSH;
                self->next_res.new_state = new InstructionInfo(self->parent, name, slot);
            };

        std::size_t ix = 0;
        for (auto it = to_show.begin(); it != to_show.begin() + count; ++it) {
            auto mnemonic =
                *it->first + " " +
                instruction_mnemonic(it->second, proc_template.features);
            auto area =
                std::to_string(area_cost(it->second, proc_template.features));
            const int y = BASE_Y + 90 + 60 * ix;
            SDL_Color color = {UI_BORDER_COLOR};

            feature_t required = required_features(it->second);
            bool available = (proc_template.features & required) == required;
            if (!available) {
                color.r -= 0x9f;
                color.g -= 0x9f;
                color.b -= 0x9f;
            }
            comps
                .add(Box(BASE_X + MENU_WIDTH / 2 + 30, y, MENU_WIDTH - 60, 50,
                         2))
                ->set_border_color(color.r, color.g, color.b, color.a);
            comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 30, y, 80, 50,
                              *it->first, *window_state));
            comps.add(Box(BASE_X + MENU_WIDTH / 2 + 30, y, 80, 50, 2))
                ->set_border_color(color.r, color.g, color.b, color.a);
            auto mem = comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 125, y, 200,
                                         50, mnemonic, *window_state));
            mem->set_align(Alignment::LEFT);
            mem->set_text_color(color.r, color.g, color.b, color.a);

            comps.add(Box(BASE_X + MENU_WIDTH / 2 + 405, y + 2, 2, 46, 1))
                ->set_border_color(color.r, color.g, color.b, color.a);
            comps.add(Box(BASE_X + MENU_WIDTH / 2 + 463, y + 2, 2, 46, 1))
                ->set_border_color(color.r, color.g, color.b, color.a);
            comps
                .add(TextBox(BASE_X + MENU_WIDTH / 2 + 405, y, 60, 50, area,
                             *window_state))
                ->set_text_color(color.r, color.g, color.b, color.a);
            select_buttons.push_back(
                comps.add(Button(BASE_X + MENU_WIDTH / 2 + 485, y + 10, 30, 30,
                                 "+", *window_state),
                          exit, this, it->second));
            select_buttons.back()->enable_button(available);
            comps.add(Button(BASE_X + MENU_WIDTH / 2 + 545, y + 10, 30, 30, "i",
                             *window_state), info, this, *it->first, it->second);
            if (!available) {
                feature_t missing = required & ~proc_template.features;
                std::string s = ProcessorFeature::string(missing);
                comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 30, y,
                                  MENU_WIDTH - 60, 50, "Requires " + s,
                                  *window_state));
            }
            ++ix;
        }
    }

    void render() override {
        parent->render();

        SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0x5f);
        SDL_Rect r = {0, 0, WIDTH, HEIGHT};
        SDL_RenderFillRect(gRenderer, &r);

        Menu::render();
    }

private:
    ProcessorTemplate &proc_template;

    std::vector<Component<Button>> select_buttons{};

    State *parent;
};

ProcessorMenu::ProcessorMenu(State *parent,
                             std::vector<ProcessorTemplate> &templates,
                             std::size_t selected_ix)
    : parent{parent}, templates{templates}, selected_ix{selected_ix} {}

void ProcessorMenu::layout_instructions() {
    Components &c = instr_comps;
    c.clear();
    c.set_window_state(window_state);

    void (*remove_instr)(ProcessorMenu *, std::size_t) =
        [](ProcessorMenu *self, std::size_t ix) { self->instr_to_remove = ix; };

    void (*info)(ProcessorMenu*, std::string, InstructionSlotType) = [](ProcessorMenu* self, std::string name, InstructionSlotType slot) {
        self->next_res.action = StateStatus::PUSH;
        self->next_res.new_state = new InstructionInfo(self, name, slot);
    };

    std::size_t ix = selected_ix;

    std::size_t i = 0;
    for (const auto &instr : templates[ix].instruction_set) {
        auto mnemonic =
            instr.first + " " +
            instruction_mnemonic(instr.second, templates[ix].features);
        auto area =
            std::to_string(area_cost(instr.second, templates[ix].features));
        const int y = BASE_Y + 90 + 60 * i;
        c.add(Box(BASE_X + MENU_WIDTH + 30, y, MENU_WIDTH - 60, 50, 2));
        c.add(TextBox(BASE_X + MENU_WIDTH + 30, y, 80, 50, instr.first,
                      *window_state));
        c.add(Box(BASE_X + MENU_WIDTH + 30, y, 80, 50, 2));
        c.add(TextBox(BASE_X + MENU_WIDTH + 125, y, 200, 50, mnemonic,
                      *window_state))
            ->set_align(Alignment::LEFT);
        c.add(Box(BASE_X + MENU_WIDTH + 405, y, 60, 50, 2));
        c.add(
            TextBox(BASE_X + MENU_WIDTH + 405, y, 60, 50, area, *window_state));
        c.add(Button(BASE_X + MENU_WIDTH + 485, y + 10, 30, 30, "x",
                     *window_state),
              remove_instr, this, i);
        c.add(Button(BASE_X + MENU_WIDTH + 545, y + 10, 30, 30, "i",
                     *window_state), info, this, instr.first, instr.second);
        ++i;
    }

    instr_slots->set_text("Instruction slots: " + std::to_string(i) + "/" +
                          std::to_string(templates[ix].instruction_slots));
    if (i >= templates[ix].instruction_slots) {
        instr_slots->set_text_color(0xf0, 0x0, 0x0, 0xff);
        add_instruction->enable_button(false);
    } else {
        instr_slots->set_text_color(TEXT_COLOR);
        add_instruction->enable_button(true);
    }
}

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
        auto add_flag = [&ix, &flags, &first, &self](std::string name,
                                                     flag_t mask) {
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

        self->features->set_text(
            ProcessorFeature::string(self->templates[ix].features));

        self->layout_instructions();
    };

    for (std::size_t i = 0; i < templates.size(); ++i) {
        const std::string &name = templates[i].name;
        auto btn = comps.add(Button(BASE_X + 70 + 166 * i,
                                    BASE_Y + CHOICE_BOX_HEIGHT / 2 - 50, 160,
                                    100, name, *window_state),
                             proc_btn, this, i);
        if (i == selected_ix) {
            btn->enable_button(false);
        }
        proc_buttons.push_back(btn);
    }

    // Line
    comps.add(Box(BASE_X, FIELDS_BASE, MENU_WIDTH, 6, 3));

    name = comps.add(
        TextBox(BASE_X + 20, FIELDS_BASE + 30, 10, 5, "", *window_state));
    name->set_align(Alignment::LEFT);
    auto regs = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 60, 10, 5,
                                  "Registers: ", *window_state));
    regs->set_align(Alignment::LEFT);

    for (int i = 0; i < 4; ++i) {
        register_types[i] = comps.add(TextBox(
            BASE_X + 30, FIELDS_BASE + 80 + 20 * i, 10, 5, "", *window_state));
        register_types[i]->set_align(Alignment::LEFT);
    }

    auto flags_title = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 170, 10, 5,
                                         "Flags: ", *window_state));
    flags_title->set_align(Alignment::LEFT);

    flags = comps.add(
        TextBox(BASE_X + 30, FIELDS_BASE + 190, 10, 5, "", *window_state));
    flags->set_align(Alignment::LEFT);

    auto features_title = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 220, 10,
                                            5, "Features: ", *window_state));
    features_title->set_align(Alignment::LEFT);

    features = comps.add(
        TextBox(BASE_X + 30, FIELDS_BASE + 240, 10, 5, "", *window_state));
    features->set_align(Alignment::LEFT);

    auto inst_title =
        comps.add(TextBox(BASE_X + MENU_WIDTH + 20, BASE_Y + 30, 10, 5,
                          "Instructions: ", *window_state));
    inst_title->set_align(Alignment::LEFT);

    comps.add(TextBox(BASE_X + MENU_WIDTH + 30, BASE_Y + 45, 80, 50, "Name",
                      *window_state));
    comps.add(TextBox(BASE_X + MENU_WIDTH + 110, BASE_Y + 45, 315, 50,
                      "Mnemonic", *window_state));
    comps.add(TextBox(BASE_X + MENU_WIDTH + 405, BASE_Y + 45, 60, 50, "Area",
                      *window_state));
    // comps.add(TextBox(BASE_X + MENU_WIDTH + 525, BASE_Y + 45, 60, 50, "Info",
    //                   *window_state));

    void (*add_instr)(ProcessorMenu *) = [](ProcessorMenu *self) {
        self->comps.enable_hover(false);
        self->instr_comps.enable_hover(false);
        self->next_res.action = StateStatus::PUSH;
        self->next_res.new_state =
            new AddInstruction(self, self->templates[self->selected_ix]);
    };

    add_instruction =
        comps.add(Button(BASE_X + MENU_WIDTH + 30, BASE_Y + MENU_HEIGHT - 80,
                         80, 60, "+", *window_state),
                  add_instr, this);
    instr_slots =
        comps.add(TextBox(BASE_X + MENU_WIDTH + 120, BASE_Y + MENU_HEIGHT - 80,
                          80, 60, "", *window_state));
    instr_slots->set_align(Alignment::LEFT);

    proc_btn(this, selected_ix);
}

void ProcessorMenu::resume() {
    layout_instructions();
    comps.enable_hover(true);
    instr_comps.enable_hover(true);
}

void ProcessorMenu::render() {
    if (instr_to_remove < templates[selected_ix].instruction_set.size()) {
        auto it = templates[selected_ix].instruction_set.begin();
        std::advance(it, instr_to_remove);
        templates[selected_ix].instruction_set.erase(it);
        instr_to_remove = std::numeric_limits<std::size_t>::max();

        layout_instructions();
    }
    parent->render();
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0x5f);
    SDL_Rect r = {0, 0, WIDTH, HEIGHT};
    SDL_RenderFillRect(gRenderer, &r);
    Menu::render();
    instr_comps.render(0, 0);
}

void ProcessorMenu::handle_up(SDL_Keycode key, Uint8 mouse) {
    Menu::handle_up(key, mouse);
    if (mouse == SDL_BUTTON_LEFT) {
        instr_comps.handle_press(0, 0, false);
    }
}

void ProcessorMenu::handle_down(SDL_Keycode key, Uint8 mouse) {
    Menu::handle_down(key, mouse);
    if (mouse == SDL_BUTTON_LEFT) {
        instr_comps.handle_press(0, 0, true);
    }
}
