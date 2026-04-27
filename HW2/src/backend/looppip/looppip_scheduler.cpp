#include "looppip_scheduler.hpp"
#include "ii_calculator.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// Internal helper structures
// -----------------------------------------------------------------------------

// For BB0 + BB1 + BB2 absolute schedule.
struct absolute_schedule_state_t
{
    scheduled_program_t scheduled_program;

    std::vector<int> cycle_by_address;
    std::vector<bundle_slot_t> slot_by_address;

    schedule_t occupancy;

    int last_cycle = -1;
};

// For BB1 relative modulo schedule.
struct loop_body_candidate_t
{
    std::vector<int> relative_cycle_by_address;
    std::vector<bundle_slot_t> slot_by_address;
    std::vector<scheduled_instruction_t> relative_placements;

    int num_stages = 0;
};

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------

static const instruction_t& get_instruction(
    const std::vector<instruction_t>& program,
    int address
)
{
    if (address < 0 || address >= static_cast<int>(program.size()))
    {
        throw std::runtime_error(
            "Invalid instruction address: " + std::to_string(address)
        );
    }

    return program[address];
}

static const instruction_dependency_info_t& get_dependency_info(
    const dependency_table_t& dependency_table,
    int address
)
{
    if (address < 0 ||
        address >= static_cast<int>(dependency_table.entries.size()))
    {
        throw std::runtime_error(
            "Invalid dependency table address: " + std::to_string(address)
        );
    }

    return dependency_table.entries[address];
}

static bool is_loop_body_instruction(const instruction_t& instruction)
{
    return instruction.block == basic_block_t::BB1;
}

static bool is_loop_control_instruction(const instruction_t& instruction)
{
    return instruction.opcode == instruction_opcode_t::Loop ||
           instruction.opcode == instruction_opcode_t::LoopPip;
}

static int instruction_latency(
    const std::vector<instruction_t>& program,
    int instruction_address
)
{
    return get_instruction(program, instruction_address).latency;
}

static int find_loop_control_instruction_address(
    const std::vector<instruction_t>& program
)
{
    for (const instruction_t& instruction : program)
    {
        if (instruction.block == basic_block_t::BB1 &&
            is_loop_control_instruction(instruction))
        {
            return instruction.instruction_address;
        }
    }

    return -1;
}

// -----------------------------------------------------------------------------
// Schedule occupancy helpers
// -----------------------------------------------------------------------------

static bool is_absolute_slot_empty(
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

    const int index = bundle_slot_to_index(slot);
    return schedule[cycle][index] == "nop";
}

static void mark_absolute_slot_occupied(
    absolute_schedule_state_t& state,
    int instruction_address,
    int cycle,
    bundle_slot_t slot,
    int stage
)
{
    put_instruction_in_schedule(
        state.occupancy,
        cycle,
        slot,
        "occupied"
    );

    state.cycle_by_address[instruction_address] = cycle;
    state.slot_by_address[instruction_address] = slot;
    state.last_cycle = std::max(state.last_cycle, cycle);

    add_scheduled_instruction(
        state.scheduled_program,
        instruction_address,
        cycle,
        slot,
        stage
    );
}

static bool try_find_absolute_slot(
    const instruction_t& instruction,
    const schedule_t& occupancy,
    int earliest_cycle,
    int& selected_cycle,
    bundle_slot_t& selected_slot
)
{
    const std::vector<bundle_slot_t> possible_slots =
        possible_slots_for_unit(instruction.unit);

    if (possible_slots.empty())
    {
        return false;
    }

    const int max_search_cycle = earliest_cycle + 4096;

    for (int cycle = earliest_cycle; cycle <= max_search_cycle; cycle++)
    {
        for (bundle_slot_t slot : possible_slots)
        {
            if (is_absolute_slot_empty(occupancy, cycle, slot))
            {
                selected_cycle = cycle;
                selected_slot = slot;
                return true;
            }
        }
    }

    return false;
}

// -----------------------------------------------------------------------------
// Dependency handling for BB0 / BB2 absolute scheduling
// -----------------------------------------------------------------------------

