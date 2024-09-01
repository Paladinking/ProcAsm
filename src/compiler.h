#ifndef PROC_ASM_COMPILER_H
#define PROC_ASM_COMPILER_H

#include "instruction.h"
#include "registers.h"
#include "editlines.h"
#include <vector>
#include <string>
#include <map>
#include <unordered_map>

constexpr uint32_t GEN_REG = 1;
constexpr uint32_t PORT = 2;
constexpr uint32_t OUT_PORT = 4;
constexpr uint32_t LABEL = 8;
constexpr uint32_t GEN_IMM = 16;
constexpr uint32_t FLOAT_REG = 32;

constexpr uint32_t ANY_OPER_TYPE = GEN_REG | PORT |
                                   LABEL | GEN_IMM | FLOAT_REG;

struct OperandSlot {
    uint32_t type;
    bool required;
};


struct ErrorMsg {
    ErrorMsg() = default;
    ErrorMsg(std::string msg, TextPosition pos);

    std::string msg {};
    TextPosition pos {};
    bool empty = true;
};

struct InstructionSlot {
    InstructionSlotType id;
    std::vector<OperandSlot> operands;
};

typedef std::map<std::string, InstructionSlotType> InstructionSet;

extern const InstructionSet ALL_INSTRUCTIONS;


class Compiler {
public:
    Compiler(const RegisterFile& registers, std::vector<std::string> ports, std::vector<Instruction>& instructions, const InstructionSet& instruction_set, feature_t features);
    bool compile(const std::vector<std::string>& lines, std::vector<ErrorMsg>& errors);

private:
    Instruction parse(InstructionSlotType slot, const std::vector<OperandSlot>& slot_opers, const std::string& line, const std::vector<std::pair<std::size_t, std::size_t>>& opers,
                      std::unordered_map<std::string, uint32_t> labels, ErrorMsg& error) const;


    const RegisterFile& registers;
    const InstructionSet& instruction_set;
    std::vector<Instruction>& instructions;

    std::vector<std::string> ports;

    feature_t features;

    uint32_t gen_reg_count = 4;
};

#endif
