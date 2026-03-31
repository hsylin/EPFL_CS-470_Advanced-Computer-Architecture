#pragma once

#include "free_list.hpp"
#include "active_list.hpp"
#include "integer_queue.hpp"

// Forward declaration to avoid circular include with state.hpp
class State;

class View
{
public:
    View();

    // Reset all working queue copies
    void reset();

    // Initialize the working copies from the current state
    void load_from_state(const State& curr);

    // Write the final queue working copies back into next state
    void write_back_to_state(State& next) const;

public:
    FreeList free_list;
    ActiveList active_list;
    IntegerQueue integer_queue;
};