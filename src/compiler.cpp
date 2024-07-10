#include "compiler.h"
#include "engine/log.h"
#include <unordered_map>
#include <cctype>
#include <utility>
#include <algorithm>
#include <array>



uint64_t immu_max(uint32_t flags) {
    if (flags & IMMU64) {
        return UINT64_MAX;
    }
    if (flags & IMMU32) {
        return UINT32_MAX;
    }
    if (flags & IMMU16) {
        return UINT16_MAX;
    }
    if (flags & IMMU8) {
        return UINT8_MAX;
    }
    return 0;
}

uint64_t imms_max(uint32_t flags) {
    return (immu_max(flags >> 4) >> 1);
}

// Converts a bitmap of operands into a name
std::string operand_name(uint32_t operand) {
    switch (operand) {
        case GEN_REG:
            return "register";
        case IN_PORT:
            return "input port";
        case OUT_PORT:
            return "output port";
        case LABEL:
            return "label";
        case IMMU8:
            return "8-bit unsinged integer";
        case IMMU16:
            return "16-bit unsinged integer";
        case IMMU32:
            return "32-bit unsinged integer";
        case IMMU64:
            return "64-bit unsinged integer";
        case IMMS8:
            return "8-bit signed integer";
        case IMMS16:
            return "16-bit signed integer";
        case IMMS32:
            return "32-bit signed integer";
        case IMMS64:
            return "64-bit signed integer";
        case LABEL | GEN_REG:
            return "destination";
        default:
            return "operand";
    }
}



