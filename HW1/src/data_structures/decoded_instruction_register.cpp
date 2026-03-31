#include "decoded_instruction_register.hpp"

DecodedInstructionRegister::DecodedInstructionRegister()
{
    buffer.clear();
}

void DecodedInstructionRegister::add(unsigned int pc)
{
    buffer.push_back(pc);
}

void DecodedInstructionRegister::remove()
{
    if (!buffer.empty())
    {
        buffer.erase(buffer.begin());
    }
}

void DecodedInstructionRegister::reset()
{
    buffer.clear();
}

void DecodedInstructionRegister::dump(json& j) const
{
    j["DecodedPCs"] = buffer;
}