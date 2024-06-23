#ifndef PROC_ASM_INSTRUCTION_H
#define PROC_ASM_INSTRUCTION_H
#include <cstdint>
#include <array>

constexpr uint32_t MAX_OPERANDS = 4;

enum class InstructionType {
    IN = 0, OUT = 1, MOVE_REG = 2, MOVE_IMM = 3,
    ADD = 4, SUB = 5, JEZ = 6, NOP = 7
};

// Only here and not in complier since it should closely match InstructionType
enum class InstructionSlotType {
    IN = 0, OUT = 1, MOVE_8 = 2, ADD = 4, SUB = 5, JEZ = 6, NOP = 7
};

struct Operand {
    uint32_t type;
    union {
        int reg;
        uint32_t label;
        int port;
        uint64_t imm_u;
        int64_t imm_s;
    };
};

struct Instruction {
    InstructionType id;
    std::array<Operand, MAX_OPERANDS> operands;
    uint64_t line;
};

#endif
