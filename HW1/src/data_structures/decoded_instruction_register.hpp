#pragma once

#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class DecodedInstructionRegister
{
public:
    DecodedInstructionRegister();

    void add(unsigned int pc);
    void remove();
    void reset();
    void dump(json& j) const;

private:
    std::vector<unsigned int> buffer;
};