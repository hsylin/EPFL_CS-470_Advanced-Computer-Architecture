#include "exception_program_counter.hpp"

ExceptionPC::ExceptionPC()
{
    value = 0;
}

void ExceptionPC::set(unsigned int new_value)
{
    value = new_value;
}

unsigned int ExceptionPC::get() const
{
    return value;
}

void ExceptionPC::reset()
{
    value = 0;
}

void ExceptionPC::dump(json& j) const
{
    j["ExceptionPC"] = value;
}