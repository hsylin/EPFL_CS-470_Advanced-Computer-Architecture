#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "instruction.hpp"
using program_t = std::vector<std::string>;


class instruction_memory
{
public:
    explicit instruction_memory(program_t program)
        : program_(std::move(program)) {}

    bool has_instruction(std::uint64_t pc) const
    {
        return pc < program_.size();
    }

    const std::string& instruction_at(std::uint64_t pc) const
    {
        return program_.at(pc);
    }

    std::size_t size() const
    {
        return program_.size();
    }

private:
    program_t program_;
};