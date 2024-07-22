#include "compiler.h"
#include "engine/log.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <unordered_map>
#include <utility>

// Converts a bitmap of operands into a name
std::string operand_name(uint32_t operand, DataSize size) {
    switch (operand) {
    case GEN_REG:
        return "register";
    case IN_PORT:
        return "input port";
    case OUT_PORT:
        return "output port";
    case LABEL:
        return "label";
    case GEN_IMM:
        switch (size) {
        case DataSize::BYTE:
            return "8-bit integer";
        case DataSize::WORD:
            return "16-bit integer";
        case DataSize::DWORD:
            return "32-bit integer";
        case DataSize::QWORD:
            return "64-bit integer";
        default:
            return "integer";
        }
    case LABEL | GEN_REG:
        return "jump target";
    default:
        return "operand";
    }
}

bool read_uint(const char *s, uint64_t max, uint64_t &val) {
    if (s[0] == '\0') {
        return false;
    }
    val = 0;
    char c;
    while ((c = *s++) != '\0') {
        if (c < '0' || c > '9') {
            return false;
        }
        uint64_t v = c - '0';
        // Detect wrap-around
        if (val >= 1844674407370955161) {
            if (val == 1844674407370955161) {
                if (v > 5) {
                    return false;
                }
            } else {
                return false;
            }
        }
        val = 10 * val + v;
        if (val > max) {
            return false;
        }
    }
    return true;
}

bool read_uint(const std::string &s, uint64_t max, uint64_t &val) {
    return read_uint(s.c_str(), max, val);
}

bool read_int(const std::string &s, uint64_t max, int64_t &val) {
    if (s.size() == 0) {
        return false;
    }
    bool negative = false;
    if (s[0] == '-') {
        negative = true;
    }
    uint64_t v;
    if (!read_uint(negative ? s.c_str() + 1 : s.c_str(),
                   negative ? max + 1 : max, v)) {
        return false;
    }
    if (negative) {
        if (val == static_cast<uint64_t>(INT64_MAX) + 1) {
            val = INT64_MIN;
        } else {
            val = -v;
        }
    } else {
        val = v;
    }
    return true;
}

bool read_gint(const std::string &s, DataSize &size, uint64_t &val) {
    if (s.size() == 0) {
        return false;
    }
    if (s[0] == '-') {
        if (!read_uint(s.c_str() + 1, INT64_MAX, val)) {
            return false;
        }
        if (val <= INT8_MAX) {
            size = DataSize::BYTE;
        } else if (val <= INT16_MAX) {
            size = DataSize::WORD;
        } else if (val <= INT32_MAX) {
            size = DataSize::DWORD;
        } else {
            size = DataSize::QWORD;
        }
        val = (~val + 1);
        return true;
    }
    if (!read_uint(s, UINT64_MAX, val)) {
        return false;
    }
    if (val <= UINT8_MAX) {
        size = DataSize::BYTE;
    } else if (val <= UINT16_MAX) {
        size = DataSize::WORD;
    } else if (val <= UINT32_MAX) {
        size = DataSize::DWORD;
    } else {
        size = DataSize::QWORD;
    }
    return true;
}

const InstructionSet ALL_INSTRUCTIONS = {
    {"IN", InstructionSlotType::IN},      {"OUT", InstructionSlotType::OUT},
    {"MOV", InstructionSlotType::MOVE_8}, {"ADD", InstructionSlotType::ADD},
    {"SUB", InstructionSlotType::SUB},    {"JEZ", InstructionSlotType::JEZ},
    {"NOP", InstructionSlotType::NOP}};

