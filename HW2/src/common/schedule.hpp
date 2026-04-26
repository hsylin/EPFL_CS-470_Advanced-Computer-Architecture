#pragma once

#include "instruction.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <vector>




enum class bundle_slot_t
{
    ALU0 = 0,
    ALU1 = 1,
    Mult = 2,
    Mem = 3,
    Branch = 4
};

using bundle_t = std::array<std::string, 5>;
using schedule_t = std::vector<bundle_t>;

// -----------------------------------------------------------------------------
// Helper functions for bundle manipulation
// -----------------------------------------------------------------------------

inline bundle_t make_empty_bundle()
{
    return {"nop", "nop", "nop", "nop", "nop"};
}

inline int bundle_slot_to_index(bundle_slot_t slot)
{
    return static_cast<int>(slot);
}

inline bool is_bundle_slot_empty(
    const bundle_t& bundle,
    bundle_slot_t slot
)
{
    return bundle[bundle_slot_to_index(slot)] == "nop";
}

inline void put_instruction_in_bundle_slot(
    bundle_t& bundle,
    bundle_slot_t slot,
    const std::string& instruction_text
)
{
    const int index = bundle_slot_to_index(slot);

    if (bundle[index] != "nop")
    {
        throw std::runtime_error(
            "Trying to put instruction into a non-empty bundle slot."
        );
    }

    bundle[index] = instruction_text;
}

// -----------------------------------------------------------------------------
// Helper functions for schedule manipulation
// -----------------------------------------------------------------------------

inline void ensure_schedule_has_cycle(schedule_t& schedule, int cycle)
{
    if (cycle < 0)
    {
        throw std::runtime_error("Schedule cycle cannot be negative.");
    }

    while (static_cast<int>(schedule.size()) <= cycle)
    {
        schedule.push_back(make_empty_bundle());
    }
}

inline bool is_schedule_slot_empty(
    const schedule_t& schedule,
    int cycle,
    bundle_slot_t slot
)
{
    if (cycle < 0)
    {
        throw std::runtime_error("Schedule cycle cannot be negative.");
    }


    if (cycle >= static_cast<int>(schedule.size()))
    {
        return true;
    }

    return is_bundle_slot_empty(schedule[cycle], slot);
}

inline bool is_any_schedule_slot_empty(
    const schedule_t& schedule,
    int cycle,
    std::vector<bundle_slot_t> slots
)
{
    for (bundle_slot_t slot : slots) {
        if (is_schedule_slot_empty(schedule, cycle, slot)) return true;
    }

    return false;
}

inline void put_instruction_in_schedule(
    schedule_t& schedule,
    int cycle,
    bundle_slot_t slot,
    const std::string& instruction_text
)
{
    ensure_schedule_has_cycle(schedule, cycle);

    put_instruction_in_bundle_slot(
        schedule[cycle],
        slot,
        instruction_text
    );
}

// -----------------------------------------------------------------------------
// Execution-unit of instruction to bundle-slot mapping
// -----------------------------------------------------------------------------

inline std::vector<bundle_slot_t> possible_slots_for_unit(
    execution_unit_t unit
)
{
    switch (unit)
    {
        case execution_unit_t::ALU:
            return {bundle_slot_t::ALU0, bundle_slot_t::ALU1};

        case execution_unit_t::Mult:
            return {bundle_slot_t::Mult};

        case execution_unit_t::Mem:
            return {bundle_slot_t::Mem};

        case execution_unit_t::Branch:
            return {bundle_slot_t::Branch};

        case execution_unit_t::None:
            return {};
    }

    return {};
}