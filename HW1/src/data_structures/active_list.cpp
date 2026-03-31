#include "active_list.hpp"

ActiveList::ActiveList()
{
    reset();
}

void ActiveList::reset()
{
    head = 0;
    tail = 0;
    count = 0;
}

bool ActiveList::empty() const
{
    return count == 0;
}

bool ActiveList::full() const
{
    return count == CAPACITY;
}

std::size_t ActiveList::size() const
{
    return count;
}

std::size_t ActiveList::available_slots() const
{
    return CAPACITY - count;
}

bool ActiveList::push_back(const ActiveListEntry& entry)
{
    if (full())
    {
        return false;
    }

    data[tail] = entry;
    tail = (tail + 1) % CAPACITY;
    count++;
    return true;
}

std::optional<ActiveListEntry> ActiveList::front() const
{
    if (empty())
    {
        return std::nullopt;
    }

    return data[head];
}

std::optional<ActiveListEntry> ActiveList::back() const
{
    if (empty())
    {
        return std::nullopt;
    }

    std::size_t idx = (tail + CAPACITY - 1) % CAPACITY;
    return data[idx];
}

bool ActiveList::pop_front()
{
    if (empty())
    {
        return false;
    }

    head = (head + 1) % CAPACITY;
    count--;
    return true;
}

bool ActiveList::pop_back()
{
    if (empty())
    {
        return false;
    }

    tail = (tail + CAPACITY - 1) % CAPACITY;
    count--;
    return true;
}

ActiveListEntry& ActiveList::at(std::size_t logical_index)
{
    std::size_t idx = (head + logical_index) % CAPACITY;
    return data[idx];
}

const ActiveListEntry& ActiveList::at(std::size_t logical_index) const
{
    std::size_t idx = (head + logical_index) % CAPACITY;
    return data[idx];
}

void ActiveList::dump(json& j) const
{
    j["ActiveList"] = json::array();

    for (std::size_t i = 0; i < count; i++)
    {
        const auto& entry = at(i);

        j["ActiveList"].push_back({
            {"Done", entry.done},
            {"Exception", entry.exception},
            {"LogicalDestination", entry.logical_destination},
            {"OldDestination", entry.old_destination},
            {"PC", entry.pc}
        });
    }
}