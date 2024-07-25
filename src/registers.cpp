#include "registers.h"
#include "engine/log.h"
#include <cassert>

RegisterFile::RegisterFile(RegisterNames genreg_names,
                           RegisterNames floatreg_names, flag_t enabled_flags)
    : genreg_names{std::move(genreg_names)},
      floatreg_names{std::move(floatreg_names)}, enabled_flags{enabled_flags},
      gen_registers(this->genreg_names.size(), {0, true}) {
      }

void RegisterFile::set_genreg(uint64_t ix, DataSize size, uint64_t val) {
    switch (size) {
    case DataSize::BYTE:
        gen_registers[ix].val = val & 0xFF;
        break;
    case DataSize::WORD:
        gen_registers[ix].val = val & 0xFFFF;
        break;
    case DataSize::DWORD:
        gen_registers[ix].val = val & 0xFFFFFFFF;
        break;
    case DataSize::QWORD:
        gen_registers[ix].val = val;
        break;
    default:
        assert(false);
    }
    gen_registers[ix].changed = true;
    flags = (flags & ~FLAG_ZERO_MASK) |
            ((gen_registers[ix].val == 0) << FLAG_ZERO_IX);
    LOG_DEBUG("Flags: %d", flags);
}

uint64_t RegisterFile::count_genreg() const { return gen_registers.size(); }

uint64_t RegisterFile::get_genreg(uint64_t reg) const {
    return gen_registers[reg].val;
}

float32_t RegisterFile::get_floatreg_single(uint64_t ix) const {
    return float_registers[ix].val.f32;
}

float64_t RegisterFile::get_floatreg_double(uint64_t ix) const {
    return float_registers[ix].val.f64;
}

void RegisterFile::clear() {
    for (uint64_t i = 0; i < gen_registers.size(); ++i) {
        gen_registers[i].val = 0;
        gen_registers[i].changed = true;
    }
    flags = 0;
}

bool RegisterFile::from_name_genreg(const std::string &name, uint64_t &ix,
                                    DataSize &size) const {
    for (uint64_t i = 0; i < genreg_names.size(); ++i) {
        if (name == genreg_names[i].first) {
            ix = i;
            size = genreg_names[i].second;
            return true;
        }
    }
    return false;
}

const std::string &RegisterFile::to_name_genreg(uint64_t ix) const {
    return genreg_names[ix].first;
}

bool RegisterFile::poll_value_genreg(uint64_t ix, std::string &s) {
    if (gen_registers[ix].changed) {
        s = to_name_genreg(ix) + ": " + std::to_string(gen_registers[ix].val);
        gen_registers[ix].changed = false;
        return true;
    }
    return false;
}