static void apply_absolute_dependency_list(
    const std::vector<instruction_t>& program,
    const absolute_schedule_state_t& state,
    const std::vector<dependency_t>& dependencies,
    int& earliest_cycle
)
{
    for (const dependency_t& dependency : dependencies)
    {
        const int producer_address =
            dependency.producer_instruction_address;

        if (producer_address == -1)
        {
            continue;
        }

        const int producer_cycle =
            state.cycle_by_address[producer_address];

        if (producer_cycle == -1)
        {
            throw std::runtime_error(
                "Producer instruction was not scheduled before consumer."
            );
        }

        earliest_cycle = std::max(
            earliest_cycle,
            producer_cycle + instruction_latency(program, producer_address)
        );
    }
}

static int calculate_absolute_earliest_cycle(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const absolute_schedule_state_t& state,
    const instruction_t& instruction,
    int minimum_cycle
)
{
    int earliest_cycle = minimum_cycle;

    const instruction_dependency_info_t& dependency_info =
        get_dependency_info(
            dependency_table,
            instruction.instruction_address
        );

    apply_absolute_dependency_list(
        program,
        state,
        dependency_info.local_dependencies,
        earliest_cycle
    );

    apply_absolute_dependency_list(
        program,
        state,
        dependency_info.loop_invariant_dependencies,
        earliest_cycle
    );

    apply_absolute_dependency_list(
        program,
        state,
        dependency_info.post_loop_dependencies,
        earliest_cycle
    );

    return earliest_cycle;
}

static void schedule_absolute_block(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    basic_block_t block,
    int minimum_cycle,
    absolute_schedule_state_t& state
)
{
    for (const instruction_t& instruction : program)
    {
        if (instruction.block != block)
        {
            continue;
        }

        const int earliest_cycle =
            calculate_absolute_earliest_cycle(
                program,
                dependency_table,
                state,
                instruction,
                minimum_cycle
            );

        int selected_cycle = -1;
        bundle_slot_t selected_slot = bundle_slot_t::ALU0;

        const bool found_slot =
            try_find_absolute_slot(
                instruction,
                state.occupancy,
                earliest_cycle,
                selected_cycle,
                selected_slot
            );

        if (!found_slot)
        {
            throw std::runtime_error(
                "Failed to find absolute schedule slot."
            );
        }

        mark_absolute_slot_occupied(
            state,
            instruction.instruction_address,
            selected_cycle,
            selected_slot,
            -1
        );
    }
}

// -----------------------------------------------------------------------------
// Dependency handling for BB1 loop-body scheduling
// -----------------------------------------------------------------------------

static void apply_loop_body_local_dependencies(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const loop_body_candidate_t& candidate,
    const instruction_t& instruction,
    int& earliest_relative_cycle
)
{
    const instruction_dependency_info_t& dependency_info =
        get_dependency_info(
            dependency_table,
            instruction.instruction_address
        );

    for (const dependency_t& dependency :
         dependency_info.local_dependencies)
    {
        const int producer_address =
            dependency.producer_instruction_address;

        if (producer_address == -1)
        {
            continue;
        }

        const instruction_t& producer =
            get_instruction(program, producer_address);

        if (producer.block != basic_block_t::BB1)
        {
            continue;
        }

        const int producer_relative_cycle =
            candidate.relative_cycle_by_address[producer_address];

        if (producer_relative_cycle == -1)
        {
            throw std::runtime_error(
                "Local loop-body producer was not scheduled."
            );
        }

        earliest_relative_cycle = std::max(
            earliest_relative_cycle,
            producer_relative_cycle + producer.latency
        );
    }
}

static bool apply_known_interloop_constraints(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const loop_body_candidate_t& candidate,
    const instruction_t& instruction,
    int ii,
    int& earliest_relative_cycle
)
{
    const instruction_dependency_info_t& dependency_info =
        get_dependency_info(
            dependency_table,
            instruction.instruction_address
        );

    for (const dependency_t& dependency :
         dependency_info.interloop_dependencies)
    {
        const int previous_iteration_producer =
            dependency.previous_iteration_producer_instruction_address;

        if (previous_iteration_producer == -1)
        {
            continue;
        }

        const instruction_t& producer =
            get_instruction(program, previous_iteration_producer);

        // Self loop-carried dependency:
        //
        //   S(P) + latency(P) <= S(C) + II
        //
        // If P == C, then latency(P) <= II.
        if (previous_iteration_producer == instruction.instruction_address)
        {
            if (producer.latency > ii)
            {
                return false;
            }

            continue;
        }

        const int producer_relative_cycle =
            candidate.relative_cycle_by_address[previous_iteration_producer];

        // If the previous-iteration producer has not been scheduled yet,
        // we cannot check it now. It will be validated after all BB1
        // instructions are scheduled.
        if (producer_relative_cycle == -1)
        {
            continue;
        }

        // Equation 2:
        //   S(P) + latency(P) <= S(C) + II
        // Therefore:
        //   S(C) >= S(P) + latency(P) - II
        earliest_relative_cycle = std::max(
            earliest_relative_cycle,
            producer_relative_cycle + producer.latency - ii
        );
    }

    return true;
}

