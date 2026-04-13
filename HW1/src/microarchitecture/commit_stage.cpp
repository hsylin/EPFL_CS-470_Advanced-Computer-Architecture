#include "commit_stage.hpp"

void commit_stage::apply_forwarding_results(
    View& view,
    CycleContext& cycle_context) const
{
    for (std::size_t i = 0; i < cycle_context.forwarding_count(); ++i)
    {
        const auto& forwarding_result = cycle_context.forwarding_at(i);

        for (std::size_t j = 0; j < view.active_list.size(); ++j)
        {
            auto& entry = view.active_list.at(j);

            if (entry.pc == forwarding_result.pc)
            {
                entry.done = true;
                entry.exception = forwarding_result.exception;

                if (j == 0 && forwarding_result.exception)
                {
                    cycle_context.request_suppress_head_exception_forwarding(
                        forwarding_result.pc
                    );
                }

                break;
            }
        }
    }
}

void commit_stage::retire_front_instruction(View& view, State& next) const
{
    const auto freed_physical_register =
        view.active_list.front().value().old_destination;

    view.free_list.push(freed_physical_register);
    next.busy_bit_table.set(freed_physical_register, false);
    view.active_list.pop_front();
}

void commit_stage::roll_back_instruction(
    const State& curr,
    View& view,
    State& next,
    CycleContext& cycle_context) const
{
    (void)curr;

    next.exception_flag.set_exception_mode(true);

    cycle_context.request_halt_fetch_decode();
    cycle_context.request_halt_rename_dispatch();
    cycle_context.request_execution_reset();
    cycle_context.request_integer_queue_reset();

    std::size_t rollback_count = 0;

    while (!view.active_list.empty() && rollback_count < 4)
    {
        const auto& youngest_entry = view.active_list.back().value();
        const auto logical_destination = youngest_entry.logical_destination;

        const auto speculative_destination =
            next.register_map_table.get_physical_register(logical_destination);

        view.free_list.push(speculative_destination);
        next.busy_bit_table.set(speculative_destination, false);

        next.register_map_table.set_physical_register(
            logical_destination,
            youngest_entry.old_destination
        );

        view.active_list.pop_back();
        ++rollback_count;
    }
}

void commit_stage::propagate(
    const State& curr,
    View& view,
    State& next,
    CycleContext& cycle_context)
{
    if (!curr.exception_flag.is_exception_mode())
    {
        std::size_t retired_count = 0;
        bool exception_detected = false;

        while (retired_count < 4 && retired_count < curr.active_list.size())
        {
            const auto& head_entry = curr.active_list.at(retired_count);

            if (!head_entry.done)
            {
                break;
            }

            if (head_entry.exception)
            {
                next.exception_flag.set_exception_mode(true);
                next.exception_pc.set(head_entry.pc);

                cycle_context.request_halt_fetch_decode();
                cycle_context.request_halt_rename_dispatch();
                cycle_context.request_execution_reset();
                cycle_context.request_integer_queue_reset();

                exception_detected = true;
                break;
            }

            retire_front_instruction(view, next);
            ++retired_count;
        }

        if (!exception_detected)
        {
            apply_forwarding_results(view, cycle_context);
        }

        return;
    }

    if (curr.active_list.empty())
    {
        next.exception_flag.set_exception_mode(false);
    
        cycle_context.request_halt_fetch_decode();
        cycle_context.request_halt_rename_dispatch();
        cycle_context.request_execution_reset();
        cycle_context.request_integer_queue_reset();
    
        return;
    }

    roll_back_instruction(curr, view, next, cycle_context);
}