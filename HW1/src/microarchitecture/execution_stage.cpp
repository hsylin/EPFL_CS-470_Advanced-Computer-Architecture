#include "execution_stage.hpp"

void execution_stage::reset(instruction_issue_t& instr)
{
    instr.op1_value = 0;
    instr.op2_value = 0;
    instr.physical_destination = 0;
    instr.pc = 0;
}

void execution_stage::reset_stage()
{
    for (int i = 0; i < COMPUTE_SIZE; ++i)
    {
        reset(pipeline_register_1[i]);
        reset(pipeline_register_2[i]);
    }

    stage1_count = 0;
    stage2_count = 0;
}

int execution_stage::push_instruction(const instruction_issue_t& instr)
{
    if (stage1_count >= COMPUTE_SIZE)
    {
        return -1;
    }

    pipeline_register_1[stage1_count] = instr;
    stage1_count++;

    return 0;
}

bool execution_stage::empty() const
{
    return stage1_count == 0 && stage2_count == 0;
}

void execution_stage::propagate(const State& curr,
                                View& view,
                                State& next,
                                CycleContext& cycle_context)
{
    (void)curr;
    (void)view;
    (void)next;

    if (cycle_context.is_execution_reset_requested())
    {
        reset_stage();
        return;
    }

    for (int i = 0; i < stage2_count; ++i)
    {
        uint64_t value = 0;
        bool exception = false;

        switch (pipeline_register_2[i].opcode)
        {
        case instruction_opcode_t::add:
        case instruction_opcode_t::addi:
            value = pipeline_register_2[i].op1_value + pipeline_register_2[i].op2_value;
            break;

        case instruction_opcode_t::sub:
            value = pipeline_register_2[i].op1_value - pipeline_register_2[i].op2_value;
            break;

        case instruction_opcode_t::mulu:
            value = pipeline_register_2[i].op1_value * pipeline_register_2[i].op2_value;
            break;

        case instruction_opcode_t::divu:
            if (pipeline_register_2[i].op2_value == 0)
            {
                exception = true;
                value = 0;
            }
            else
            {
                value = pipeline_register_2[i].op1_value / pipeline_register_2[i].op2_value;
            }
            break;

        case instruction_opcode_t::remu:
            if (pipeline_register_2[i].op2_value == 0)
            {
                exception = true;
                value = 0;
            }
            else
            {
                value = pipeline_register_2[i].op1_value % pipeline_register_2[i].op2_value;
            }
            break;
        }

        cycle_context.add_forwarding_result(
            pipeline_register_2[i].physical_destination,
            value,
            exception,
            static_cast<unsigned int>(pipeline_register_2[i].pc)
        );

        reset(pipeline_register_2[i]);
    }

    stage2_count = 0;

    for (int i = 0; i < stage1_count; ++i)
    {
        pipeline_register_2[i] = pipeline_register_1[i];
        reset(pipeline_register_1[i]);
    }

    stage2_count = stage1_count;
    stage1_count = 0;
}