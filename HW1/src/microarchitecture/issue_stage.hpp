#include "execution_stage.hpp"

class issue_stage
{
public:
    void propagate(const State& curr,
                   View& view,
                   State& next,
                   CycleContext& cycle_context,
                   execution_stage& execution);

private:
    void apply_forwarding_results(View& view,
                                  const CycleContext& cycle_context) const;
};