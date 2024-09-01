#ifndef PROC_ASM_PROCESSOR_H
#define PROC_ASM_PROCESSOR_H
#include "instruction.h"
#include "json.h"
#include "compiler.h"
#include <vector>
#include "ports.h"

#ifndef PROC_GUI_HEAD
#define PROC_GUI_HEAD
class ProcessorGui;
#endif

constexpr int MAX_REGISTERS = 64;
constexpr int MAX_PORTS = 16;

enum class ProcessorChange {
    REGISTER, IN_PORT, OUT_PORT, TICKS, RUNNING
};

class Processor;

struct PortLayout {
    uint8_t up;
    uint8_t right;
    uint8_t down;
    uint8_t left;

    inline uint16_t total() const noexcept {
        return up + right + down + left;
    }

    inline std::string name(uint16_t ix) const noexcept {
        assert(ix < total());
        if (ix < up) {
            return "U" + std::to_string(ix);
        }
        if (ix < up + right) {
            return "R" + std::to_string(ix - up);
        }
        if (ix < up + right + down) {
            return "D" + std::to_string(ix - up - right);
        }
        return "L" + std::to_string(ix - up - right - down);
    }
};

struct ProcessorTemplate {
    std::string name;

    std::vector<PortTemplate> ports {};
    PortLayout port_layout;

    RegisterNames genreg_names {};
    RegisterNames floatreg_names {};

    uint64_t instruction_slots {0};

    feature_t features {0};

    InstructionSet instruction_set {};

    /**
      * Removes any registers / ports / instructions not supported by features. 
      **/ 
    void validate();

    bool read_from_json(const JsonObject& obj);

    Processor instantiate() const;
};


class Processor {
public:
    Processor(std::vector<std::shared_ptr<SharedPort>> ports,
              PortLayout port_layout, InstructionSet instruction_set,
              RegisterFile registers, feature_t features) noexcept;

    bool compile_program(std::vector<std::string> lines, std::vector<ErrorMsg>& errors);

    void in_tick();

    void out_tick();

    void clock_tick();

    bool is_valid() const noexcept;

    bool is_running() const noexcept;

    void invalidate();

    void reset();
private:
    friend class ProcessorGui;

    bool valid = false;
    bool running = false;

    // All ports: up, right, down, left
    std::vector<std::shared_ptr<SharedPort>> ports;
    PortLayout port_layout;

    std::vector<Instruction> instructions {};

    InstructionSet instruction_set;

    std::uint32_t pending_pc;

    std::uint32_t pc;
    std::uint64_t ticks;

    RegisterFile registers;

    feature_t features;
};


#endif
