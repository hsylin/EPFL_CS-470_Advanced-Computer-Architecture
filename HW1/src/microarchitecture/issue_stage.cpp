#include "issue_stage.hpp"
#include "execution_stage.hpp"


void issue_stage::apply_forwarding_results(
    View& view,
    const CycleContext& cycle_context) const
{
    for (std::size_t i = 0; i < cycle_context.forwarding_count(); ++i)
    {
        const auto& forwarding_result = cycle_context.forwarding_at(i);

        if (cycle_context.should_suppress_forwarding(forwarding_result.pc))
        {
            continue;
        }

        for (std::size_t j = 0; j < view.integer_queue.size(); ++j)
        {
            auto& entry = view.integer_queue.at(j);

            if (!entry.op_a_is_ready &&
                entry.op_a_reg_tag == forwarding_result.physical_register)
            {
                entry.op_a_value = forwarding_result.value;
                entry.op_a_is_ready = true;
            }

            if (!entry.op_b_is_ready &&
                entry.op_b_reg_tag == forwarding_result.physical_register)
            {
                entry.op_b_value = forwarding_result.value;
                entry.op_b_is_ready = true;
            }
        }
    }
}


void issue_stage::propagate(const State& curr,
                            View& view,
                            State& next,
                            CycleContext& cycle_context,
                            execution_stage& execution)
{
    (void)curr;
    (void)next;


    
    if (cycle_context.is_integer_queue_reset_requested())
    {
        view.integer_queue.reset();
        return;
    }

    apply_forwarding_results(view, cycle_context);
    

    std::size_t issue_count = 0;
    std::size_t j = 0;

    while (j < view.integer_queue.size() && issue_count < 4)
    {
        auto& entry = view.integer_queue.at(j);

        if (entry.op_a_is_ready && entry.op_b_is_ready)
        {
            const int push_result =
                execution.push_instruction(
                    instruction_issue_t{
                        .opcode = entry.op_code,
                        .op1_value = entry.op_a_value.value(),
                        .op2_value = entry.op_b_value.value(),
                        .physical_destination = entry.dest_register,
                        .pc = entry.pc
                    }
                );

            if (push_result == 0)
            {
                view.integer_queue.erase_at(j);
                issue_count++;
            }
            else
            {
                break;
            }
        }
        else
        {
            j++;
        }
    }
}