#include "exception_flag.hpp"

ExceptionFlag::ExceptionFlag()
{
    exception_mode = false;
}

void ExceptionFlag::set_exception_mode(bool mode)
{
    exception_mode = mode;
}

bool ExceptionFlag::get() const
{
    return exception_mode;
}

void ExceptionFlag::reset()
{
    exception_mode = false;
}

void ExceptionFlag::dump(json& j) const
{
    j["Exception"] = exception_mode;
}