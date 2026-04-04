#pragma once

#include "state.hpp"
#include "view.hpp"
#include "cycle_context.hpp"

class rename_and_dispatch_stage
{
public:
    void propagate(const State& curr, View& view, State& next, CycleContext& cycle_context);

private:
    void apply_forwarding_results(State& next, const CycleContext& cycle_context) const;
    bool has_enough_resources(const State& curr, const View& view) const;
};




































