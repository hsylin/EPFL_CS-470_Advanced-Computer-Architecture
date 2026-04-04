#pragma once

#include <cstddef>
#include <vector>
#include <nlohmann/json.hpp>
#include "instruction.hpp"

using json = nlohmann::json;

class DecodedInstructionRegister
{
public:
    DecodedInstructionRegister();

    void add(instruction_decode_t instruction);
    void reset();
    std::size_t size() const;
    void dump(json& j) const;

    auto begin() const { return buffer.begin(); }
    auto end() const { return buffer.end(); }

private:
    std::vector<instruction_decode_t> buffer;
};