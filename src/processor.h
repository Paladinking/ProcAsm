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

struct ProcessorTemplate {
    std::string name;

    std::vector<PortTemplate> in_ports {};
    std::vector<PortTemplate> out_ports {};

    RegisterNames register_names {};

    uint64_t instruction_slots {0};

    flag_t supported_flags {0};

    feature_t features {0};

    InstructionSet instruction_set {};

    bool read_from_json(const JsonObject& obj);

    Processor instantiate() const;
};

class Processor {
public:
    Processor(std::vector<std::unique_ptr<BytePort>> in_ports, std::vector<std::unique_ptr<BytePort>> out_ports,
              InstructionSet instruction_set, RegisterFile registers, feature_t features) noexcept;

    bool compile_program(std::vector<std::string> lines, std::vector<ErrorMsg>& errors);

    void clock_tick();

    bool is_valid() const noexcept;

    bool is_running() const noexcept;

    void invalidate();

    void reset();
private:
    friend class ProcessorGui;

    bool valid = false;
    bool running = false;

    std::vector<std::unique_ptr<BytePort>> in_ports;
    std::vector<std::unique_ptr<BytePort>> out_ports;

    std::vector<Instruction> instructions {};

    InstructionSet instruction_set;

    std::uint32_t pc;
    std::uint64_t ticks;

    RegisterFile registers;

    feature_t features;
};


#endif
