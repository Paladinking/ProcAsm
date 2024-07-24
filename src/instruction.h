#ifndef PROC_ASM_INSTRUCTION_H
#define PROC_ASM_INSTRUCTION_H
#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr uint32_t MAX_OPERANDS = 4;

enum class DataSize { UNKNOWN = 0, BYTE = 1, WORD = 2, DWORD = 3, QWORD = 4 };

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
// Should be continuous and end with NOP.
enum class InstructionSlotType {
    IN = 0,
    OUT = 1,
    MOVE_8 = 2,
    ADD = 3,
    SUB = 4,
    JEZ = 5,
    NOP = 6
};
constexpr uint32_t INSTRUCTION_SLOT_COUNT =
    static_cast<uint32_t>(InstructionSlotType::NOP) + 1;

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
constexpr flag_t FLAG_CARRY_IX = 1;
constexpr flag_t FLAG_ZERO_MASK = (1 << FLAG_ZERO_IX);
constexpr flag_t FLAG_CARRY_MASK = (1 << FLAG_CARRY_IX);

typedef uint64_t feature_t;
namespace ProcessorFeature {
constexpr feature_t ALU = 1;
constexpr feature_t REGISTER_FILE = 2;
constexpr feature_t ALU_IMM = 4;
constexpr feature_t ZERO_FLAG = 8;
constexpr feature_t CARRY_FLAG = 16;

constexpr feature_t ALL =
    ALU | REGISTER_FILE | ALU_IMM | ZERO_FLAG | CARRY_FLAG;

inline std::string string(feature_t features) {
    std::string res = "";
    if (features & ALU) {
        res += "ALU, ";
    }
    if (features & REGISTER_FILE) {
        res += "Register file, ";
    }
    if (features & ALU_IMM) {
        res += "ALU Immediate operands, ";
    }
    if (features & ZERO_FLAG) {
        res += "Z flag, ";
    }
    if (features & CARRY_FLAG) {
        res += "C flag, ";
    }
    if (res.size() == 0) {
        return "None";
    }
    res.pop_back();
    res.pop_back();
    return res;
}

constexpr inline uint32_t area(feature_t features) {
    uint32_t area = 0;
    if (features & ALU) {
        area += 50;
    }
    if (features & REGISTER_FILE) {
        area += 20;
    }
    if (features & ALU_IMM) {
        area += 16;
    }
    if (features & ZERO_FLAG) {
        area += 10;
    }
    if (features & CARRY_FLAG) {
        area += 10;
    }
    return area;
}
} // namespace ProcessorFeature

/**
 * Returns the mnemonic for <i>.
 */
std::string instruction_mnemonic(InstructionSlotType i, feature_t features);

uint32_t instruction_area(InstructionSlotType type, feature_t features);

const char* instruction_description(InstructionSlotType i);

std::string instruction_usage(InstructionSlotType i, const std::string& name);

feature_t required_features(InstructionSlotType i);

typedef std::vector<std::pair<std::string, DataSize>> RegisterNames;

class RegisterFile {
public:
    struct Register {
        uint64_t val;
        bool changed;
    };

    RegisterFile(RegisterNames register_names, flag_t enabled_flags);

    uint64_t gen_reg_count() const;

    uint64_t get_genreg(uint64_t reg) const;

    void set_genreg(uint64_t reg, DataSize size, uint64_t val);

    bool from_name(const std::string &name, uint64_t &ix, DataSize &size) const;

    void clear();

    const std::string &to_name(uint64_t ix) const;

    bool poll_value(uint64_t ix, std::string &s);

    flag_t enabled_flags;
    flag_t flags = 0;

private:
    RegisterNames register_names;

    std::vector<Register> gen_registers{};
};
#endif
