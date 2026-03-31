#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ProgramCounter 
{
public:
    ProgramCounter();
    ~ProgramCounter();

    void reset();
    void increment();
    void set(unsigned int value);
    unsigned int get() const;

    void dump(json& j) const;

private:
    unsigned int value;
};