#ifndef PROC_ASM_PROCESSOR_H
#define PROC_ASM_PROCESSOR_H
#include "instruction.h"
#include "event_id.h"
#include "compiler.h"
#include <vector>
#include <functional>
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

class Processor {
public:
    Processor(uint32_t register_count) noexcept;

    bool compile_program(const std::vector<std::string> lines, std::vector<ErrorMsg>& errors);

    void clock_tick();

    void register_events(Events* events);

    bool is_valid() const noexcept;

    bool is_running() const noexcept;

    void invalidate();

    void reset();
private:
    friend class ProcessorGui;

    bool valid = false;
    bool running = false;

    std::vector<BlockingBytePort<uint8_t>> in_ports {{"A"}, {"B"}};
    std::vector<BlockingBytePort<uint8_t>> out_ports {{"Z"}};
    std::vector<Instruction> instructions {};

    std::uint32_t pc;
    std::uint64_t ticks;

    bool zero_flag = false;

    std::vector<uint8_t> gen_registers;

    Events* events;
};


#endif
