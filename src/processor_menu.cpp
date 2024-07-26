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

class InstructionInfo : public OverlayMenu {
public:
    InstructionInfo(State *parent, std::string name, InstructionSlotType slot,
                    feature_t features)
        : OverlayMenu(parent), name{std::move(name)}, slot{slot},
          features{features} {}

    void init(WindowState *window_state) override {
        OverlayMenu::init(window_state);
        int base_y = BASE_Y + MENU_HEIGHT / 2 - MENU_HEIGHT / 4;

        comps.add(Box(BASE_X + MENU_WIDTH / 2, base_y, MENU_WIDTH,
                      MENU_HEIGHT / 2, 6));

        comps.add(TextBox(BASE_X + MENU_WIDTH / 2, base_y + 20, MENU_WIDTH, 20,
                          name, *window_state));

        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 30, base_y + 45, 80, 50,
                          "Name", *window_state));
        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 110, base_y + 45, 315, 50,
                          "Mnemonic", *window_state));
        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 405, base_y + 45, 60, 50,
                          "Area", *window_state));

        auto mnemonic = name + " " + instruction_mnemonic(slot, features);
        auto area = std::to_string(instruction_area(slot, features));
        const int y = base_y + 90;
        comps.add(Box(BASE_X + MENU_WIDTH / 2 + 30, y, MENU_WIDTH - 60, 50, 2));
        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 30, y, 80, 50, name,
                          *window_state));
        comps.add(Box(BASE_X + MENU_WIDTH / 2 + 30, y, 80, 50, 2));
        comps
            .add(TextBox(BASE_X + MENU_WIDTH / 2 + 125, y, 400, 50, mnemonic,
                         *window_state))
            ->set_align(Alignment::LEFT);
        comps.add(Box(BASE_X + MENU_WIDTH / 2 + 405, y, 60, 50, 2));
        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 405, y, 60, 50, area,
                          *window_state));

        auto req_features =
            ProcessorFeature::string(instruction_features(slot));
        comps
            .add(TextBox(BASE_X + MENU_WIDTH / 2 + 20, base_y + 150,
                         MENU_WIDTH - 40, 20,
                         "Required features: " + req_features, *window_state))
            ->set_align(Alignment::LEFT);
        comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 20, base_y + 175,
                          MENU_WIDTH - 40, 20, "Description", *window_state));

        comps
            .add(TextBox(BASE_X + MENU_WIDTH / 2 + 20, base_y + 205,
                         MENU_WIDTH - 40, 20, instruction_usage(slot, name),
                         *window_state))
            ->set_align(Alignment::LEFT);
        comps
            .add(TextBox(BASE_X + MENU_WIDTH / 2 + 20, base_y + 245,
                         MENU_WIDTH - 40, 20, instruction_description(slot),
                         *window_state))
            ->set_align(Alignment::LEFT);
    }

private:
    feature_t features;

    std::string name;
    InstructionSlotType slot;
};

class AddInstruction : public OverlayMenu {
public:
    AddInstruction(State *parent, ProcessorTemplate &proc_template)
        : OverlayMenu(parent), proc_template{proc_template} {}

    void init(WindowState *window_state) override {
        OverlayMenu::init(window_state);

        layout();
    }

private:
    void layout() {
        comps.clear();

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
                feature_t required_a = instruction_features(a.second);
                feature_t required_b = instruction_features(b.second);

                return bit_count(~proc_template.features & required_a) <
                       bit_count(~proc_template.features & required_b);
            });

        void (*add_instr)(AddInstruction *, InstructionSlotType) =
            [](AddInstruction *self, InstructionSlotType slot) {
                if (self->next_res.will_leave()) {
                    return;
                }
                for (auto &v : ALL_INSTRUCTIONS) {
                    if (v.second == slot) {
                        self->proc_template.instruction_set.insert(v);
                        self->layout();
                        return;
                    }
                }
            };

        void (*info)(AddInstruction *, std::string, InstructionSlotType) =
            [](AddInstruction *self, std::string name,
               InstructionSlotType slot) {
                self->next_res.action = StateStatus::PUSH;
                self->next_res.new_state = new InstructionInfo(
                    self, name, slot, self->proc_template.features);
            };

        std::size_t ix = 0;
        for (auto it = to_show.begin(); it != to_show.begin() + count; ++it) {
            auto mnemonic =
                *it->first + " " +
                instruction_mnemonic(it->second, proc_template.features);
            auto area = std::to_string(
                instruction_area(it->second, proc_template.features));
            const int y = BASE_Y + 90 + 60 * ix;
            SDL_Color color = {UI_BORDER_COLOR};

            feature_t required = instruction_features(it->second);
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
            auto mem = comps.add(TextBox(BASE_X + MENU_WIDTH / 2 + 125, y, 400,
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
                          add_instr, this, it->second));
            select_buttons.back()->enable_button(available);
            comps.add(Button(BASE_X + MENU_WIDTH / 2 + 545, y + 10, 30, 30, "i",
                             *window_state),
                      info, this, *it->first, it->second);
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

    ProcessorTemplate &proc_template;

    std::vector<Component<Button>> select_buttons{};
};

