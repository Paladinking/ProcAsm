#ifndef REGISTERS_H
#define REGISTERS_H
#include "instruction.h"
#include <string>
#include <utility>
#include <vector>

#ifdef SOFT_FLOAT
extern "C" {
#include "softfloat.h"
}
#else
typedef float float32_t;
typedef double float64_t;
static_assert(sizeof(float) == 4, "4 byte float required");
static_assert(sizeof(double) == 8, "8 byte double required");
#endif

typedef std::vector<std::pair<std::string, DataSize>> RegisterNames;

class RegisterFile {
public:
    template <class T> struct Register {
        T val;
        bool changed;
    };

    RegisterFile(RegisterNames genreg_names, RegisterNames floatreg_names,
                 flag_t enabled_flags);

    uint64_t count_genreg() const;

    uint64_t get_genreg(uint64_t reg) const;

    float32_t get_floatreg_single(uint64_t reg) const;

    float64_t get_floatreg_double(uint64_t reg) const;

    void set_genreg(uint64_t reg, DataSize size, uint64_t val);

    void set_floatreg_single(uint64_t reg, float32_t val);

    void set_floatreg_double(uint64_t reg, float64_t val);

    bool from_name_genreg(const std::string &name, uint64_t &ix,
                          DataSize &size) const;

    bool from_name_floatreg(const std::string &name, uint64_t &ix,
                            DataSize &size) const;

    void clear();

    const std::string &to_name_genreg(uint64_t ix) const;

    bool poll_value_genreg(uint64_t ix, std::string &s);

    flag_t enabled_flags;
    flag_t flags = 0;

private:
    RegisterNames genreg_names;
    RegisterNames floatreg_names;

    union Float {
        float32_t f32;
        float64_t f64;
    };

    std::vector<Register<uint64_t>> gen_registers{};
    std::vector<Register<Float>> float_registers{};
};

#endif
