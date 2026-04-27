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
            instruction.scheduled_cycle = cycle;
            return;
        }
    }

    throw std::runtime_error(
        "Trying to schedule instruction with no available slot."
    );
}

static int find_earliest_satisfying_cycle(
    const std::vector<instruction_t>& program, 
    const dependency_t& dependency
) 
{
    const instruction_t instuction = program[dependency.producer_instruction_address];
    return instuction.scheduled_cycle + instuction.latency
}

static int find_ii_satisfying_interloop(const std::vector<instruction_t>& program, const dependency_t& dependency) {
    const instruction_t producer = program[dependency.previous_iteration_producer_instruction_address];
    const instruction_t consumer = program[dependency.consumer_instruction_address];
    return producer.scheduled_cycle + producer.latency - consumer.scheduled_cycle;
}

static void increment_instructions_scheduled_cycle(
    const std::vector<instruction_t>& program,
    const int value,
    const int start_index,
    const int end_index 
)
{
    for (int i = start_index; i < end_index; i++) {
        program[i].scheduled_cycle += value;
    }
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
            scheduled_slot = fmax(scheduled_slot, find_earliest_satisfying_cycle(program, dependency));
        }

        scheduled_slot = find_valid_cycle(schedule, scheduled_slot, program[i].unit);
        schedule_instruction(schedule, scheduled_slot, program[i]);
    }

    // Only schedule BB1 and BB2 if they exist
    if (block_info.has_loop) {
        schedule_t bb1_schedule;
        int bb0_to_bb1_padding = 0;
        int initiation_interval = block_info.loop_instruction_address - block_info.loop_start_address;

        // Schedule all of block 1 except the loop instruction
        for (int i = block_info.loop_start_address; i < block_info.loop_instruction_address) {
            int scheduled_slot = 0;

            // Find the earliest cycle at which all local dependencies are satisfied
            for (dependency_t dependency : dependency_table.entries[i].local_dependencies) {
                scheduled_slot = fmax(scheduled_slot, find_earliest_satisfying_cycle(program, dependency));
            }

            scheduled_slot = find_valid_cycle(bb1_schedule, scheduled_slot, program[i].unit);
            schedule_instruction(bb1_schedule, scheduled_slot, program[i]);

            // Find how many empty bundle we must add between bb0 and bb1 to satisfy loop invarient dependencies
            for (dependency_t dependency : dependency_table.entries[i].loop_invariant_dependencies) {
                bb0_to_bb1_padding = fmax(bb0_to_bb1_padding, find_earliest_satisfying_cycle(program, dependency));
            }

            // Update the minimum ii such that interloop dependencies are satisfied
            for (dependency_t dependency : dependency_table.entries[i].interloop_dependencies) {
                initiation_interval = fmax(initiation_interval, find_ii_satisfying_interloop(program, dependency));
            }
        }

        // Schedule loop instuction
        schedule_instruction(bb1_schedule, initiation_interval, program[block_info.loop_instruction_address]);

        increment_instructions_scheduled_cycle(
            program,
            initiation_interval,
            block_info.loop_start_address,
            block_info.loop_instruction_address + 1
        );
        combine_schedules(schedule, bb1_schedule, initiation_interval);



        // Schedule block 2
        schedule_t bb2_schedule;

        for (int i = 0; i < size_of_block_0; i++)
        {
            int scheduled_slot = 0;

            // Find the earliest cycle at which all local and post-loop dependencies are satisfied
            for (dependency_t dependency : dependency_table.entries[i].local_dependencies) {
                scheduled_slot = fmax(scheduled_slot, find_earliest_satisfying_cycle(program, dependency));
            }
            for (dependency_t dependency : dependency_table.entries[i].post_loop_dependencies) {
                scheduled_slot = fmax(scheduled_slot, find_earliest_satisfying_cycle(program, dependency) - schedule.size());
            }

            scheduled_slot = find_valid_cycle(schedule, scheduled_slot, program[i].unit);
            schedule_instruction(schedule, scheduled_slot, program[i]);
        }

        increment_instructions_scheduled_cycle(
            program,
            initiation_interval,
            block_info.loop_instruction_address + 1,
            program.size()
        );
        combine_schedules(schedule, bb2_schedule);
    }

    return schedule;
}