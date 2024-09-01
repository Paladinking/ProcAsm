#ifndef PROC_ASM_INSTRUCTION_H
#define PROC_ASM_INSTRUCTION_H
#include <array>
#include <cstdint>
#include <string>

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
constexpr feature_t ALU = 1 << 0;
constexpr feature_t REGISTER_FILE = 1 << 1;
constexpr feature_t INOUT_PORTS = 1 << 2;
constexpr feature_t ALU_IMM = 1 << 3;
constexpr feature_t ZERO_FLAG = 1 << 4;
constexpr feature_t CARRY_FLAG = 1 << 5;
constexpr feature_t REG_16 = 1 << 6;
constexpr feature_t REG_32 = 1 << 7;
constexpr feature_t REG_64 = 1 << 8;
constexpr feature_t GEN_FLOAT = 1 << 9;
constexpr feature_t REG_FLOAT = 1 << 10;
constexpr feature_t REG_DOUBLE = 1 << 11;
constexpr feature_t FPU = 1 << 12;
constexpr feature_t PORT_16 = 1 << 13;
constexpr feature_t PORT_32 = 1 << 14;
constexpr feature_t PORT_64 = 1 << 15;

constexpr std::size_t COUNT = 16;

constexpr feature_t ALL =
    ALU | INOUT_PORTS | REGISTER_FILE | ALU_IMM | ZERO_FLAG | CARRY_FLAG
    | REG_16 | REG_32 | REG_64 | GEN_FLOAT | REG_FLOAT | REG_DOUBLE
    | FPU | PORT_16 | PORT_32 | PORT_64;

inline constexpr flag_t flags(feature_t features) {
    return (features & ZERO_FLAG ? FLAG_ZERO_MASK : 0) |
           (features & CARRY_FLAG ? FLAG_CARRY_MASK : 0);
}

inline std::string string(feature_t features) {
    std::string res = "";
    auto add = [&res, &features](feature_t feature, const char* s) {
        if (features & feature) {
            res = (res + s) + ", ";
        }
    };
    add(ALU, "ALU");
    add(INOUT_PORTS, "Problem ports");
    add(REGISTER_FILE, "Register file");
    add(ALU_IMM, "ALU Immediate operands");
    add(ZERO_FLAG, "Z flag");
    add(CARRY_FLAG, "C flag");
    add(REG_16, "16-bit registers");
    add(REG_32, "32-bit registers");
    add(REG_64, "64-bit registers");
    add(GEN_FLOAT, "Float operations for general purpose registers");
    add(REG_FLOAT, "32-bit floating point registers");
    add(REG_DOUBLE, "64-bit floating point registers");
    add(PORT_16, "16-bit ports");
    add(PORT_32, "32-bit ports");
    add(PORT_64, "64-bit ports");
    add(FPU, "FPU");
    if (res.size() == 0) {
        return "None";
    }
    res.pop_back();
    res.pop_back();
    return res;
}


constexpr inline uint32_t area(feature_t features) {
    uint32_t area = 0;
#define AREA_ADD(feature, a) if (features & feature) { area += a; }
    AREA_ADD(ALU, 50);
    AREA_ADD(INOUT_PORTS, 20);
    AREA_ADD(REGISTER_FILE, 20);
    AREA_ADD(ALU_IMM, 16);
    AREA_ADD(ZERO_FLAG, 10);
    AREA_ADD(CARRY_FLAG, 10);
    AREA_ADD(REG_16, 16);
    AREA_ADD(REG_32, 24);
    AREA_ADD(REG_64, 30);
    AREA_ADD(GEN_FLOAT, 4);
    AREA_ADD(REG_FLOAT, 12);
    AREA_ADD(REG_DOUBLE, 16);
    AREA_ADD(FPU, 40);
    AREA_ADD(PORT_16, 10);
    AREA_ADD(PORT_32, 10);
    AREA_ADD(PORT_64, 10);
#undef AREA_ADD
    return area;
}

constexpr inline feature_t dependents(feature_t features) {
    feature_t deps = 0;
#define DEPEND_ADD(dep, feature) if (features & feature) { deps |= dep; }
    DEPEND_ADD(REG_FLOAT, FPU);
    DEPEND_ADD(REG_DOUBLE, REG_FLOAT);
    DEPEND_ADD(GEN_FLOAT, REG_16);
    DEPEND_ADD(GEN_FLOAT, FPU);
    DEPEND_ADD(REG_64, REG_32);
    DEPEND_ADD(REG_32, REG_16);
    DEPEND_ADD(REG_16, REGISTER_FILE);
    DEPEND_ADD(ALU_IMM, ALU);
    DEPEND_ADD(PORT_32, PORT_16);
    DEPEND_ADD(PORT_64, PORT_32);
#undef DEPEND_ADD
    return deps;
}

// Returns all features directly required by any feature
// in <features>.
constexpr inline feature_t dependencies(feature_t features) {
    feature_t deps = 0;
#define DEPEND_ADD(feature, dep) if (features & feature) { deps |= dep; }
    DEPEND_ADD(REG_FLOAT, FPU);
    DEPEND_ADD(REG_DOUBLE, REG_FLOAT);
    DEPEND_ADD(GEN_FLOAT, REG_16);
    DEPEND_ADD(GEN_FLOAT, FPU);
    DEPEND_ADD(REG_64, REG_32);
    DEPEND_ADD(REG_32, REG_16);
    DEPEND_ADD(REG_16, REGISTER_FILE);
    DEPEND_ADD(ALU_IMM, ALU);
    DEPEND_ADD(PORT_32, PORT_16);
    DEPEND_ADD(PORT_64, PORT_32);
#undef DEPEND_ADD
    return deps;
}

} // namespace ProcessorFeature

/**
 * Returns the mnemonic for <i>.
 */
std::string instruction_mnemonic(InstructionSlotType i, feature_t features);

uint32_t instruction_area(InstructionSlotType type, feature_t features);

const char* instruction_description(InstructionSlotType i);

std::string instruction_usage(InstructionSlotType i, const std::string& name);

feature_t instruction_features(InstructionSlotType i);

#endif
