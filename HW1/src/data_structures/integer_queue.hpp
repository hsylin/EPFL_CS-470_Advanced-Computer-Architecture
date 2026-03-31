#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct IntegerQueueEntry
{
    unsigned int dest_register;

    bool op_a_is_ready;
    unsigned int op_a_reg_tag;
    std::optional<uint64_t> op_a_value;

    bool op_b_is_ready;
    unsigned int op_b_reg_tag;
    std::optional<uint64_t> op_b_value;

    std::string op_code;
    unsigned int pc;
};

class IntegerQueue
{
public:
    IntegerQueue();

    void reset();

    bool empty() const;
    bool full() const;
    std::size_t size() const;
    std::size_t available_slots() const;

    bool push_back(const IntegerQueueEntry& entry);

    IntegerQueueEntry& at(std::size_t index);
    const IntegerQueueEntry& at(std::size_t index) const;

    bool erase_at(std::size_t index);

    void dump(json& j) const;

private:
    static constexpr std::size_t CAPACITY = 32;

    IntegerQueueEntry entries[CAPACITY];
    std::size_t count;
};