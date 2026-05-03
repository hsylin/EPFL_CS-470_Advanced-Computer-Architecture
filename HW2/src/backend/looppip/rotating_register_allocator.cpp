#include "rotating_register_allocator.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// Internal allocation state
// -----------------------------------------------------------------------------

struct register_allocation_state_t
{
    // instruction_address -> renamed destination x-register index.
    // -1 means this instruction has no renamed x-register destination.
    std::vector<int> renamed_dest_by_address;

    // instruction_address -> whether renamed_dest_by_address is rotating.
    std::vector<bool> dest_is_rotating;

    std::vector<std::vector<int>> external_source_by_instruction_original;

    // instruction_address -> scheduled placement. e.g.instruction 5 -> cycle 4, stage 1, Mult slot
    std::vector<scheduled_instruction_t> placement_by_address;
    std::vector<bool> has_placement;

    int next_static_register = 1;
    int next_rotating_register = 32;

    int num_stages = 0;
};

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static const instruction_t& get_instruction(
    const std::vector<instruction_t>& program,
    int address
)
{

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

static const scheduled_instruction_t& get_placement(
    const register_allocation_state_t& state,
    int instruction_address
)
{
    if (instruction_address < 0 ||
        instruction_address >= static_cast<int>(state.has_placement.size()) ||
        !state.has_placement[instruction_address])
    {
        throw std::runtime_error(
            "Instruction was not scheduled: " +
            std::to_string(instruction_address)
        );
    }

    return state.placement_by_address[instruction_address];
}

static bool instruction_has_x_destination(const instruction_t& instruction)
{
    return instruction.dest.has_value() && is_x_register(*instruction.dest);
}

static bool same_register(
    const register_ref_t& lhs,
    const register_ref_t& rhs
)
{
    return lhs == rhs;
}


static bool is_loop_control_instruction(const instruction_t& instruction)
{
    return instruction.opcode == instruction_opcode_t::Loop ||
           instruction.opcode == instruction_opcode_t::LoopPip;
}


// -----------------------------------------------------------------------------
// Fresh register allocation
// -----------------------------------------------------------------------------

static int allocate_static_register(register_allocation_state_t& state)
{
    if (state.next_static_register > 31)
    {
        throw std::runtime_error(
            "Ran out of static x-registers x1-x31."
        );
    }

    return state.next_static_register++;
}

static int allocate_rotating_register_base(register_allocation_state_t& state)
{
    const int base = state.next_rotating_register;

    // Each rotating destination owns a small window.
    // The +1 is needed for interloop references to the previous iteration.
    state.next_rotating_register += state.num_stages + 1;

    if (state.next_rotating_register > 96)
    {
        throw std::runtime_error(
            "Ran out of rotating x-registers x32-x95."
        );
    }

    return base;
}

static int normalize_rotating_offset(int offset, int window_size)
{
    int result = offset % window_size;

    if (result < 0)
    {
        result += window_size;
    }

    return result;
}

static int rotating_register_with_offset(
    int base_register,
    int offset,
    int num_stages
)
{
    const int window_size = num_stages + 1;
    return base_register + normalize_rotating_offset(offset, window_size);
}

static std::string x_register_to_string(int index)
{
    if (index < 0 || index > 95)
    {
        throw std::runtime_error(
            "x-register index out of range: " + std::to_string(index)
        );
    }

    return "x" + std::to_string(index);
}

// -----------------------------------------------------------------------------
// Table construction
// -----------------------------------------------------------------------------

static void build_placement_tables(
    const std::vector<instruction_t>& program,
    const looppip_schedule_result_t& schedule_result,
    register_allocation_state_t& state
)
{
    state.placement_by_address.assign(program.size(), scheduled_instruction_t{});
    state.has_placement.assign(program.size(), false);

    for (const scheduled_instruction_t& placement :
         schedule_result.scheduled_program.scheduled_instructions)
    {
        state.placement_by_address[placement.instruction_address] = placement;
        state.has_placement[placement.instruction_address] = true;
    }
}

// -----------------------------------------------------------------------------
// Destination allocation
// -----------------------------------------------------------------------------

static std::vector<int> get_scheduled_addresses_in_order(
    const std::vector<instruction_t>& program,
    const register_allocation_state_t& state
)
{
    std::vector<int> addresses;

    for (const instruction_t& instruction : program)
    {
        const int address = instruction.instruction_address;

        if (address < 0 ||
            address >= static_cast<int>(state.has_placement.size()))
        {
            continue;
        }

        if (!state.has_placement[address])
        {
            continue;
        }

        addresses.push_back(address);
    }

    std::sort(
        addresses.begin(),
        addresses.end(),
        [&](int lhs, int rhs)
        {
            const scheduled_instruction_t& lhs_placement =
                state.placement_by_address[lhs];

            const scheduled_instruction_t& rhs_placement =
                state.placement_by_address[rhs];

            if (lhs_placement.cycle != rhs_placement.cycle)
            {
                return lhs_placement.cycle < rhs_placement.cycle;
            }

            const int lhs_slot =
                bundle_slot_to_index(lhs_placement.slot);

            const int rhs_slot =
                bundle_slot_to_index(rhs_placement.slot);

            if (lhs_slot != rhs_slot)
            {
                return lhs_slot < rhs_slot;
            }

            return lhs < rhs;
        }
    );

    return addresses;
}




static void assign_rotating_destinations_for_bb1(
    const std::vector<instruction_t>& program,
    register_allocation_state_t& state
)
{
    const std::vector<int> scheduled_addresses =
        get_scheduled_addresses_in_order(program, state);

    for (int address : scheduled_addresses)
    {
        const instruction_t& instruction =
            get_instruction(program, address);

        if (instruction.block != basic_block_t::BB1)
        {
            continue;
        }

        if (is_loop_control_instruction(instruction))
        {
            continue;
        }

        if (!instruction_has_x_destination(instruction))
        {
            continue;
        }

        state.renamed_dest_by_address[address] =
            allocate_rotating_register_base(state);

        state.dest_is_rotating[address] = true;
    }
}

static void assign_static_destination(
    const instruction_t& instruction,
    register_allocation_state_t& state
)
{
    if (!instruction_has_x_destination(instruction))
    {
        return;
    }

    const int address = instruction.instruction_address;

    if (state.renamed_dest_by_address[address] != -1)
    {
        return;
    }

    state.renamed_dest_by_address[address] =
        allocate_static_register(state);

    state.dest_is_rotating[address] = false;
}

static void assign_loop_invariant_producers(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    register_allocation_state_t& state
)
{
    for (const instruction_t& consumer : program)
    {
        const instruction_dependency_info_t& dependency_info =
            get_dependency_info(
                dependency_table,
                consumer.instruction_address
            );

        for (const dependency_t& dependency :
             dependency_info.loop_invariant_dependencies)
        {
            const int producer_address =
                dependency.producer_instruction_address;

            if (producer_address == -1)
            {
                continue;
            }

            const instruction_t& producer =
                get_instruction(program, producer_address);

           assign_static_destination(producer, state);
        }
    }
}

static void assign_initial_interloop_producers(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    register_allocation_state_t& state
)
{
    for (const instruction_t& consumer : program)
    {
        if (consumer.block != basic_block_t::BB1)
        {
            continue;
        }

        const instruction_dependency_info_t& dependency_info =
            get_dependency_info(
                dependency_table,
                consumer.instruction_address
            );

        for (const dependency_t& dependency :
             dependency_info.interloop_dependencies)
        {
            const int initial_producer_address =
                dependency.producer_instruction_address;

            const int previous_iteration_producer_address =
                dependency.previous_iteration_producer_instruction_address;

            if (initial_producer_address == -1 ||
                previous_iteration_producer_address == -1)
            {
                continue;
            }

            const instruction_t& initial_producer =
                get_instruction(program, initial_producer_address);

            const instruction_t& previous_iteration_producer =
                get_instruction(program, previous_iteration_producer_address);

            if (initial_producer.block != basic_block_t::BB0)
            {
                continue;
            }

            if (!instruction_has_x_destination(initial_producer) ||
                !instruction_has_x_destination(previous_iteration_producer))
            {
                continue;
            }

            const int previous_base =
                state.renamed_dest_by_address[
                    previous_iteration_producer_address
                ];

            if (previous_base == -1)
            {
                throw std::runtime_error(
                    "Previous-iteration producer has no rotating destination."
                );
            }

            const int producer_stage =
                get_placement(
                    state,
                    previous_iteration_producer_address
                ).stage;

            // The BB0 producer initializes the value consumed by the first
            // iteration. The later iterations use the previous-iteration BB1
            // producer. This maps the BB0 initial value into the rotating
            // register window used by that recurrence.
            const int initial_register =
                rotating_register_with_offset(
                    previous_base,
                    1 - producer_stage,
                    state.num_stages
                );

            state.renamed_dest_by_address[initial_producer_address] =
                initial_register;

            state.dest_is_rotating[initial_producer_address] = true;
        }
    }
}

static void assign_remaining_static_destinations(
    const std::vector<instruction_t>& program,
    register_allocation_state_t& state
)
{
    for (const instruction_t& instruction : program)
    {
        if (instruction.block == basic_block_t::BB1)
        {
            continue;
        }

       assign_static_destination(instruction, state);
    }
}

static void allocate_destinations(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    register_allocation_state_t& state
)
{
    state.renamed_dest_by_address.assign(program.size(), -1);
    state.dest_is_rotating.assign(program.size(), false);

    // Step 1. BB1 destinations use rotating registers.
    assign_rotating_destinations_for_bb1(program, state);

    // Step 2. Loop-invariant producers in BB0 use static registers.
    assign_loop_invariant_producers(program, dependency_table, state);

    // Step 3. BB0 initial producers for interloop dependencies must be mapped into the same rotating window used by the BB1 recurrence.
    assign_initial_interloop_producers(program, dependency_table, state);

    // Step 4. Remaining BB0 / BB2 x-register destinations use static registers.
    assign_remaining_static_destinations(program, state);
}

// -----------------------------------------------------------------------------
// Dependency lookup for source operands
// -----------------------------------------------------------------------------

static const dependency_t* find_dependency_for_register(
    const std::vector<dependency_t>& dependencies,
    const register_ref_t& source_register
)
{
    for (const dependency_t& dependency : dependencies)
    {
        if (same_register(dependency.operand_register, source_register))
        {
            return &dependency;
        }
    }

    return nullptr;
}

static bool has_dependency_for_source_register(
    const dependency_table_t& dependency_table,
    const instruction_t& instruction,
    const register_ref_t& source_register
)
{
    const instruction_dependency_info_t& dependency_info =
        get_dependency_info(
            dependency_table,
            instruction.instruction_address
        );

    return find_dependency_for_register(
               dependency_info.local_dependencies,
               source_register
           ) != nullptr ||
           find_dependency_for_register(
               dependency_info.interloop_dependencies,
               source_register
           ) != nullptr ||
           find_dependency_for_register(
               dependency_info.loop_invariant_dependencies,
               source_register
           ) != nullptr ||
           find_dependency_for_register(
               dependency_info.post_loop_dependencies,
               source_register
           ) != nullptr;
}

static void assign_external_source_registers(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    register_allocation_state_t& state
)
{
    state.external_source_by_instruction_original.assign(
        program.size(),
        std::vector<int>(96, -1)
    );

    const std::vector<int> scheduled_addresses =
        get_scheduled_addresses_in_order(program, state);

    for (int address : scheduled_addresses)
    {
        const instruction_t& instruction =
            get_instruction(program, address);

        for (const register_ref_t& source_register : instruction.src_regs)
        {
            if (!is_x_register(source_register))
            {
                continue;
            }

            if (source_register.index < 0 ||
                source_register.index >= 96)
            {
                continue;
            }

            if (instruction.instruction_address < 0 ||
                instruction.instruction_address >= static_cast<int>(
                    state.external_source_by_instruction_original.size()
                ))
            {
                continue;
            }

            if (has_dependency_for_source_register(
                    dependency_table,
                    instruction,
                    source_register
                ))
            {
                continue;
            }

            int& external_index =
                state.external_source_by_instruction_original
                    [instruction.instruction_address]
                    [source_register.index];

            if (external_index == -1)
            {
                external_index = allocate_static_register(state);
            }
        }
    }
}

static std::string renamed_destination_string(
    const std::vector<instruction_t>& program,
    const register_allocation_state_t& state,
    int producer_address
)
{
    if (producer_address == -1)
    {
        throw std::runtime_error("Missing producer address.");
    }

    const instruction_t& producer =
        get_instruction(program, producer_address);

    if (!producer.dest.has_value())
    {
        throw std::runtime_error(
            "Producer does not have a destination register."
        );
    }

    if (!is_x_register(*producer.dest))
    {
        return register_to_string(*producer.dest);
    }

    const int renamed_index =
        state.renamed_dest_by_address[producer_address];

    if (renamed_index == -1)
    {
        throw std::runtime_error(
            "Producer has no renamed destination register."
        );
    }

    return x_register_to_string(renamed_index);
}

static std::string renamed_local_source_string(
    const std::vector<instruction_t>& program,
    const register_allocation_state_t& state,
    int consumer_address,
    int producer_address
)
{
    const instruction_t& producer =
        get_instruction(program, producer_address);

    if (producer.block != basic_block_t::BB1)
    {
        return renamed_destination_string(program, state, producer_address);
    }

    const int producer_base =
        state.renamed_dest_by_address[producer_address];

    if (producer_base == -1)
    {
        throw std::runtime_error(
            "Local producer has no rotating destination."
        );
    }

    const int consumer_stage =
        get_placement(state, consumer_address).stage;

    const int producer_stage =
        get_placement(state, producer_address).stage;

    const int offset = consumer_stage - producer_stage;

    return x_register_to_string(
        rotating_register_with_offset(
            producer_base,
            offset,
            state.num_stages
        )
    );
}

static std::string renamed_interloop_source_string(
    const std::vector<instruction_t>& program,
    const register_allocation_state_t& state,
    int consumer_address,
    int previous_iteration_producer_address
)
{
    const int producer_base =
        state.renamed_dest_by_address[
            previous_iteration_producer_address
        ];

    if (producer_base == -1)
    {
        throw std::runtime_error(
            "Interloop producer has no rotating destination."
        );
    }

    const int consumer_stage =
        get_placement(state, consumer_address).stage;

    const int producer_stage =
        get_placement(state, previous_iteration_producer_address).stage;

    // Interloop dependency reads the previous iteration.
    const int offset = consumer_stage - producer_stage + 1;

    return x_register_to_string(
        rotating_register_with_offset(
            producer_base,
            offset,
            state.num_stages
        )
    );
}

static std::string renamed_post_loop_source_string(
    const std::vector<instruction_t>& program,
    const register_allocation_state_t& state,
    int producer_address
)
{
    const instruction_t& producer =
        get_instruction(program, producer_address);

    if (producer.block != basic_block_t::BB1)
    {
        return renamed_destination_string(program, state, producer_address);
    }

    const int producer_base =
        state.renamed_dest_by_address[producer_address];

    if (producer_base == -1)
    {
        throw std::runtime_error(
            "Post-loop producer has no rotating destination."
        );
    }

    const int producer_stage =
        get_placement(state, producer_address).stage;

    const int offset = state.num_stages - 1 - producer_stage;

    return x_register_to_string(
        rotating_register_with_offset(
            producer_base,
            offset,
            state.num_stages
        )
    );
}

static std::string resolve_source_register(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const register_allocation_state_t& state,
    const instruction_t& instruction,
    const register_ref_t& source_register
)
{
    if (!is_x_register(source_register))
    {
        return register_to_string(source_register);
    }

    const instruction_dependency_info_t& dependency_info =
        get_dependency_info(
            dependency_table,
            instruction.instruction_address
        );

    const dependency_t* local_dependency =
        find_dependency_for_register(
            dependency_info.local_dependencies,
            source_register
        );

    if (local_dependency != nullptr)
    {
        return renamed_local_source_string(
            program,
            state,
            instruction.instruction_address,
            local_dependency->producer_instruction_address
        );
    }

    const dependency_t* interloop_dependency =
        find_dependency_for_register(
            dependency_info.interloop_dependencies,
            source_register
        );

    if (interloop_dependency != nullptr)
    {
        if (instruction.block == basic_block_t::BB1 &&
            interloop_dependency
                ->previous_iteration_producer_instruction_address != -1)
        {
            return renamed_interloop_source_string(
                program,
                state,
                instruction.instruction_address,
                interloop_dependency
                    ->previous_iteration_producer_instruction_address
            );
        }

        if (interloop_dependency->producer_instruction_address != -1)
        {
            return renamed_destination_string(
                program,
                state,
                interloop_dependency->producer_instruction_address
            );
        }
    }

    const dependency_t* loop_invariant_dependency =
        find_dependency_for_register(
            dependency_info.loop_invariant_dependencies,
            source_register
        );

    if (loop_invariant_dependency != nullptr)
    {
        return renamed_destination_string(
            program,
            state,
            loop_invariant_dependency->producer_instruction_address
        );
    }

    const dependency_t* post_loop_dependency =
        find_dependency_for_register(
            dependency_info.post_loop_dependencies,
            source_register
        );

    if (post_loop_dependency != nullptr)
    {
        return renamed_post_loop_source_string(
            program,
            state,
            post_loop_dependency->producer_instruction_address
        );
    }

    if (instruction.instruction_address >= 0 &&
        instruction.instruction_address < static_cast<int>(
            state.external_source_by_instruction_original.size()
        ) &&
        source_register.index >= 0 &&
        source_register.index < static_cast<int>(
            state.external_source_by_instruction_original
                [instruction.instruction_address].size()
        ))
    {
        const int external_index =
            state.external_source_by_instruction_original
                [instruction.instruction_address]
                [source_register.index];

        if (external_index != -1)
        {
            return x_register_to_string(external_index);
        }
    }

    // No dependency means this is an external / unused source.
    // Keep the original register name.
    return register_to_string(source_register);
}


// -----------------------------------------------------------------------------
// String formatting helpers
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
        case instruction_opcode_t::Unknown:
            break;
    }

    throw std::runtime_error("Cannot print unknown opcode.");
}

