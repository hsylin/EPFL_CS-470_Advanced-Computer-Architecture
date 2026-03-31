#pragma once

#include <cstdint>
#include <cstddef>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class PhysicalRegisterFile
{
public:
    PhysicalRegisterFile();

    void reset();
    void set(std::size_t index, uint64_t value);
    uint64_t get(std::size_t index) const;
    void dump(json& j) const;

private:
    uint64_t registers[64];
};