#ifndef PROC_ASM_PROCESSOR_H
#define PROC_ASM_PROCESSOR_H
#include "instruction.h"
#include "compiler.h"
#include "engine/ui.h"
#include <vector>
#include <functional>
#include "ports.h"

#ifndef PROC_GUI_HEAD
#define PROC_GUI_HEAD
class ProcessorGui;
#endif

enum class ProcessorChange {
    REGISTER, IN_PORT, OUT_PORT, TICKS, RUNNING
};

class Processor {
public:
    Processor(uint32_t register_count) noexcept;

    bool compile_program(const std::vector<std::string> lines, std::vector<ErrorMsg>& errors);

    void clock_tick();

    void register_change_callback(std::function<void(ProcessorChange, uint32_t)> fn);

    bool is_valid() const noexcept;

    bool is_running() const noexcept;

    void invalidate();

    std::vector<InPort*> get_inputs();

    std::vector<OutPort*> get_outputs();
private:
    friend class ProcessorGui;

    bool valid = false;
    bool running = false;

    std::vector<ForwardingInputPort<uint8_t>> in_ports {{"A"}};
    std::vector<ForwardingOutputPort<uint8_t>> out_ports {{"Z"}};
    std::vector<Instruction> instructions {};

    std::uint32_t pc;
    std::uint64_t ticks;

    bool zero_flag = false;

    std::vector<uint8_t> gen_registers;

    std::function<void(ProcessorChange, uint32_t)> change_callback;
};


#endif