static std::string destination_register_string(
    const register_allocation_state_t& state,
    const instruction_t& instruction
)
{
    if (!instruction.dest.has_value())
    {
        return "";
    }

    const register_ref_t& dest = *instruction.dest;

    if (!is_x_register(dest))
    {
        return register_to_string(dest);
    }

    const int renamed_index =
        state.renamed_dest_by_address[instruction.instruction_address];

    if (renamed_index == -1)
    {
        return register_to_string(dest);
    }

    return x_register_to_string(renamed_index);
}

static std::string source_register_string(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const register_allocation_state_t& state,
    const instruction_t& instruction,
    const register_ref_t& source_register
)
{
    return resolve_source_register(
        program,
        dependency_table,
        state,
        instruction,
        source_register
    );
}

static std::string immediate_to_string(const operand_t& operand)
{
    return std::to_string(operand.immediate);
}

static std::string memory_immediate_text(const operand_t& operand)
{
    return std::to_string(operand.memory.immediate);
}

static std::string memory_operand_string(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const register_allocation_state_t& state,
    const instruction_t& instruction,
    const operand_t& operand
)
{
    const std::string base_register =
        source_register_string(
            program,
            dependency_table,
            state,
            instruction,
            operand.memory.base_register
        );

    return memory_immediate_text(operand) + "(" + base_register + ")";
}

