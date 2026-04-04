#include "register_map_table.hpp"

RegisterMapTable::RegisterMapTable()
{
    reset();
}

void RegisterMapTable::reset()
{
    for (int i = 0; i < 32; i++)
    {
        reg_map[i] = i;
    }
}

int RegisterMapTable::get_physical_register(int arch_reg) const
{
    return reg_map[arch_reg];
}

void RegisterMapTable::set_physical_register(int arch_reg, int phys_reg)
{
    reg_map[arch_reg] = phys_reg;
}

void RegisterMapTable::dump(json& j) const
{
    j["RegisterMapTable"] = json::array();

    for (int i = 0; i < 32; i++)
    {
        j["RegisterMapTable"].push_back(reg_map[i]);
    }
}