class AddFeature : public OverlayMenu {
public:
    AddFeature(State *parent, ProcessorTemplate &proc_template)
        : OverlayMenu(parent), proc_template{proc_template} {}

    void init(WindowState *window_state) override {
        OverlayMenu::init(window_state);
        layout();
    }

private:
    void layout() {
        comps.clear();
        comps.add(
            Box(BASE_X + MENU_WIDTH / 2, BASE_Y, MENU_WIDTH, MENU_HEIGHT, 5));

        comps.add(TextBox(BASE_X + MENU_WIDTH / 2, BASE_Y + 20, MENU_WIDTH, 20,
                          "Add feature", *window_state));
        comps.add(TextBox(BASE_X + MENU_WIDTH / 2,
                          BASE_Y + MENU_HEIGHT / 2 + 20, MENU_WIDTH, 20,
                          "Remove feature", *window_state));

        std::size_t ix = 0;
        void (*remove)(AddFeature *, feature_t) = [](AddFeature *self,
                                                     feature_t feature) {
            self->proc_template.features &= ~feature;
            self->layout();
        };

        void (*add)(AddFeature *, feature_t) = [](AddFeature *self,
                                                  feature_t feature) {
            self->proc_template.features |= feature;
            self->layout();
        };

        for (std::size_t i = 0; i < ProcessorFeature::COUNT; ++i) {
            feature_t f = 1 << i;
            if ((proc_template.features & f) &&
                !(ProcessorFeature::dependents(f) & proc_template.features)) {
                comps.add(Button(BASE_X + MENU_WIDTH / 2 + 20,
                                 BASE_Y + 40 + MENU_HEIGHT / 2 + 30 * ix, 25,
                                 25, "-", *window_state),
                          remove, this, f);
                comps
                    .add(TextBox(BASE_X + MENU_WIDTH / 2 + 50,
                                 BASE_Y + 40 + MENU_HEIGHT / 2 + 30 * ix,
                                 MENU_WIDTH - 60, 25,
                                 ProcessorFeature::string(f), *window_state))
                    ->set_align(Alignment::LEFT);
                ++ix;
            }
        }

        ix = 0;
        for (std::size_t i = 0; i < ProcessorFeature::COUNT; ++i) {
            feature_t f = 1 << i;
            if (ProcessorFeature::ALL & f) {
                if ((proc_template.features & f) ||
                    (ProcessorFeature::dependencies(f) &
                     ~proc_template.features)) {
                    continue;
                }
                comps.add(Button(BASE_X + MENU_WIDTH / 2 + 20,
                                 BASE_Y + 40 + 30 * ix, 25, 25, "+",
                                 *window_state),
                          add, this, f);
                comps
                    .add(TextBox(BASE_X + MENU_WIDTH / 2 + 50,
                                 BASE_Y + 40 + 30 * ix, MENU_WIDTH - 60, 25,
                                 ProcessorFeature::string(f), *window_state))
                    ->set_align(Alignment::LEFT);
                ++ix;
            }
        }
    }

    State *parent;

    ProcessorTemplate &proc_template;
};

class AddRegister : public OverlayMenu {
public:
    AddRegister(State *parent, ProcessorTemplate &proc_template)
        : OverlayMenu(parent), proc_template{proc_template} {}