static std::string operand_as_source_string(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const register_allocation_state_t& state,
    const instruction_t& instruction,
    const operand_t& operand
)
{
    switch (operand.kind)
    {
        case operand_kind_t::Register:
            return source_register_string(
                program,
                dependency_table,
                state,
                instruction,
                operand.reg
            );

        case operand_kind_t::Immediate:
            return immediate_to_string(operand);

        case operand_kind_t::Memory:
            return memory_operand_string(
                program,
                dependency_table,
                state,
                instruction,
                operand
            );

        case operand_kind_t::Boolean:
            return operand.boolean_value ? "true" : "false";

        case operand_kind_t::None:
            break;
    }

    throw std::runtime_error("Invalid operand kind.");
}

static void require_operand_count(
    const instruction_t& instruction,
    std::size_t expected_count
)
{
    if (instruction.operands.size() != expected_count)
    {
        throw std::runtime_error(
            "Unexpected operand count while printing instruction: " +
            instruction.raw
        );
    }
}

static std::string build_allocated_instruction_string(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const register_allocation_state_t& state,
    const instruction_t& instruction
)
{
    if (instruction.opcode == instruction_opcode_t::Nop)
    {
        return "nop";
    }

    std::string result;

    if (instruction.predicate.has_value())
    {
        result += "(" + register_to_string(*instruction.predicate) + ") ";
    }

    result += opcode_to_string(instruction.opcode);

    switch (instruction.opcode)
    {
        case instruction_opcode_t::Add:
        case instruction_opcode_t::Sub:
        case instruction_opcode_t::Mulu:
        {
            require_operand_count(instruction, 3);

            result += " ";
            result += destination_register_string(state, instruction);
            result += ", ";
            result += operand_as_source_string(
                program, dependency_table, state, instruction,
                instruction.operands[1]
            );
            result += ", ";
            result += operand_as_source_string(
                program, dependency_table, state, instruction,
                instruction.operands[2]
            );
            break;
        }

        case instruction_opcode_t::Addi:
        {
            require_operand_count(instruction, 3);

            result += " ";
            result += destination_register_string(state, instruction);
            result += ", ";
            result += operand_as_source_string(
                program, dependency_table, state, instruction,
                instruction.operands[1]
            );
            result += ", ";
            result += immediate_to_string(instruction.operands[2]);
            break;
        }

        case instruction_opcode_t::Ld:
        {
            require_operand_count(instruction, 2);

            result += " ";
            result += destination_register_string(state, instruction);
            result += ", ";
            result += memory_operand_string(
                program, dependency_table, state, instruction,
                instruction.operands[1]
            );
            break;
        }

        case instruction_opcode_t::St:
        {
            require_operand_count(instruction, 2);

            result += " ";
            result += operand_as_source_string(
                program, dependency_table, state, instruction,
                instruction.operands[0]
            );
            result += ", ";
            result += memory_operand_string(
                program, dependency_table, state, instruction,
                instruction.operands[1]
            );
            break;
        }

        case instruction_opcode_t::Mov:
        {
            require_operand_count(instruction, 2);

            result += " ";
            result += destination_register_string(state, instruction);
            result += ", ";
            result += operand_as_source_string(
                program, dependency_table, state, instruction,
                instruction.operands[1]
            );
            break;
        }

        case instruction_opcode_t::Loop:
        case instruction_opcode_t::LoopPip:
        {
            require_operand_count(instruction, 1);

            result += " ";
            result += immediate_to_string(instruction.operands[0]);
            break;
        }

        case instruction_opcode_t::Nop:
        case instruction_opcode_t::Unknown:
            throw std::runtime_error(
                "Invalid opcode while printing instruction."
            );
    }

    return result;
}

