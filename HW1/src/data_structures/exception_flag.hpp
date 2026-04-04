#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ExceptionFlag
{
public:
    ExceptionFlag();

    void set_exception_mode(bool mode);
    bool is_exception_mode() const;


    void reset();
    void dump(json& j) const;

private:
    bool exception_mode;
};

