#pragma once

#include <cstddef>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ActiveListEntry
{
    bool done;
    bool exception;
    unsigned int logical_destination;
    unsigned int old_destination;
    unsigned int pc;
};

class ActiveList
{
public:
    ActiveList();

    void reset();
    bool full() const;
    bool empty() const;
    std::size_t size() const;
    std::size_t available_slots() const;

    bool push_back(const ActiveListEntry& entry);

    std::optional<ActiveListEntry> front() const;
    std::optional<ActiveListEntry> back() const;

    bool pop_front();
    bool pop_back();

    // Both overloads are needed to support complete container-style access:
    // writable access for non-const objects, and read-only access for const objects.
    ActiveListEntry& at(std::size_t logical_index);
    const ActiveListEntry& at(std::size_t logical_index) const;

    void dump(json& j) const;

private:
    static constexpr std::size_t CAPACITY = 32;

    ActiveListEntry data[CAPACITY];
    std::size_t head;
    std::size_t tail;
    std::size_t count;
};