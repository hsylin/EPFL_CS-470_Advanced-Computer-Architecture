#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ExceptionPC
{
public:
    ExceptionPC();

    void set(unsigned int value);
    unsigned int get() const;
    void reset();
    void dump(json& j) const;

private:
    unsigned int value;
};