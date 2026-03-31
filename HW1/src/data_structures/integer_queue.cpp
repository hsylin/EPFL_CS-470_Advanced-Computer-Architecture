#include "integer_queue.hpp"

IntegerQueue::IntegerQueue()
{
    reset();
}

void IntegerQueue::reset()
{
    count = 0;
}

bool IntegerQueue::empty() const
{
    return count == 0;
}

bool IntegerQueue::full() const
{
    return count == CAPACITY;
}

std::size_t IntegerQueue::size() const
{
    return count;
}

std::size_t IntegerQueue::available_slots() const
{
    return CAPACITY - count;
}

bool IntegerQueue::push_back(const IntegerQueueEntry& entry)
{
    if (full())
    {
        return false;
    }

    entries[count] = entry;
    count++;
    return true;
}

IntegerQueueEntry& IntegerQueue::at(std::size_t index)
{
    return entries[index];
}

const IntegerQueueEntry& IntegerQueue::at(std::size_t index) const
{
    return entries[index];
}

bool IntegerQueue::erase_at(std::size_t index)
{
    if (index >= count)
    {
        return false;
    }

    for (std::size_t i = index; i + 1 < count; i++)
    {
        entries[i] = entries[i + 1];
    }

    count--;
    return true;
}

void IntegerQueue::dump(json& j) const
{
    j["IntegerQueue"] = json::array();

    for (std::size_t i = 0; i < count; i++)
    {
        const auto& entry = entries[i];

        json obj;
        obj["DestRegister"] = entry.dest_register;

        obj["OpAIsReady"] = entry.op_a_is_ready;
        obj["OpARegTag"] = entry.op_a_reg_tag;
        obj["OpAValue"] = entry.op_a_value.has_value()
                            ? json(*entry.op_a_value)
                            : json(nullptr);

        obj["OpBIsReady"] = entry.op_b_is_ready;
        obj["OpBRegTag"] = entry.op_b_reg_tag;
        obj["OpBValue"] = entry.op_b_value.has_value()
                            ? json(*entry.op_b_value)
                            : json(nullptr);

        obj["OpCode"] = entry.op_code;
        obj["PC"] = entry.pc;

        j["IntegerQueue"].push_back(obj);
    }
}