    void init(WindowState *window_state) override {
        OverlayMenu::init(window_state);

        layout();
    }

private:
    void layout() {
        comps.clear();

        comps.add(
            Box(BASE_X + MENU_WIDTH / 2, BASE_Y, MENU_WIDTH, MENU_HEIGHT, 5));

        comps.add(TextBox(BASE_X + MENU_WIDTH / 2, BASE_Y + 20, MENU_WIDTH, 20,
                          "Modify registers", *window_state));

        int y = BASE_Y + 40;

        int w = (MENU_WIDTH - 60 - 5 * 4) / 4;

        auto has_name = [](const std::string &name, RegisterNames &names) {
            for (auto &r : names) {
                if (r.first == name) {
                    return true;
                }
            }
            return false;
        };

        auto set_name = [&has_name](const std::string &prefix,
                                    RegisterNames &names, std::string &name) {
            std::size_t ix = 0;
            do {
                name = prefix + std::to_string(ix);
                if (!has_name(name, names)) {
                    break;
                }
                ++ix;
            } while (true);
        };

        typedef void (*Callback_t)(AddRegister *, DataSize);

        int x, new_y;

        auto layout_regs = [this, &y, w, &x,
                            &new_y](DataSize size, std::string name,
                                    Callback_t add_reg, Callback_t remove_reg,
                                    RegisterNames &names, uint64_t cost) {
            int ry = y + 25;
            comps.add(TextBox(x, y, w, 20, std::move(name), *window_state));
            std::size_t count = 0;
            for (auto &reg : names) {
                if (reg.second == size) {
                    ++count;
                    comps
                        .add(TextBox(x + 5, ry + 5, w - 10, 20, reg.first,
                                     *window_state))
                        ->set_align(Alignment::LEFT);
                    comps.add(Box(x, ry, w, 30, 2));
                    ry += 40;
                }
            }
            comps.add(Button(x, ry, w / 2 - 5, 30, "+", *window_state), add_reg,
                      this, size);
            auto remove = comps.add(
                Button(x + w / 2 + 5, ry, w / 2 - 5, 30, "-", *window_state),
                remove_reg, this, size);
            if (count == 0) {
                remove->enable_button(false);
            }
            ry += 40;
            comps.add(TextBox(x, ry, w, 20,
                              "Area cost: " + std::to_string(cost),
                              *window_state));
            ry += 25;
            x += w + 5;
            new_y = ry > new_y ? ry : new_y;
        };

        if (proc_template.features & ProcessorFeature::REGISTER_FILE) {
            set_name("R", proc_template.genreg_names, genreg_name);
            comps
                .add(TextBox(BASE_X + MENU_WIDTH / 2 + 30, y, MENU_WIDTH - 60,
                             20, "General purpose registers:", *window_state))
                ->set_align(Alignment::LEFT);
            y += 30;
            new_y = y;
            x = BASE_X + MENU_WIDTH / 2 + 30;

            Callback_t add_reg = [](AddRegister *self, DataSize size) {
                self->proc_template.genreg_names.push_back(
                    {self->genreg_name, size});
                self->layout();
            };
            Callback_t remove_reg = [](AddRegister *self, DataSize size) {
                auto &names = self->proc_template.genreg_names;
                for (int64_t i = names.size() - 1; i >= 0; --i) {
                    if (names[i].second == size) {
                        names.erase(names.begin() + i);
                        self->layout();
                        return;
                    }
                }
            };

            layout_regs(DataSize::BYTE, "8-bit registers", add_reg, remove_reg,
                        proc_template.genreg_names, 10);
            if (proc_template.features & ProcessorFeature::REG_16) {
                layout_regs(DataSize::WORD, "16-bit registers", add_reg,
                            remove_reg, proc_template.genreg_names, 10);
            }
            if (proc_template.features & ProcessorFeature::REG_32) {
                layout_regs(DataSize::DWORD, "32-bit registers", add_reg,
                            remove_reg, proc_template.genreg_names, 10);
            }
            if (proc_template.features & ProcessorFeature::REG_64) {
                layout_regs(DataSize::QWORD, "64-bit registers", add_reg,
                            remove_reg, proc_template.genreg_names, 10);
            }
            y = new_y + 15;
        }
        if (proc_template.features & ProcessorFeature::REG_FLOAT) {
            set_name("F", proc_template.floatreg_names, floatreg_name);
            comps
                .add(TextBox(BASE_X + MENU_WIDTH / 2 + 30, y, MENU_WIDTH - 60,
                             20, "Floating point registers:", *window_state))
                ->set_align(Alignment::LEFT);

            Callback_t add_reg = [](AddRegister *self, DataSize size) {
                self->proc_template.floatreg_names.push_back(
                    {self->floatreg_name, size});
                self->layout();
            };
            Callback_t remove_reg = [](AddRegister *self, DataSize size) {
                auto &names = self->proc_template.floatreg_names;
                for (int64_t i = names.size() - 1; i >= 0; --i) {
                    if (names[i].second == size) {
                        names.erase(names.begin() + i);
                        self->layout();
                        return;
                    }
                }
            };

            y += 30;
            new_y = y;
            x = BASE_X + MENU_WIDTH / 2 + 30;

            layout_regs(DataSize::DWORD, "32-bit registers", add_reg,
                        remove_reg, proc_template.floatreg_names, 12);
            if (proc_template.features & ProcessorFeature::REG_DOUBLE) {
                layout_regs(DataSize::QWORD, "64-bit registers", add_reg,
                            remove_reg, proc_template.floatreg_names, 12);
            }
            y = new_y + 15;
        }
    }

