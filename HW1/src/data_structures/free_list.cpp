#include "free_list.hpp"

FreeList::FreeList()
{
    reset();
}

void FreeList::reset()
{
    head = 0;
    tail = 0;
    count = 0;

    for (int reg = 32; reg <= 63; reg++)
    {
        push(reg);
    }
}

bool FreeList::push(int phys_reg)
{
    if (full())
    {
        return false;
    }

    data[tail] = phys_reg;
    tail = (tail + 1) % CAPACITY;
    count++;
    return true;
}

std::optional<int> FreeList::pop()
{
    if (empty())
    {
        return std::nullopt;
    }

    int value = data[head];
    head = (head + 1) % CAPACITY;
    count--;
    return value;
}

bool FreeList::empty() const
{
    return count == 0;
}

bool FreeList::full() const
{
    return count == CAPACITY;
}

std::size_t FreeList::size() const
{
    return count;
}

void FreeList::dump(json& j) const
{
    j["FreeList"] = json::array();

    for (std::size_t i = 0; i < count; i++)
    {
        std::size_t idx = (head + i) % CAPACITY;
        j["FreeList"].push_back(data[idx]);
    }
}