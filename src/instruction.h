#ifndef PROC_ASM_INSTRUCTION_H
#define PROC_ASM_INSTRUCTION_H
#include <cstdint>
#include <array>
#include <string>
#include <vector>

constexpr uint32_t MAX_OPERANDS = 4;

enum class DataSize {
    UNKNOWN = 0,
    BYTE = 1,
    WORD = 2,
    DWORD = 3,
    QWORD = 4
};

enum class InstructionType {
    IN = 0,
    OUT = 1,
    MOVE_REG = 2,
    MOVE_IMM = 3,
    ADD = 4,
    SUB = 5,
    JEZ = 6, 
    NOP = 7
};

// Only here and not in complier since it should closely match InstructionType
enum class InstructionSlotType {
    IN = 0, OUT = 1, MOVE_8 = 2, ADD = 4, SUB = 5, JEZ = 6, NOP = 7
};

struct Operand {
    uint32_t type;
    DataSize size;
    union {
        uint64_t reg;
        uint32_t label;
        uint64_t port;
        uint64_t imm_u;
        int64_t imm_s;
    };
};

struct Instruction {
    InstructionType id;
    std::array<Operand, MAX_OPERANDS> operands;
    uint64_t line;
};


class RegisterFile {
public:
    RegisterFile(uint16_t byte, uint16_t word, uint16_t dword, uint16_t qword);

    uint64_t gen_reg_count() const;

    uint64_t get_genreg(uint64_t reg) const;

    void set_genreg(uint64_t reg, DataSize size, uint64_t val);

    bool from_name(const std::string& name, uint64_t& ix, DataSize& size) const;

    void clear();

    std::string to_name(uint64_t ix) const;

    bool zero_flag = false;
private:
    uint16_t byte_regs, word_regs, dword_regs, qword_regs;

    std::vector<uint64_t> gen_registers {};

};
#endif