void get_opers(InstructionSlotType i, feature_t features,
               std::vector<OperandSlot> &opers) {
    opers.resize(0);
    switch (i) {
    case InstructionSlotType::IN:
        opers.push_back({GEN_REG, true});
        opers.push_back({IN_PORT, true});
        return;
    case InstructionSlotType::OUT:
        opers.push_back({GEN_REG, true});
        opers.push_back({OUT_PORT, true});
        return;
    case InstructionSlotType::MOVE_8:
        opers.push_back({GEN_REG, true});
        opers.push_back({GEN_REG | GEN_IMM, true});
        return;
    case InstructionSlotType::ADD:
    case InstructionSlotType::SUB:
        opers.push_back({GEN_REG, true});
        if (features & ProcessorFeature::ALU_IMM) {
            opers.push_back({GEN_REG | GEN_IMM, true});
        } else {
            opers.push_back({GEN_REG, true});
        }
        return;
    case InstructionSlotType::JEZ:
        opers.push_back({LABEL, true});
        return;
    default:
        return;
    }
}

std::string instruction_mnemonic(InstructionSlotType i, feature_t features) {
    std::vector<OperandSlot> opers;
    get_opers(i, features, opers);
    std::string res{};
    for (const auto &oper : opers) {
        res += '<';
        if (oper.type & GEN_REG) {
            res += "register|";
        }
        if (oper.type & (IN_PORT | OUT_PORT)) {
            res += "port|";
        }
        if (oper.type & LABEL) {
            res += "label|";
        }
        if (oper.type & GEN_IMM) {
            res += "imm|";
        }
        if (!(oper.type & ANY_OPER_TYPE)) {
            res += "unknown|";
        }
        res.pop_back();
        res += "> ";
    }
    if (res.size() > 0) {
        res.pop_back();
    }
    return res;
}

uint32_t area_cost(InstructionSlotType slot, feature_t features) {
    switch (slot) {
    case InstructionSlotType::IN:
    case InstructionSlotType::OUT:
        return 10;
    case InstructionSlotType::MOVE_8:
        return 2;
    case InstructionSlotType::ADD:
    case InstructionSlotType::SUB:
        return 6;
    case InstructionSlotType::JEZ:
        return 8;
    case InstructionSlotType::NOP:
        return 1;
    default:
        return 255;
    }
}

feature_t required_features(InstructionSlotType slot) {
    switch (slot) {
    case InstructionSlotType::ADD:
    case InstructionSlotType::SUB:
        return ProcessorFeature::ALU;
    case InstructionSlotType::JEZ:
        return ProcessorFeature::ZERO_FLAG;
    default:
        return 0;
    }
}

Compiler::Compiler(const RegisterFile &registers,
                   std::vector<std::string> in_ports,
                   std::vector<std::string> out_ports,
                   std::vector<Instruction> &instructions,
                   const InstructionSet &instruction_set,
                   feature_t features)
    : registers{registers}, in_ports{std::move(in_ports)},
      out_ports{std::move(out_ports)}, instructions{instructions},
      instruction_set{instruction_set}, features{features} {}

