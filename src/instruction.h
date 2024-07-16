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
    MOVE_IMM = 7,
    ADD = 3,
    SUB = 4,
    JEZ = 5, 
    NOP = 6
};

// Only here and not in complier since it should closely match InstructionType
enum class InstructionSlotType {
    IN = 0, OUT = 1, MOVE_8 = 2, ADD = 3, SUB = 4, JEZ = 5, NOP = 6
};


constexpr uint32_t INSTRUCTION_SLOT_COUNT = 7;

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

typedef uint32_t flag_t;
constexpr flag_t FLAG_ZERO_IX = 0;
constexpr flag_t FLAG_ZERO_MASK = (1 << FLAG_ZERO_IX);

typedef std::vector<std::pair<std::string, DataSize>> RegisterNames;

class RegisterFile {
public:
    struct Register {
        uint64_t val;
        bool changed;
    };

    RegisterFile(RegisterNames register_names);

    uint64_t gen_reg_count() const;

    uint64_t get_genreg(uint64_t reg) const;

    void set_genreg(uint64_t reg, DataSize size, uint64_t val);

    bool from_name(const std::string& name, uint64_t& ix, DataSize& size) const;

    void clear();

    const std::string& to_name(uint64_t ix) const;

    bool poll_value(uint64_t ix, std::string& s);

    flag_t enabled_flags;
    flag_t flags = 0;
private:
    RegisterNames register_names;

    std::vector<Register> gen_registers {};

};
#endif
