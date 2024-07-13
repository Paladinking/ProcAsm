#ifndef PROC_ASM_COMPILER_H
#define PROC_ASM_COMPILER_H

#include "instruction.h"
#include "editlines.h"
#include <vector>
#include <string>
#include <unordered_map>

constexpr uint32_t NONE = 0;
constexpr uint32_t GEN_REG = 1;
constexpr uint32_t IN_PORT = 2;
constexpr uint32_t OUT_PORT = 4;
constexpr uint32_t LABEL = 8;
constexpr uint32_t IMMU8 = 16;
constexpr uint32_t IMMU16 = 32;
constexpr uint32_t IMMU32 = 64;
constexpr uint32_t IMMU64 = 128;
constexpr uint32_t IMMS8 = 256;
constexpr uint32_t IMMS16 = 512;
constexpr uint32_t IMMS32 = 1024;
constexpr uint32_t IMMS64 = 2048;

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

typedef std::unordered_map<std::string, InstructionSlot> InstructionSet;

extern InstructionSet BASIC_INSTRUCTIONS;

class Compiler {
public:
    Compiler(const RegisterFile& registers, std::vector<std::string> in_ports, std::vector<std::string> out_ports,
            std::vector<Instruction>& instructions, const InstructionSet& instruction_set);
    bool compile(const std::vector<std::string>& lines, std::vector<ErrorMsg>& errors);

private:
    Instruction parse(InstructionSlot slot, const std::string& line, const std::vector<std::pair<std::size_t, std::size_t>>& opers,
                      std::unordered_map<std::string, uint32_t> labels, ErrorMsg& error) const;


    const RegisterFile& registers;
    const InstructionSet& instruction_set;
    std::vector<Instruction>& instructions;

    std::vector<std::string> in_ports;
    std::vector<std::string> out_ports;

    uint32_t gen_reg_count = 4;
};

#endif
