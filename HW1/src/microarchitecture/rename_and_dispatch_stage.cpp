#include "rename_and_dispatch_stage.hpp"

void rename_and_dispatch_stage::apply_forwarding_results(
    State& next,
    const CycleContext& cycle_context) const
{
    for (std::size_t i = 0; i < cycle_context.forwarding_count(); ++i)
    {
        const auto& forwarding_result = cycle_context.forwarding_at(i);

        if (forwarding_result.exception)
        {
            continue;
        }

        next.physical_register_file.set(
            forwarding_result.physical_register,
            forwarding_result.value
        );

        next.busy_bit_table.set(
            forwarding_result.physical_register,
            false
        );
    }
}

bool rename_and_dispatch_stage::has_enough_resources(
    const State& curr,
    const View& view) const
{
    const std::size_t bundle_size = curr.decoded_instruction_register.size();

    return bundle_size <= view.free_list.size()
        && bundle_size <= view.active_list.available_slots()
        && bundle_size <= view.integer_queue.available_slots();
}

void rename_and_dispatch_stage::propagate(
    const State& curr,
    View& view,
    State& next,
    CycleContext& cycle_context)
{

    if (cycle_context.is_halt_rename_dispatch_requested())
    {
        return;
    }

    apply_forwarding_results(next, cycle_context);

    const std::size_t bundle_size = curr.decoded_instruction_register.size();

    if (bundle_size == 0)
    {
        return;
    }

    if (!has_enough_resources(curr, view))
    {
        cycle_context.set_rename_backpressure(true);
        return;
    }

    for (const auto& instruction : curr.decoded_instruction_register)
    {
        const auto op_a_tag =
            next.register_map_table.get_physical_register(
                instruction.op1_logical_reg
            );

        const bool op_a_ready = !next.busy_bit_table.get(op_a_tag);

        std::optional<uint64_t> op_a_value = std::nullopt;
        if (op_a_ready)
        {
            op_a_value = next.physical_register_file.get(op_a_tag);
        }

        bool op_b_ready = false;
        uint64_t op_b_tag = 0;
        std::optional<uint64_t> op_b_value = std::nullopt;

        if (instruction.op2_is_immediate)
        {
            op_b_ready = true;
            op_b_tag = 0;
            op_b_value = static_cast<uint64_t>(instruction.op2_immediate);
        }
        else
        {
            op_b_tag = next.register_map_table.get_physical_register(
                instruction.op2_logical_reg
            );

            op_b_ready = !next.busy_bit_table.get(op_b_tag);

            if (op_b_ready)
            {
                op_b_value = next.physical_register_file.get(op_b_tag);
            }
        }

        const auto old_destination =
            next.register_map_table.get_physical_register(
                instruction.logical_destination
            );

        const auto new_destination = view.free_list.pop().value();

        next.register_map_table.set_physical_register(
            instruction.logical_destination,
            new_destination
        );

        next.busy_bit_table.set(new_destination, true);

        view.integer_queue.push_back(
            IntegerQueueEntry{
                .dest_register = new_destination,
                .op_a_is_ready = op_a_ready,
                .op_a_reg_tag = op_a_tag,
                .op_a_value = op_a_value,
                .op_b_is_ready = op_b_ready,
                .op_b_reg_tag = op_b_tag,
                .op_b_value = op_b_value,
                .op_code = instruction.opcode,
                .pc = instruction.pc
            });

        view.active_list.push_back(
            ActiveListEntry{
                .done = false,
                .exception = false,
                .logical_destination = instruction.logical_destination,
                .old_destination = old_destination,
                .pc = instruction.pc
            });
    }

    next.decoded_instruction_register.reset();
}