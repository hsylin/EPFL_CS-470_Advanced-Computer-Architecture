#ifndef EXECUTION_STAGE_HPP
#define EXECUTION_STAGE_HPP

#include "instruction.hpp"
#include "state.hpp"
#include "view.hpp"
#include "cycle_context.hpp"

class execution_stage
{
public:
    void propagate(const State& curr,
                   View& view,
                   State& next,
                   CycleContext& cycle_context);

    int push_instruction(const instruction_issue_t& instr);
    bool empty() const;

    void reset_stage();

private:
    static constexpr int COMPUTE_SIZE = 4;

    instruction_issue_t pipeline_register_1[COMPUTE_SIZE];
    instruction_issue_t pipeline_register_2[COMPUTE_SIZE];
    int stage1_count = 0;
    int stage2_count = 0;

    void reset(instruction_issue_t& instr);
};

#endif