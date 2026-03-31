#include "program_counter.hpp"

ProgramCounter::ProgramCounter()
{
    this->value = 0;
}

ProgramCounter::~ProgramCounter()
{
    this->value = 0;
}

void ProgramCounter::reset()
{
    value = 0;
}

void ProgramCounter::increment()
{
    this->value++;
}

void ProgramCounter::set(unsigned int value)
{
    this->value = value;
}

unsigned int ProgramCounter::get() const
{
    return this->value;
}

void ProgramCounter::dump(json& j) const
{
    j["PC"] = this->value;
}