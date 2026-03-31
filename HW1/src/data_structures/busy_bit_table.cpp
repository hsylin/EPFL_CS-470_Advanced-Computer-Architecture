#include "busy_bit_table.hpp"

BusyBitTable::BusyBitTable()
{
    reset();
}

void BusyBitTable::reset()
{
    for (std::size_t i = 0; i < 64; i++)
    {
        bits[i] = false;
    }
}

void BusyBitTable::set(std::size_t index, bool value)
{
    bits[index] = value;
}

bool BusyBitTable::get(std::size_t index) const
{
    return bits[index];
}

void BusyBitTable::dump(json& j) const
{
    j["BusyBitTable"] = json::array();

    for (std::size_t i = 0; i < 64; i++)
    {
        j["BusyBitTable"].push_back(bits[i]);
    }
}