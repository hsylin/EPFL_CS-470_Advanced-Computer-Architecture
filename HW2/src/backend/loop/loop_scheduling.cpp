#include "loop_scheduling.hpp"

#include <stdexcept>
#include <string>
#include <vector>


// -----------------------------------------------------------------------------
// Helper functions for loop scheduling
// -----------------------------------------------------------------------------

static int find_valid_cycle(loop_schedule_result_t& schedule, const int start, const execution_unit_t unit) {
    int cycle = start;
    std::vector<bundle_slot_t> slots = possible_slots_for_unit(unit);

    while (!is_any_slot_empty(schedule, cycle, slots)) {
        cycle++;
    }

    ensure_loop_schedule_has_cycle(schedule, cycle);
    return cycle;
}

static void schedule_instruction(loop_schedule_result_t& schedule, const int cycle, instruction_t& instruction) {
    std::vector<bundle_slot_t> slots = possible_slots_for_unit(instruction.unit);

    for (bundle_slot_t& slot : slots) {
        if (is_slot_empty(schedule, cycle, slot)) {
            instruction.scheduled_cycle = cycle;
            put_instruction_in_loop_schedule(schedule, cycle, slot, instruction);
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
    return instuction.scheduled_cycle + instuction.latency;
}

static int find_ii_satisfying_interloop(std::vector<instruction_t>& program, const dependency_t& dependency) {
    const instruction_t producer = program[dependency.previous_iteration_producer_instruction_address];
    const instruction_t consumer = program[dependency.consumer_instruction_address];
    return producer.scheduled_cycle + producer.latency - consumer.scheduled_cycle;
}

static void increment_instructions_scheduled_cycle(
    std::vector<instruction_t>& program,
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
// Helper functions for schedule encoding
// -----------------------------------------------------------------------------

static std::string opcode_to_string(instruction_opcode_t opcode)
{
    switch (opcode)
    {
    case instruction_opcode_t::Add:
        return "add";
    case instruction_opcode_t::Addi:
        return "addi";
    case instruction_opcode_t::Sub:
        return "sub";
    case instruction_opcode_t::Mulu:
        return "mulu";
    case instruction_opcode_t::Ld:
        return "ld";
    case instruction_opcode_t::St:
        return "st";
    case instruction_opcode_t::Loop:
        return "loop";
    case instruction_opcode_t::LoopPip:
        return "loop.pip";
    case instruction_opcode_t::Nop:
        return "nop";
    case instruction_opcode_t::Mov:
        return "mov";
    default:
        break;
    }

    throw std::runtime_error("Cannot print unknown opcode.");
}

static std::string operand_to_string(operand_t op) {
    switch (op.kind)
    {
    case operand_kind_t::Register:
        return register_to_string(op.reg);
    case operand_kind_t::Memory:
        return std::to_string(op.memory.immediate) + "(" + register_to_string(op.memory.base_register) + ")";
    case operand_kind_t::Boolean:
        return op.boolean_value ? "true" : "false";
    case operand_kind_t::Immediate:
        return std::to_string(op.immediate);
    default:
        break;
    }

    throw std::runtime_error("Cannot print non existant operand.");
}

static std::string encode_instruction(instruction_t& instruction) {
    std::string encoded;
    
    if (instruction.predicate) {
        encoded = "(" + register_to_string(*instruction.predicate) + ") ";
    }

    encoded += opcode_to_string(instruction.opcode);

    for (operand_t& op : instruction.operands) {
        encoded += " " + operand_to_string(op) + ",";
    }

    if (encoded.back() == ',') encoded.pop_back(); // Remove last ','

    return encoded;
}

static bundle_t encode_bundle(std::array<instruction_t, 5> raw_bundle) {
    bundle_t encoded_bundle = {};
    for (int i = 0; i < 5; i++) {
        encoded_bundle[i] = encode_instruction(raw_bundle[i]);
    }
    return encoded_bundle;
}


// -----------------------------------------------------------------------------
// Main functions for loop scheduling
// -----------------------------------------------------------------------------

loop_schedule_result_t schedule_loop(
	const std::vector<instruction_t>& program,
	const dependency_table_t& dependency_table,
	const basic_block_info_t& block_info)
{
    std::vector<instruction_t> copy_program = program;
    loop_schedule_result_t schedule;

    const int size_of_block_0 = block_info.has_loop ? block_info.loop_start_address : copy_program.size();

    for (int i = 0; i < size_of_block_0; i++)
    {
        int scheduled_slot = 0;

        // Find the earliest cycle at which all local dependencies are satisfied
        for (const dependency_t& dependency : dependency_table.entries[i].local_dependencies) {
            scheduled_slot = max(scheduled_slot, find_earliest_satisfying_cycle(copy_program, dependency));
        }

        scheduled_slot = find_valid_cycle(schedule, scheduled_slot, copy_program[i].unit);
        schedule_instruction(schedule, scheduled_slot, copy_program[i]);
    }

    // Only schedule BB1 and BB2 if they exist
    if (block_info.has_loop) {
        loop_schedule_result_t bb1_schedule;
        int bb0_to_bb1_padding = 0;
        int initiation_interval = calculate_ii_res(copy_program, block_info);

        // Schedule all of block 1 except the loop instruction
        for (int i = block_info.loop_start_address; i < block_info.loop_instruction_address; i++) {
            int scheduled_slot = 0;

            // Find the earliest cycle at which all local dependencies are satisfied
            for (const dependency_t& dependency : dependency_table.entries[i].local_dependencies) {
                scheduled_slot = max(scheduled_slot, find_earliest_satisfying_cycle(copy_program, dependency));
            }

            scheduled_slot = find_valid_cycle(bb1_schedule, scheduled_slot, copy_program[i].unit);
            schedule_instruction(bb1_schedule, scheduled_slot, copy_program[i]);

            initiation_interval = max(initiation_interval, scheduled_slot + 1);

            // Find how many empty bundle we must add between bb0 and bb1 to satisfy loop invarient dependencies
            for (const dependency_t& dependency : dependency_table.entries[i].loop_invariant_dependencies) {
                bb0_to_bb1_padding = max(
                    bb0_to_bb1_padding,
                    find_earliest_satisfying_cycle(copy_program, dependency) - schedule.size()
                );
            }
        }

        // Update the minimum ii such that interloop dependencies are satisfied
        for (int i = block_info.loop_start_address; i < block_info.loop_instruction_address; i++) {
            for (const dependency_t& dependency : dependency_table.entries[i].interloop_dependencies) {
                initiation_interval = max(initiation_interval, find_ii_satisfying_interloop(copy_program, dependency));
                if (dependency.producer_instruction_address != -1) {
                    bb0_to_bb1_padding = max(
                        bb0_to_bb1_padding,
                        find_earliest_satisfying_cycle(copy_program, dependency) - schedule.size()
                    );
                }
            }
        }

        // Schedule loop instuction
        instruction_t& loop_instruction = copy_program[block_info.loop_instruction_address];
        loop_instruction.operands[0].immediate = schedule.size() + bb0_to_bb1_padding;
        schedule_instruction(bb1_schedule, initiation_interval - 1, loop_instruction);

        increment_instructions_scheduled_cycle(
            copy_program,
            schedule.size() + bb0_to_bb1_padding,
            block_info.loop_start_address,
            block_info.loop_instruction_address + 1
        );
        combine_loop_schedules(schedule, bb1_schedule, bb0_to_bb1_padding);



        // Schedule block 2
        loop_schedule_result_t bb2_schedule;

        for (int i = block_info.loop_instruction_address + 1; i < static_cast<int>(copy_program.size()); i++)
        {
            int scheduled_slot = 0;

            // Find the earliest cycle at which all local and post-loop dependencies are satisfied
            for (const dependency_t& dependency : dependency_table.entries[i].local_dependencies) {
                scheduled_slot = max(scheduled_slot, find_earliest_satisfying_cycle(copy_program, dependency));
            }
            for (const dependency_t& dependency : dependency_table.entries[i].post_loop_dependencies) {
                scheduled_slot = max(scheduled_slot, find_earliest_satisfying_cycle(copy_program, dependency) - schedule.size());
            }

            scheduled_slot = find_valid_cycle(bb2_schedule, scheduled_slot, copy_program[i].unit);
            schedule_instruction(bb2_schedule, scheduled_slot, copy_program[i]);
        }

        increment_instructions_scheduled_cycle(
            copy_program,
            schedule.size(),
            block_info.loop_instruction_address + 1,
            copy_program.size()
        );
        combine_loop_schedules(schedule, bb2_schedule);
    }

    return schedule;
}



// -----------------------------------------------------------------------------
// Main functions for loop scheduling
// -----------------------------------------------------------------------------


schedule_t encode_schedule(const loop_schedule_result_t& schedule) {
    schedule_t final_schedule;
    for (auto& bundle : schedule) {
        final_schedule.push_back(encode_bundle(bundle));
    }
    
    return final_schedule;
}