bool read_uint(const char* s, uint64_t max, uint64_t& val) {
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

bool read_uint(const std::string& s, uint64_t max, uint64_t& val) {
    return read_uint(s.c_str(), max, val);
}

bool read_int(const std::string& s, uint64_t max, int64_t& val) {
    if (s.size() == 0) {
        return false;
    }
    bool negative = false;
    if (s[0] == '-') {
        negative = true;
    }
    uint64_t v;
    if (!read_uint(negative ? s.c_str() + 1 : s.c_str(), negative ? max + 1: max, v)) {
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

Compiler::Compiler(const RegisterFile& registers, std::vector<std::string> in_ports, std::vector<std::string> out_ports,
            std::vector<Instruction>& instructions, const std::unordered_map<std::string, InstructionSlot>& instruction_set) :
            registers {registers}, in_ports{std::move(in_ports)}, out_ports{std::move(out_ports)},
            instructions{instructions}, instruction_set{instruction_set} {}

Instruction Compiler::parse(InstructionSlot slot, const std::string& line, const std::vector<std::pair<std::size_t, std::size_t>>& opers,
                            std::unordered_map<std::string, uint32_t> labels, ErrorMsg& error) const {
    std::size_t i = 1;
    std::size_t slot_ix = 0;
    std::array<Operand, MAX_OPERANDS> out_operands {};
    memset(out_operands.data(), 0, MAX_OPERANDS * sizeof(Operand));

    for (; i < opers.size(); ++i, ++slot_ix) {
        std::size_t start = opers[i].first;
        std::size_t len = opers[i].second;
        const std::string original_part = line.substr(start, len);
        std::string part = original_part;
        for (auto& c: part) {
            c = std::toupper(c);
        }
        if (slot_ix >= slot.operands.size()) {
            error = {"Unexpected operand " + original_part, {0, static_cast<int>(start)}};
            return {InstructionType::NOP};
        }
        if ((slot.operands[slot_ix].type & GEN_REG) != 0) {
            uint64_t reg;
            DataSize reg_size;
            if (registers.from_name(part, reg, reg_size)) {
                out_operands[i - 1].type = GEN_REG;
                out_operands[i - 1].reg = reg;
                out_operands[i - 1].size = reg_size;
                continue;
            } 
        }
        if ((slot.operands[slot_ix].type & IN_PORT) != 0) {
            auto pos = std::find(in_ports.begin(), in_ports.end(), part);
            if (pos != in_ports.end()) {
                out_operands[i - 1].type = IN_PORT;
                out_operands[i - 1].port = pos - in_ports.begin();
                continue;
            }
        }
        if ((slot.operands[slot_ix].type & OUT_PORT) != 0) {
            auto pos = std::find(out_ports.begin(), out_ports.end(), part);
            if (pos != out_ports.end()) {
                out_operands[i - 1].type = OUT_PORT;
                out_operands[i - 1].port = pos - out_ports.begin();
                continue;
            }
        }
        if ((slot.operands[slot_ix].type & LABEL) != 0) {
            auto it = labels.find(part);
            if (it != labels.end()) {
                out_operands[i - 1].type = LABEL;
                out_operands[i - 1].label = it->second;
                continue;
            }
        }
        uint64_t max;
        if ((max = immu_max(slot.operands[slot_ix].type)) != 0) {
            uint64_t val;
            if (read_uint(part, max, val)) {
                out_operands[i - 1].type = IMMU64;
                out_operands[i - 1].imm_u = val;
                continue;
            }
        }
        if ((max = imms_max(slot.operands[slot_ix].type)) != 0) {
            int64_t val;
            if (read_int(part, max, val)) {
                out_operands[i - 1].type = IMMS64;
                out_operands[i - 1].imm_s = val;
                continue;
            }
        }
        if (slot.operands[slot_ix].required || slot_ix == slot.operands.size() - 1) {
            error = {"Invalid " + operand_name(slot.operands[slot_ix].type) + " " + original_part, {0, static_cast<int>(start)}};
            return {InstructionType::NOP};
        }
        --i;
    }
    if (slot_ix != slot.operands.size()) {
        error = {"Missing operand", {0, 0}};
        return {InstructionType::NOP};
    }
    InstructionType type;
    switch(slot.id) {
        case InstructionSlotType::MOVE_8:
            if (out_operands[1].type == GEN_REG) {
                type = InstructionType::MOVE_REG;
            } else {
                type = InstructionType::MOVE_IMM;
            }
            break;
        default:
            type = static_cast<InstructionType>(static_cast<int>(slot.id));
            break;
    }
    return {type, out_operands};
};



std::unordered_map<std::string, InstructionSlot> BASIC_INSTRUCTIONS = {
    {"IN", {InstructionSlotType::IN, {{GEN_REG, true}, {IN_PORT, false}}}},
    {"OUT", {InstructionSlotType::OUT, {{GEN_REG, true}, {OUT_PORT, false}}}},
    {"MOV", {InstructionSlotType::MOVE_8, {{GEN_REG, true}, {GEN_REG | IMMU8, true}}}},
    {"ADD", {InstructionSlotType::ADD, {{GEN_REG, true}, {GEN_REG, true}}}},
    {"SUB", {InstructionSlotType::SUB, {{GEN_REG, true}, {GEN_REG, true}}}},
    {"JEZ", {InstructionSlotType::JEZ, {{LABEL, true}}}},
    {"NOP", {InstructionSlotType::NOP, {}}}
};

/*
 * Divides a line into parts split on ',', ' ' and '\t'.
 * Ignores labels and comments.
 */
std::vector<std::pair<std::size_t, std::size_t>> split(const std::string& s, std::size_t& label_end) {
    std::vector<std::pair<std::size_t, std::size_t>> res {};
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
    Instruction i {};

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

ErrorMsg::ErrorMsg(std::string msg, TextPosition pos) : msg{std::move(msg)}, pos{pos}, empty{false} {}

bool Compiler::compile(const std::vector<std::string>& lines, std::vector<ErrorMsg>& errors) {
    errors.clear();
    int row = -1;
    uint32_t instruction_count = 0;
    std::vector<Instruction> res {};
    std::unordered_map<std::string, uint32_t> labels{};
    for (auto& line: lines) {
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
            if ((label[0] == 'r' || label[0] == 'R') && std::all_of(label.begin(), label.end(), [](unsigned char c) {
                return std::isdigit(c);
            })) {
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
            for (auto& c: label) {
                c = std::toupper(c);
            }
            labels.insert({label, instruction_count - 1});
        }
    }
    row = -1;
    for (auto& line: lines) {
        ++row;
        std::size_t label_size;
        auto parts = split(line, label_size);
        if (parts.size() == 0) {
            continue;
        }
        std::string instr = line.substr(parts[0].first, parts[0].second);
        for (auto& c : instr) {
            c = std::toupper(c);
        }
        auto val = instruction_set.find(instr);
        if (val == instruction_set.end()) {
            errors.emplace_back("Invalid instruction " + line.substr(parts[0].first, parts[0].second),
                                TextPosition{row, static_cast<int>(parts[0].first)});
            continue;
        }
        ErrorMsg error;
        Instruction i = parse(val->second, line, parts, std::move(labels), error);
        if (!error.empty) {
            error.pos.row = row;
            errors.push_back(error);
            continue;
        }
        i.line = row;
        res.push_back(i);
        char out[256];
        char* base = out;
        base = base + sprintf(base, "%d: ", static_cast<int>(i.id));
        for (int ix = 0; ix < MAX_OPERANDS; ++ix) {
            if (i.operands[ix].type == GEN_REG) {
                base = base + sprintf(base, "r%llu, ", i.operands[ix].reg);
            } else if (i.operands[ix].type == IMMU64) {
                base = base + sprintf(base, "%llu, ", i.operands[ix].imm_u);
            } else if (i.operands[ix].type == IMMS64) {
                base = base + sprintf(base, "%lld, ", i.operands[ix].imm_s);
            } else if (i.operands[ix].type == LABEL) {
                base = base + sprintf(base, "%d, ", i.operands[ix].label);
            } else if (i.operands[ix].type == IN_PORT) {
                base = base + sprintf(base, "%s, ", in_ports[i.operands[ix].port].c_str());
            } else if (i.operands[ix].type == OUT_PORT) {
                base = base + sprintf(base, "%s, ", out_ports[i.operands[ix].port].c_str());
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
