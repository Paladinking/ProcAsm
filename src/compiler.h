#ifndef PROC_ASM_COMPILER_H
#define PROC_ASM_COMPILER_H

#include "instruction.h"
#include "editlines.h"
#include <vector>
#include <string>
#include <unordered_map>


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

struct InstuctionSlot {
    InstructionSlotType id;
    std::vector<OperandSlot> operands;
};

extern std::unordered_map<std::string, InstuctionSlot> BASIC_INSTRUCTIONS;

class Compiler {
public:
    Compiler(uint32_t gen_reg_count, std::vector<std::string> in_ports, std::vector<std::string> out_ports,
            std::vector<Instruction>& instructions, const std::unordered_map<std::string, InstuctionSlot>& instruction_set);
    bool compile(const std::vector<std::string>& lines, std::vector<ErrorMsg>& errors);

private:
    Instruction parse(InstuctionSlot slot, const std::string& line, const std::vector<std::pair<std::size_t, std::size_t>>& opers,
                      std::unordered_map<std::string, uint32_t> labels, ErrorMsg& error) const;

    const std::unordered_map<std::string, InstuctionSlot>& instruction_set;
    std::vector<Instruction>& instructions;

    std::vector<std::string> in_ports;
    std::vector<std::string> out_ports;

    uint32_t gen_reg_count = 4;
};

#endif