static bool validate_all_interloop_dependencies(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const loop_body_candidate_t& candidate,
    int ii
)
{
    for (const instruction_t& consumer : program)
    {
        if (!is_loop_body_instruction(consumer) ||
            is_loop_control_instruction(consumer))
        {
            continue;
        }

        const instruction_dependency_info_t& dependency_info =
            get_dependency_info(
                dependency_table,
                consumer.instruction_address
            );

        const int consumer_relative_cycle =
            candidate.relative_cycle_by_address[
                consumer.instruction_address
            ];

        if (consumer_relative_cycle == -1)
        {
            return false;
        }

        for (const dependency_t& dependency :
             dependency_info.interloop_dependencies)
        {
            const int previous_iteration_producer =
                dependency.previous_iteration_producer_instruction_address;

            if (previous_iteration_producer == -1)
            {
                continue;
            }

            const int producer_relative_cycle =
                candidate.relative_cycle_by_address[
                    previous_iteration_producer
                ];

            if (producer_relative_cycle == -1)
            {
                return false;
            }

            const int producer_latency =
                instruction_latency(program, previous_iteration_producer);

            // Equation 2:
            //
            //   S(P) + latency(P) <= S(C) + II
            if (producer_relative_cycle + producer_latency >
                consumer_relative_cycle + ii)
            {
                return false;
            }
        }
    }

    return true;
}

static bool try_place_loop_body_instruction(
    const instruction_t& instruction,
    int earliest_relative_cycle,
    int ii,
    std::set<std::pair<int, int>>& reserved_kernel_slots,
    int& selected_relative_cycle,
    bundle_slot_t& selected_slot
)
{
    const std::vector<bundle_slot_t> possible_slots =
        possible_slots_for_unit(instruction.unit);

    if (possible_slots.empty())
    {
        return false;
    }

    const int max_relative_cycle = earliest_relative_cycle + 4096;

    for (int relative_cycle = earliest_relative_cycle;
         relative_cycle <= max_relative_cycle;
         relative_cycle++)
    {
        const int offset = relative_cycle % ii;

        for (bundle_slot_t slot : possible_slots)
        {
            const int slot_index = bundle_slot_to_index(slot);
            const std::pair<int, int> reservation_key =
                {offset, slot_index};

            if (reserved_kernel_slots.count(reservation_key) != 0)
            {
                continue;
            }

            selected_relative_cycle = relative_cycle;
            selected_slot = slot;
            return true;
        }
    }

    return false;
}

static void append_loop_control_instruction_to_candidate(
    const std::vector<instruction_t>& program,
    int ii,
    loop_body_candidate_t& candidate
)
{
    const int loop_instruction_address =
        find_loop_control_instruction_address(program);

    if (loop_instruction_address == -1)
    {
        return;
    }

    if (candidate.num_stages == 0)
    {
        candidate.num_stages = 1;
    }

    // The loop.pip instruction must be placed at the last offset of the
    // compact loop body, in the Branch slot.
    //
    // Example:
    //   II = 2, num_stages = 2
    //   loop_relative_cycle = 2 * 2 - 1 = 3
    //   offset = 3 % 2 = 1
    //
    // Therefore loop.pip is placed in the final compact bundle.
    const int loop_relative_cycle =
        candidate.num_stages * ii - 1;

    const int loop_stage =
        loop_relative_cycle / ii;

    candidate.relative_cycle_by_address[loop_instruction_address] =
        loop_relative_cycle;

    candidate.slot_by_address[loop_instruction_address] =
        bundle_slot_t::Branch;

    scheduled_instruction_t loop_placement;
    loop_placement.instruction_address = loop_instruction_address;
    loop_placement.cycle = loop_relative_cycle;
    loop_placement.stage = loop_stage;
    loop_placement.slot = bundle_slot_t::Branch;

    candidate.relative_placements.push_back(loop_placement);
}

