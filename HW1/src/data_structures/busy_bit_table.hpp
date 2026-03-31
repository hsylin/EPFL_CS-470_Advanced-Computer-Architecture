#pragma once

#include <cstddef>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class BusyBitTable
{
public:
    BusyBitTable();

    void reset();

    void set(std::size_t index, bool value);
    bool get(std::size_t index) const;

    void dump(json& j) const;

private:
    bool bits[64];
};