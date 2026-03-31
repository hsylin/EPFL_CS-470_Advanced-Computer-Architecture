#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class RegisterMapTable
{
public:
    RegisterMapTable();
    ~RegisterMapTable() = default;

    int get_physical_register(int arch_reg) const;
    void set_physical_register(int arch_reg, int phys_reg);
    void reset();
    void dump(json& j) const;

private:
    int reg_map[32];
};