    std::string genreg_name{};
    std::string floatreg_name{};

    ProcessorTemplate &proc_template;
};

class InfoPort : public OverlayMenu {
public:
    InfoPort(State *parent, PortTemplate &port, bool input, feature_t features)
        : OverlayMenu(parent), port{port}, input{input}, features{features} {}

    void init(WindowState *window_state) {
        OverlayMenu::init(window_state);
        layout();
    }

private:
    int to_ix(PortDatatype type) { return static_cast<int>(type); }

    int to_ix(PortType type) { return static_cast<int>(type); }

    void layout() {
        constexpr int x = BASE_X + MENU_WIDTH / 2;
        int y = BASE_Y + MENU_HEIGHT / 4;
        comps.add(Box(x, y, MENU_WIDTH, MENU_HEIGHT / 2, 5));
        y += 20;
        comps.add(
            TextBox(x, y, MENU_WIDTH, 20, "Port " + port.name, *window_state));
        y += 40;
        typedef void (*Callback_t)(int64_t, InfoPort *);

        Callback_t type_choice = [](int64_t ix, InfoPort *self) {
            self->port.port_type = static_cast<PortType>(ix);
        };

        auto type_text = comps.add(TextBox(x + 20, y + 10, MENU_WIDTH - 40, 20,
                                           "Port type: ", *window_state));
        type_text->set_align(Alignment::LEFT);

        auto type_drop = comps.add(
            Dropdown(x + 110, y, 120, 40, "", {"Blocking"}, *window_state));
        type_drop->set_choice(to_ix(port.port_type));

        Callback_t size_choice = [](int64_t ix, InfoPort *self) {
            self->port.data_type = static_cast<PortDatatype>(ix);
        };

        auto size_text = comps.add(TextBox(x + 250, y + 10, MENU_WIDTH - 40, 20,
                                           "Data size: ", *window_state));
        size_text->set_align(Alignment::LEFT);
        std::vector<std::string> sizes{{"8-bit"}};
        if (features & ProcessorFeature::PORT_16) {
            sizes.push_back("16-bit");
        }
        if (features & ProcessorFeature::PORT_32) {
            sizes.push_back("32-bit");
        }
        if (features & ProcessorFeature::PORT_64) {
            sizes.push_back("64-bit");
        }
        auto size_drop =
            comps.add(Dropdown(x + 340, y, 120, 40, "", sizes, *window_state),
                      size_choice, this);
        size_drop->set_choice(to_ix(port.data_type));
    }

    PortTemplate &port;
    const bool input;
    const feature_t features;
};

ProcessorMenu::ProcessorMenu(State *parent,
                             std::vector<ProcessorTemplate> &templates,
                             std::size_t selected_ix)
    : OverlayMenu(parent), templates{templates}, selected_ix{selected_ix} {}

