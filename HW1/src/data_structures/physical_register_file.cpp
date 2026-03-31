#include "physical_register_file.hpp"

PhysicalRegisterFile::PhysicalRegisterFile()
{
    reset();
}

void PhysicalRegisterFile::reset()
{
    for (std::size_t i = 0; i < 64; i++)
    {
        registers[i] = 0;
    }
}

void PhysicalRegisterFile::set(std::size_t index, uint64_t value)
{
    registers[index] = value;
}

uint64_t PhysicalRegisterFile::get(std::size_t index) const
{
    return registers[index];
}

void PhysicalRegisterFile::dump(json& j) const
{
    j["PhysicalRegisterFile"] = registers;
}