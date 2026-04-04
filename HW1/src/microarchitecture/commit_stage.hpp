#pragma once

#include "state.hpp"
#include "view.hpp"
#include "cycle_context.hpp"

class commit_stage
{
public:
    void propagate(const State& curr, View& view, State& next, CycleContext& cycle_context);

private:
    void apply_forwarding_results(View& view, const CycleContext& cycle_context) const;
    void retire_front_instruction(View& view, State& next) const;
    void roll_back_instruction(const State& curr,
                                View& view,
                                State& next,
                                CycleContext& cycle_context) const;
};