void ProcessorMenu::layout_instructions() {
    Components &c = instr_comps;
    c.clear();
    c.set_window_state(window_state);

    void (*remove_instr)(ProcessorMenu *, std::size_t) = [](ProcessorMenu *self,
                                                            std::size_t ix) {
        auto it = self->templates[self->selected_ix].instruction_set.begin();
        std::advance(it, ix);
        self->templates[self->selected_ix].instruction_set.erase(it);
        self->layout_instructions();
    };

    void (*info)(ProcessorMenu *, std::string, InstructionSlotType) =
        [](ProcessorMenu *self, std::string name, InstructionSlotType slot) {
            self->next_res.action = StateStatus::PUSH;
            self->next_res.new_state = new InstructionInfo(
                self, name, slot, self->templates[self->selected_ix].features);
        };

    std::size_t ix = selected_ix;

    std::size_t i = 0;
    for (const auto &instr : templates[ix].instruction_set) {
        auto mnemonic =
            instr.first + " " +
            instruction_mnemonic(instr.second, templates[ix].features);
        auto area = std::to_string(
            instruction_area(instr.second, templates[ix].features));
        const int y = BASE_Y + 90 + 60 * i;
        c.add(Box(BASE_X + MENU_WIDTH + 30, y, MENU_WIDTH - 60, 50, 2));
        c.add(TextBox(BASE_X + MENU_WIDTH + 30, y, 80, 50, instr.first,
                      *window_state));
        c.add(Box(BASE_X + MENU_WIDTH + 30, y, 80, 50, 2));
        c.add(TextBox(BASE_X + MENU_WIDTH + 125, y, 400, 50, mnemonic,
                      *window_state))
            ->set_align(Alignment::LEFT);
        c.add(Box(BASE_X + MENU_WIDTH + 405, y, 60, 50, 2));
        c.add(
            TextBox(BASE_X + MENU_WIDTH + 405, y, 60, 50, area, *window_state));
        c.add(Button(BASE_X + MENU_WIDTH + 485, y + 10, 30, 30, "x",
                     *window_state),
              remove_instr, this, i);
        c.add(Button(BASE_X + MENU_WIDTH + 545, y + 10, 30, 30, "i",
                     *window_state),
              info, this, instr.first, instr.second);
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

    int w = (MENU_WIDTH - 40 - 5 * 4) / 5;

    std::string inports;
    if (templates[selected_ix].in_ports.size() == 1) {
        inports = "1 input port";
    } else {
        inports =
            std::to_string(templates[ix].in_ports.size()) + " input ports";
    }
    input_ports->set_text(std::move(inports));
    std::string outports;
    if (templates[ix].out_ports.size() == 1) {
        outports = "1 output port";
    } else {
        outports =
            std::to_string(templates[ix].out_ports.size()) + " output ports";
    }
    output_ports->set_text(std::move(outports));

    void (*add_inport)(ProcessorMenu *) = [](ProcessorMenu *self) {
        std::string name = "A";
        auto &ports = self->templates[self->selected_ix].in_ports;
        for (std::size_t i = 0; i < ports.size();) {
            if (ports[i].name == name) {
                name[0] += 1;
                i = 0;
            } else {
                ++i;
            }
        }
        ports.push_back({name, PortDatatype::BYTE, PortType::BLOCKING});
        self->layout_instructions();
    };
    void (*add_outport)(ProcessorMenu *) = [](ProcessorMenu *self) {
        std::string name = "Z";
        auto &ports = self->templates[self->selected_ix].out_ports;
        for (std::size_t i = 0; i < ports.size();) {
            if (ports[i].name == name) {
                name[0] -= 1;
                i = 0;
            } else {
                ++i;
            }
        }
        ports.push_back({name, PortDatatype::BYTE, PortType::BLOCKING});
        self->layout_instructions();
    };
    void (*remove_port)(ProcessorMenu *, std::vector<PortTemplate> *,
                        std::size_t) = [](ProcessorMenu *self,
                                          std::vector<PortTemplate> *ports,
                                          std::size_t ix) {
        ports->erase(ports->begin() + ix);
        self->layout_instructions();
    };

    void (*info_port)(ProcessorMenu *, std::vector<PortTemplate> *,
                      std::size_t) = [](ProcessorMenu *self,
                                        std::vector<PortTemplate> *ports,
                                        std::size_t ix) {
        self->comps.enable_hover(false);
        self->instr_comps.enable_hover(false);
        self->next_res.action = StateStatus::PUSH;
        bool input = ports == &self->templates[self->selected_ix].in_ports;
        self->next_res.new_state =
            new InfoPort(self, ports->at(ix), input,
                         self->templates[self->selected_ix].features);
    };

    for (i = 0; i < templates[ix].in_ports.size(); ++i) {
        c.add(Box(BASE_X + 20 + (w + 5) * i, FIELDS_BASE + 420, w, 75, 2));
        c.add(TextBox(BASE_X + 20 + (w + 5) * i, FIELDS_BASE + 435, w, 5,
                      templates[ix].in_ports[i].name, *window_state));
        c.add(Button(BASE_X + 25 + (w + 5) * i, FIELDS_BASE + 450, 40, 40, "i",
                     *window_state),
              info_port, this, &templates[ix].in_ports, i);
        c.add(Button(BASE_X + 20 + w - 45 + (w + 5) * i, FIELDS_BASE + 450, 40,
                     40, "x", *window_state),
              remove_port, this, &templates[ix].in_ports, i);
    }
    if (i < 5) {
        c.add(Button(BASE_X + 20 + (w + 5) * i, FIELDS_BASE + 420, w, 75, "+",
                     *window_state),
              add_inport, this);
    }
    for (i = 0; i < templates[ix].out_ports.size(); ++i) {
        c.add(Box(BASE_X + 20 + (w + 5) * i, FIELDS_BASE + 530, w, 75, 2));
        c.add(TextBox(BASE_X + 20 + (w + 5) * i, FIELDS_BASE + 545, w, 5,
                      templates[ix].out_ports[i].name, *window_state));
        c.add(Button(BASE_X + 25 + (w + 5) * i, FIELDS_BASE + 560, 40, 40, "i",
                     *window_state),
              info_port, this, &templates[ix].out_ports, i);
        c.add(Button(BASE_X + 20 + w - 45 + (w + 5) * i, FIELDS_BASE + 560, 40,
                     40, "x", *window_state),
              remove_port, this, &templates[ix].out_ports, i);
    }
    if (i < 5) {
        c.add(Button(BASE_X + 20 + (w + 5) * i, FIELDS_BASE + 530, w, 75, "+",
                     *window_state),
              add_outport, this);
    }
}

void ProcessorMenu::change_processor(std::size_t ix) {
    proc_buttons[selected_ix]->enable_button(true);
    selected_ix = ix;
    proc_buttons[ix]->enable_button(false);

    name->set_text("Name: " + templates[ix].name);

    std::size_t reg_ix = 0;

    auto reg_text = [&ix, this, &reg_ix](DataSize size, std::string post,
                                         RegisterNames &names) {
        uint32_t count = 0;
        for (const auto &name : names) {
            count += name.second == size;
        }
        if (count == 0) {
            return;
        }
        post += (count > 1 ? " registers" : " register");
        std::string s = std::to_string(count) + post;
        register_types[reg_ix++]->set_text(std::move(s));
    };

    reg_text(DataSize::BYTE, " general purpose 8 bit",
             templates[ix].genreg_names);
    reg_text(DataSize::WORD, " general purpose 16 bit",
             templates[ix].genreg_names);
    reg_text(DataSize::DWORD, " general purpose 32 bit",
             templates[ix].genreg_names);
    reg_text(DataSize::QWORD, " general purpose 64 bit",
             templates[ix].genreg_names);
    reg_text(DataSize::DWORD, " floating point 32 bit",
             templates[ix].floatreg_names);
    reg_text(DataSize::QWORD, " floating point 64 bit",
             templates[ix].floatreg_names);

    if (reg_ix == 0) {
        register_types[reg_ix++]->set_text("None");
    }

    for (; reg_ix < register_types.size(); ++reg_ix) {
        register_types[reg_ix]->set_text("");
    }
    std::string flags = "";
    bool first = true;
    flag_t enabled_flags = ProcessorFeature::flags(templates[ix].features);
    auto add_flag = [&flags, enabled_flags, &first](std::string name,
                                                    flag_t mask) {
        if (enabled_flags & mask) {
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
    this->flags->set_text(flags);

    features->set_text(ProcessorFeature::string(templates[ix].features));

    layout_instructions();
}

void ProcessorMenu::init(WindowState *window_state) {
    OverlayMenu::init(window_state);

    comps.add(Box(BASE_X, BASE_Y, MENU_WIDTH, MENU_HEIGHT, 6));
    comps.add(Box(BASE_X + MENU_WIDTH, BASE_Y, MENU_WIDTH, MENU_HEIGHT, 6));

    void (*proc_btn)(ProcessorMenu *, std::size_t) =
        [](ProcessorMenu *self, std::size_t ix) { self->change_processor(ix); };

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

    name = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 30, MENU_WIDTH - 60, 5,
                             "", *window_state));
    name->set_align(Alignment::LEFT);

    void (*add_register)(ProcessorMenu *) = [](ProcessorMenu *self) {
        self->next_res.action = StateStatus::PUSH;
        self->next_res.new_state =
            new AddRegister(self, self->templates[self->selected_ix]);
        self->comps.enable_hover(false);
        self->instr_comps.enable_hover(false);
    };

    auto regs =
        comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 60, MENU_WIDTH - 40, 5,
                          "Registers: ", *window_state));
    regs->set_align(Alignment::LEFT);

    comps.add(
        Button(BASE_X + 105, FIELDS_BASE + 50, 25, 25, "+", *window_state),
        add_register, this);

    for (int i = 0; i < register_types.size(); ++i) {
        register_types[i] =
            comps.add(TextBox(BASE_X + 30, FIELDS_BASE + 80 + 20 * i,
                              MENU_WIDTH - 60, 5, "", *window_state));
        register_types[i]->set_align(Alignment::LEFT);
    }

    auto flags_title =
        comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 210, MENU_WIDTH - 40, 5,
                          "Flags: ", *window_state));
    flags_title->set_align(Alignment::LEFT);

    flags = comps.add(TextBox(BASE_X + 30, FIELDS_BASE + 230, MENU_WIDTH - 60,
                              5, "", *window_state));
    flags->set_align(Alignment::LEFT);

    auto features_title =
        comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 260, MENU_WIDTH - 40, 5,
                          "Features: ", *window_state));
    features_title->set_align(Alignment::LEFT);

    void (*add_feature)(ProcessorMenu *) = [](ProcessorMenu *self) {
        self->comps.enable_hover(false);
        self->instr_comps.enable_hover(false);
        self->next_res.action = StateStatus::PUSH;
        self->next_res.new_state =
            new AddFeature(self, self->templates[self->selected_ix]);
    };

    comps.add(
        Button(BASE_X + 95, FIELDS_BASE + 250, 25, 25, "+", *window_state),
        add_feature, this);

    features = comps.add(TextBox(BASE_X + 30, FIELDS_BASE + 285,
                                 MENU_WIDTH - 60, 5, "", *window_state));
    features->set_align(Alignment::LEFT);
    input_ports = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 400,
                                    MENU_WIDTH - 40, 5, "", *window_state));
    input_ports->set_align(Alignment::LEFT);

    output_ports = comps.add(TextBox(BASE_X + 20, FIELDS_BASE + 510,
                                     MENU_WIDTH - 40, 5, "", *window_state));
    output_ports->set_align(Alignment::LEFT);

    auto inst_title =
        comps.add(TextBox(BASE_X + MENU_WIDTH + 20, BASE_Y + 30, 120, 5,
                          "Instructions: ", *window_state));
    inst_title->set_align(Alignment::LEFT);

    comps.add(TextBox(BASE_X + MENU_WIDTH + 30, BASE_Y + 45, 80, 50, "Name",
                      *window_state));
    comps.add(TextBox(BASE_X + MENU_WIDTH + 110, BASE_Y + 45, 315, 50,
                      "Mnemonic", *window_state));
    comps.add(TextBox(BASE_X + MENU_WIDTH + 405, BASE_Y + 45, 60, 50, "Area",
                      *window_state));

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
                          MENU_WIDTH - 120, 60, "", *window_state));
    instr_slots->set_align(Alignment::LEFT);

    proc_btn(this, selected_ix);
}

void ProcessorMenu::resume() {
    templates[selected_ix].validate();

    change_processor(selected_ix);

    comps.enable_hover(true);
    instr_comps.enable_hover(true);
}

void ProcessorMenu::render() {
    OverlayMenu::render();
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