static bool try_schedule_loop_body_with_ii(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    int ii,
    loop_body_candidate_t& candidate
)
{
    candidate.relative_cycle_by_address.assign(program.size(), -1);
    candidate.slot_by_address.assign(program.size(), bundle_slot_t::ALU0);
    candidate.relative_placements.clear();
    candidate.num_stages = 0;

    // This represents the reservation information after compacting.
    // If (offset, slot) is used by one stage, no other stage can use
    // the same offset and slot.
    std::set<std::pair<int, int>> reserved_kernel_slots;

    for (const instruction_t& instruction : program)
    {
        // Important:
        // The loop / loop.pip instruction is not scheduled like a normal
        // BB1 instruction. It is inserted manually at the final compact
        // bundle's Branch slot.
        if (!is_loop_body_instruction(instruction) ||
            is_loop_control_instruction(instruction))
        {
            continue;
        }

        int earliest_relative_cycle = 0;

        apply_loop_body_local_dependencies(
            program,
            dependency_table,
            candidate,
            instruction,
            earliest_relative_cycle
        );

        const bool interloop_possible =
            apply_known_interloop_constraints(
                program,
                dependency_table,
                candidate,
                instruction,
                ii,
                earliest_relative_cycle
            );

        if (!interloop_possible)
        {
            return false;
        }

        int selected_relative_cycle = -1;
        bundle_slot_t selected_slot = bundle_slot_t::ALU0;

        const bool placed =
            try_place_loop_body_instruction(
                instruction,
                earliest_relative_cycle,
                ii,
                reserved_kernel_slots,
                selected_relative_cycle,
                selected_slot
            );

        if (!placed)
        {
            return false;
        }

        const int offset = selected_relative_cycle % ii;
        const int stage = selected_relative_cycle / ii;

        reserved_kernel_slots.insert(
            {offset, bundle_slot_to_index(selected_slot)}
        );

        candidate.relative_cycle_by_address[
            instruction.instruction_address
        ] = selected_relative_cycle;

        candidate.slot_by_address[
            instruction.instruction_address
        ] = selected_slot;

        scheduled_instruction_t placement;
        placement.instruction_address = instruction.instruction_address;
        placement.cycle = selected_relative_cycle;
        placement.stage = stage;
        placement.slot = selected_slot;

        candidate.relative_placements.push_back(placement);

        candidate.num_stages =
            std::max(candidate.num_stages, stage + 1);
    }

    if (candidate.num_stages == 0 &&
        find_loop_control_instruction_address(program) == -1)
    {
        return false;
    }

    append_loop_control_instruction_to_candidate(
        program,
        ii,
        candidate
    );

    return validate_all_interloop_dependencies(
        program,
        dependency_table,
        candidate,
        ii
    );
}

// -----------------------------------------------------------------------------
// Shifting BB1 after BB0
// -----------------------------------------------------------------------------

static void apply_loop_start_constraint_from_dependency_list(
    const std::vector<instruction_t>& program,
    const absolute_schedule_state_t& state,
    const std::vector<dependency_t>& dependencies,
    int consumer_relative_cycle,
    int& loop_body_start_cycle
)
{
    for (const dependency_t& dependency : dependencies)
    {
        const int producer_address =
            dependency.producer_instruction_address;

        if (producer_address == -1)
        {
            continue;
        }

        const instruction_t& producer =
            get_instruction(program, producer_address);

        if (producer.block != basic_block_t::BB0)
        {
            continue;
        }

        const int producer_cycle =
            state.cycle_by_address[producer_address];

        if (producer_cycle == -1)
        {
            throw std::runtime_error(
                "BB0 producer was not scheduled."
            );
        }

        // producer_cycle + latency <= loop_start + consumer_relative_cycle
        //
        // Therefore:
        //
        // loop_start >= producer_cycle + latency - consumer_relative_cycle
        loop_body_start_cycle = std::max(
            loop_body_start_cycle,
            producer_cycle + producer.latency - consumer_relative_cycle
        );
    }
}