Instruction Compiler::parse(
    InstructionSlotType slot, const std::vector<OperandSlot> &slot_operands,
    const std::string &line,
    const std::vector<std::pair<std::size_t, std::size_t>> &opers,
    std::unordered_map<std::string, uint32_t> labels, ErrorMsg &error) const {
    std::size_t i = 1;
    std::size_t slot_ix = 0;
    std::array<Operand, MAX_OPERANDS> out_operands{};
    DataSize active_size = DataSize::UNKNOWN;
    memset(out_operands.data(), 0, MAX_OPERANDS * sizeof(Operand));

    for (; i < opers.size(); ++i, ++slot_ix) {
        std::size_t start = opers[i].first;
        std::size_t len = opers[i].second;
        const std::string original_part = line.substr(start, len);
        std::string part = original_part;
        for (auto &c : part) {
            c = std::toupper(c);
        }
        if (slot_ix >= slot_operands.size()) {
            error = {"Unexpected operand " + original_part,
                     {0, static_cast<int>(start)}};
            return {InstructionType::NOP};
        }
        if ((slot_operands[slot_ix].type & GEN_REG) != 0) {
            uint64_t reg;
            DataSize reg_size;
            if (registers.from_name(part, reg, reg_size)) {
                out_operands[i - 1].type = GEN_REG;
                out_operands[i - 1].reg = reg;
                out_operands[i - 1].size = reg_size;
                active_size = reg_size;
                continue;
            }
        }
        if ((slot_operands[slot_ix].type & IN_PORT) != 0) {
            auto pos = std::find(in_ports.begin(), in_ports.end(), part);
            if (pos != in_ports.end()) {
                out_operands[i - 1].type = IN_PORT;
                out_operands[i - 1].port = pos - in_ports.begin();
                continue;
            }
        }
        if ((slot_operands[slot_ix].type & OUT_PORT) != 0) {
            auto pos = std::find(out_ports.begin(), out_ports.end(), part);
            if (pos != out_ports.end()) {
                out_operands[i - 1].type = OUT_PORT;
                out_operands[i - 1].port = pos - out_ports.begin();
                continue;
            }
        }
        if ((slot_operands[slot_ix].type & LABEL) != 0) {
            auto it = labels.find(part);
            if (it != labels.end()) {
                out_operands[i - 1].type = LABEL;
                out_operands[i - 1].label = it->second;
                continue;
            }
        }
        uint64_t max;
        if ((slot_operands[slot_ix].type & GEN_IMM) != 0) {
            uint64_t val;
            assert(active_size != DataSize::UNKNOWN);
            DataSize size;
            if (read_gint(part, size, val)) {
                if (size <= active_size) {
                    out_operands[i - 1].type = GEN_IMM;
                    out_operands[i - 1].imm_u = val;
                    continue;
                }
            }
        }
        if (slot_operands[slot_ix].required ||
            slot_ix == slot_operands.size() - 1) {
            error = {
                "Invalid " +
                    operand_name(slot_operands[slot_ix].type, active_size) +
                    " " + original_part,
                {0, static_cast<int>(start)}};
            return {InstructionType::NOP};
        }
        --i;
    }
    if (slot_ix != slot_operands.size()) {
        error = {"Missing operand", {0, 0}};
        return {InstructionType::NOP};
    }
    InstructionType type;
    switch (slot) {
    case InstructionSlotType::MOVE_8:
        if (out_operands[1].type == GEN_REG) {
            type = InstructionType::MOVE_REG;
        } else {
            type = InstructionType::MOVE_IMM;
        }
        break;
    default:
        type = static_cast<InstructionType>(static_cast<int>(slot));
        break;
    }
    return {type, out_operands};
};

/*
 * Divides a line into parts split on ',', ' ' and '\t'.
 * Ignores labels and comments.
 */
std::vector<std::pair<std::size_t, std::size_t>> split(const std::string &s,
                                                       std::size_t &label_end) {
    std::vector<std::pair<std::size_t, std::size_t>> res{};
    std::size_t end_ix = s.find_last_of(";#");

    std::size_t start_ix = s.find(':');
    if (start_ix == std::string::npos || start_ix > end_ix) {
        start_ix = 0;
        label_end = 0;
    } else {
        label_end = start_ix;
        start_ix = start_ix + 1;
    }
    start_ix = s.find_first_not_of(", \t", start_ix);
    Instruction i{};

    while (true) {
        if (start_ix == std::string::npos || start_ix >= end_ix) {
            return res;
        }
        std::size_t ix = s.find_first_of(", \t;", start_ix);
        if (ix == std::string::npos) {
            res.emplace_back(start_ix, s.size() - start_ix);
            return res;
        }
        res.emplace_back(start_ix, ix - start_ix);
        start_ix = s.find_first_not_of(", \t", ix + 1);
    }
}

ErrorMsg::ErrorMsg(std::string msg, TextPosition pos)
    : msg{std::move(msg)}, pos{pos}, empty{false} {}

