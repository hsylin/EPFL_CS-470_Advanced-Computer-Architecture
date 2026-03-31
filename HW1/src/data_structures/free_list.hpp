#pragma once

#include <cstddef>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class FreeList
{
public:
    FreeList();

    void reset();

    // FIFO push to the back
    bool push(int phys_reg);

    // FIFO pop from the front
    std::optional<int> pop();

    bool empty() const;
    bool full() const;
    std::size_t size() const;

    void dump(json& j) const;

private:
    static constexpr std::size_t CAPACITY = 32;

    int data[CAPACITY];
    std::size_t head;
    std::size_t tail;
    std::size_t count;
};