static int calculate_loop_body_start_cycle(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const absolute_schedule_state_t& state,
    const loop_body_candidate_t& loop_body
)
{
    int loop_body_start_cycle = state.last_cycle + 1;

    for (const instruction_t& instruction : program)
    {
        if (!is_loop_body_instruction(instruction) ||
            is_loop_control_instruction(instruction))
        {
            continue;
        }

        const int consumer_relative_cycle =
            loop_body.relative_cycle_by_address[
                instruction.instruction_address
            ];

        if (consumer_relative_cycle == -1)
        {
            throw std::runtime_error(
                "Loop-body instruction was not scheduled."
            );
        }

        const instruction_dependency_info_t& dependency_info =
            get_dependency_info(
                dependency_table,
                instruction.instruction_address
            );

        apply_loop_start_constraint_from_dependency_list(
            program,
            state,
            dependency_info.loop_invariant_dependencies,
            consumer_relative_cycle,
            loop_body_start_cycle
        );

        // For interloop dependencies such as x2: (B or I'),
        // the first iteration still consumes the BB0 producer B.
        apply_loop_start_constraint_from_dependency_list(
            program,
            state,
            dependency_info.interloop_dependencies,
            consumer_relative_cycle,
            loop_body_start_cycle
        );
    }

    return loop_body_start_cycle;
}

static void commit_loop_body_candidate(
    const loop_body_candidate_t& loop_body,
    int loop_body_start_cycle,
    absolute_schedule_state_t& state
)
{
    for (const scheduled_instruction_t& relative_placement :
         loop_body.relative_placements)
    {
        const int absolute_cycle =
            loop_body_start_cycle + relative_placement.cycle;

        const int instruction_address =
            relative_placement.instruction_address;

        mark_absolute_slot_occupied(
            state,
            instruction_address,
            absolute_cycle,
            relative_placement.slot,
            relative_placement.stage
        );
    }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

looppip_schedule_result_t schedule_looppip(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const basic_block_info_t& block_info,
    int max_ii_attempts
)
{
    if (!block_info.has_loop)
    {
        throw std::runtime_error(
            "Cannot schedule loop.pip: program has no loop."
        );
    }

    if (max_ii_attempts <= 0)
    {
        throw std::runtime_error(
            "max_ii_attempts must be positive."
        );
    }

    const int initial_ii =
        calculate_ii_res(program, block_info);

    for (int ii = initial_ii;
         ii < initial_ii + max_ii_attempts;
         ii++)
    {
        absolute_schedule_state_t state;
        state.cycle_by_address.assign(program.size(), -1);
        state.slot_by_address.assign(program.size(), bundle_slot_t::ALU0);
        state.last_cycle = -1;

        // Step 1: schedule BB0 in absolute cycles.
        schedule_absolute_block(
            program,
            dependency_table,
            basic_block_t::BB0,
            0,
            state
        );

        // Step 2: schedule BB1 in relative modulo cycles.
        loop_body_candidate_t loop_body;

        const bool loop_body_valid =
            try_schedule_loop_body_with_ii(
                program,
                dependency_table,
                ii,
                loop_body
            );

        if (!loop_body_valid)
        {
            continue;
        }

        // Step 3: decide where the relative BB1 schedule starts absolutely.
        const int loop_body_start_cycle =
            calculate_loop_body_start_cycle(
                program,
                dependency_table,
                state,
                loop_body
            );

        // Step 4: convert BB1 relative placements into absolute placements.
        commit_loop_body_candidate(
            loop_body,
            loop_body_start_cycle,
            state
        );

        // Step 5: schedule BB2 after the multi-stage loop body.
        const int loop_body_end_cycle =
            loop_body_start_cycle + loop_body.num_stages * ii;

        schedule_absolute_block(
            program,
            dependency_table,
            basic_block_t::BB2,
            loop_body_end_cycle,
            state
        );

        looppip_schedule_result_t result;
        result.ii = ii;
        result.num_stages = loop_body.num_stages;
        result.loop_body_start_cycle = loop_body_start_cycle;
        result.scheduled_program = state.scheduled_program;

        return result;
    }

    throw std::runtime_error(
        "Failed to find a valid loop.pip schedule."
    );
}