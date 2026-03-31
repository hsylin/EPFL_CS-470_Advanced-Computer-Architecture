#include "register_map_table.hpp"

register_map_table::register_map_table()
{
    reset();
}

void register_map_table::reset()
{
    for (int i = 0; i < 32; i++)
    {
        reg_map[i] = i;
    }
}

int register_map_table::get_physical_register(int arch_reg) const
{
    return reg_map[arch_reg];
}

void register_map_table::set_physical_register(int arch_reg, int phys_reg)
{
    reg_map[arch_reg] = phys_reg;
}

void register_map_table::dump(json& j) const
{
    j["RegisterMapTable"] = json::array();

    for (int i = 0; i < 32; i++)
    {
        j["RegisterMapTable"].push_back(reg_map[i]);
    }
}