bool Compiler::compile(const std::vector<std::string> &lines,
                       std::vector<ErrorMsg> &errors) {
    errors.clear();
    int row = -1;
    uint32_t instruction_count = 0;
    std::vector<Instruction> res{};
    std::unordered_map<std::string, uint32_t> labels{};

    std::array<std::vector<OperandSlot>, INSTRUCTION_SLOT_COUNT> operand_map{};
    for (const auto &i : instruction_set) {
        auto &res = operand_map[static_cast<std::size_t>(i.second)];
        get_opers(i.second, features, res);
    }

    for (auto &line : lines) {
        ++row;
        std::size_t label_size;
        auto parts = split(line, label_size);
        if (parts.size() > 0) {
            ++instruction_count;
        }
        if (label_size > 0) {
            std::string label = line.substr(0, label_size);
            if (!std::all_of(label.begin(), label.end(), [](unsigned char c) {
                    return std::isalnum(c) || c == '_';
                })) {
                errors.emplace_back("Invalid label", TextPosition{row, 0});
                continue;
            }
            if ((label[0] == 'r' || label[0] == 'R') &&
                std::all_of(label.begin(), label.end(),
                            [](unsigned char c) { return std::isdigit(c); })) {
                errors.emplace_back("Invalid label", TextPosition{row, 0});
                continue;
            }
            if (std::isdigit(label[0])) {
                errors.emplace_back("Invalid label", TextPosition{row, 0});
                continue;
            }
            if (labels.find(label) != labels.end()) {
                errors.emplace_back("Duplicate label", TextPosition{row, 0});
                continue;
            }
            for (auto &c : label) {
                c = std::toupper(c);
            }
            labels.insert({label, instruction_count - 1});
        }
    }
    row = -1;
    LOG_DEBUG("Size: %d", instruction_set.size());
    for (auto &s : instruction_set) {
        LOG_DEBUG("INST: %s", s.first.c_str());
    }
    for (auto &line : lines) {
        ++row;
        std::size_t label_size;
        auto parts = split(line, label_size);
        if (parts.size() == 0) {
            continue;
        }
        std::string instr = line.substr(parts[0].first, parts[0].second);
        for (auto &c : instr) {
            c = std::toupper(c);
        }
        LOG_DEBUG("I: %s", instr.c_str());
        auto val = instruction_set.find(instr);
        if (val == instruction_set.end()) {
            errors.emplace_back(
                "Invalid instruction " +
                    line.substr(parts[0].first, parts[0].second),
                TextPosition{row, static_cast<int>(parts[0].first)});
            continue;
        }
        ErrorMsg error;
        Instruction i = parse(
            val->second, operand_map[static_cast<std::size_t>(val->second)],
            line, parts, std::move(labels), error);
        if (!error.empty) {
            error.pos.row = row;
            errors.push_back(error);
            continue;
        }
        i.line = row;
        res.push_back(i);
        char out[256];
        char *base = out;
        base = base + sprintf(base, "%d: ", static_cast<int>(i.id));
        for (int ix = 0; ix < MAX_OPERANDS; ++ix) {
            if (i.operands[ix].type == GEN_REG) {
                base = base + sprintf(base, "r%llu, ", i.operands[ix].reg);
            } else if (i.operands[ix].type == GEN_IMM) {
                base = base + sprintf(base, "%llu, ", i.operands[ix].imm_u);
            } else if (i.operands[ix].type == LABEL) {
                base = base + sprintf(base, "%d, ", i.operands[ix].label);
            } else if (i.operands[ix].type == IN_PORT) {
                base = base + sprintf(base, "%s, ",
                                      in_ports[i.operands[ix].port].c_str());
            } else if (i.operands[ix].type == OUT_PORT) {
                base = base + sprintf(base, "%s, ",
                                      out_ports[i.operands[ix].port].c_str());
            } else {
                break;
            }
        }
        LOG_DEBUG("%s", out);
    }
    if (errors.size() > 0) {
        return false;
    }

    instructions = std::move(res);
    return true;
}
