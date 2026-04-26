#include "loop_scheduling.hpp"

#include <stdexcept>
#include <string>
#include <vector>


// -----------------------------------------------------------------------------
// Helper functions for dependency analysis
// -----------------------------------------------------------------------------

static int find_valid_cycle(const schedule_t& schedule, const int start, const execution_unit_t unit) {
    int cycle = start;
    std::vector<bundle_slot_t> slots = possible_slots_for_unit(unit);

    while (!is_any_schedule_slot_empty(schedule, cycle, slots)) {
        cycle++;
    }

    ensure_schedule_has_cycle(cycle);
    return cycle;
}

static void schedule_instruction(schedule_t& schedule, const int cycle, const instruction_t& instruction) {
    std::vector<bundle_slot_t> slots = possible_slots_for_unit(instruction.unit);

    for (bundle_slot_t slot : slots) {
        if (is_schedule_slot_empty(schedule, cycle, slot)) {
            put_instruction_in_schedule(schedule, cycle, slot, instruction.raw);
            return;
        }
    }

    throw std::runtime_error(
        "Trying to schedule instruction with no available slot."
    );
}


// -----------------------------------------------------------------------------
// Main functions for dependency analysis
// -----------------------------------------------------------------------------

schedule_t schedule_loop(
	const std::vector<instruction_t>& program,
	const dependency_table_t& dependency_table,
	const basic_block_info_t& block_info)
{
    schedule_t schedule;

    const int size_of_block_0 = block_info.has_loop ? block_info.loop_start_address : program.size();

    for (int i = 0; i < size_of_block_0; i++)
    {
        int scheduled_slot = 0;

        // Find the earliest cycle at which all local dependencies are satisfied
        for (dependency_t dependency : dependency_table.entries[i].local_dependencies) {
            scheduled_slot = fmax(scheduled_slot, program[dependency.producer_instruction_address].scheduled_cycle);
        }

        scheduled_slot = find_valid_cycle(schedule, scheduled_slot, program[i].unit);
        schedule_instruction(schedule, scheduled_slot, program[i]);
    }

    // TODO: similar for block 1 if has_loop (if not return here), add empty slot between both for loop invarient dependencies, 
    // add empty slot at the end for interloop dependancy, put loop instruction on the last of those, 
    // make the same for block 2 as block 0, add empty slots between 1 and 2 for post loop dependencies
}