// -----------------------------------------------------------------------------
// Final schedule construction
// -----------------------------------------------------------------------------

static schedule_t build_allocated_schedule(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const register_allocation_state_t& state,
    const looppip_schedule_result_t& schedule_result
)
{
    schedule_t schedule;

    for (const scheduled_instruction_t& placement :
         schedule_result.scheduled_program.scheduled_instructions)
    {
        const instruction_t& instruction =
            get_instruction(program, placement.instruction_address);

        const std::string instruction_text =
            build_allocated_instruction_string(
                program,
                dependency_table,
                state,
                instruction
            );

        put_instruction_in_schedule(
            schedule,
            placement.cycle,
            placement.slot,
            instruction_text
        );
    }

    return schedule;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

schedule_t allocate_rotating_registers(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const basic_block_info_t& block_info,
    const looppip_schedule_result_t& schedule_result
)
{
    if (!block_info.has_loop)
    {
        throw std::runtime_error(
            "Cannot allocate rotating registers: program has no loop."
        );
    }


    register_allocation_state_t state;
    state.num_stages = schedule_result.num_stages;

    build_placement_tables(program, schedule_result, state);

    allocate_destinations(
        program,
        dependency_table,
        state
    );

    assign_external_source_registers(
        program,
        dependency_table,
        state
    );

    return build_allocated_schedule(
        program,
        dependency_table,
        state,
        schedule_result
    );
}