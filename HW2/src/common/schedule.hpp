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

inline void combine_schedules(schedule_t& schedule1, schedule_t& schedule2, int padding = 0) 
{
    padding += schedule1.size();
    ensure_schedule_has_cycle(schedule1, padding);

    for (bundle_t bundle : schedule2) {
        schedule1.push_back(bundle);
    }
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








// -----------------------------------------------------------------------------
// Intermediate scheduling result for loopip
// -----------------------------------------------------------------------------
//
// schedule_t is the final output format used by schedule_writer.
// scheduled_program_t is an internal backend representation.
//
// The scheduler only decides where each original instruction is placed:
//   instruction_address -> cycle / stage / slot
//
// Register allocation later uses this placement information to generate
// the final instruction strings stored in schedule_t.
//

struct scheduled_instruction_t
{
    int instruction_address = -1;
    int cycle = -1;
    int stage = -1;

    bundle_slot_t slot = bundle_slot_t::ALU0;
};

struct scheduled_program_t
{
    std::vector<scheduled_instruction_t> scheduled_instructions;
};



// -----------------------------------------------------------------------------
// Helper functions for scheduled_program_t
// -----------------------------------------------------------------------------

inline void add_scheduled_instruction(
    scheduled_program_t& scheduled_program,
    int instruction_address,
    int cycle,
    bundle_slot_t slot,
    int stage = -1
)
{
    if (instruction_address < 0)
    {
        throw std::runtime_error("Instruction address cannot be negative.");
    }

    if (cycle < 0)
    {
        throw std::runtime_error("Scheduled cycle cannot be negative.");
    }

    scheduled_instruction_t scheduled_instruction;
    scheduled_instruction.instruction_address = instruction_address;
    scheduled_instruction.cycle = cycle;
    scheduled_instruction.stage = stage;
    scheduled_instruction.slot = slot;

    scheduled_program.scheduled_instructions.push_back(scheduled_instruction);
}

inline const scheduled_instruction_t* find_scheduled_instruction(
    const scheduled_program_t& scheduled_program,
    int instruction_address
)
{
    for (const scheduled_instruction_t& scheduled_instruction :
         scheduled_program.scheduled_instructions)
    {
        if (scheduled_instruction.instruction_address == instruction_address)
        {
            return &scheduled_instruction;
        }
    }

    return nullptr;
}