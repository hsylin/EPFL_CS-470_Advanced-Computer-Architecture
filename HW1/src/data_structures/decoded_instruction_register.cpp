#include "decoded_instruction_register.hpp"

DecodedInstructionRegister::DecodedInstructionRegister()
{
    buffer.clear();
}

void DecodedInstructionRegister::add(instruction_decode_t instruction)
{
    buffer.push_back(instruction);
}

void DecodedInstructionRegister::reset()
{
    buffer.clear();
}

std::size_t DecodedInstructionRegister::size() const
{
    return buffer.size();
}

void DecodedInstructionRegister::dump(json& j) const
{
    j["DecodedPCs"] = json::array();

    for (const auto& inst : buffer)
    {
        j["DecodedPCs"].push_back(inst.pc);
    }
}