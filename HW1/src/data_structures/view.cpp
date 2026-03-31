#include "view.hpp"
#include "state.hpp"

View::View()
{
    reset();
}

void View::reset()
{
    free_list.reset();
    active_list.reset();
    integer_queue.reset();
}

void View::load_from_state(const State& curr)
{
    free_list = curr.free_list;
    active_list = curr.active_list;
    integer_queue = curr.integer_queue;
}

void View::write_back_to_state(State& next) const
{
    next.free_list = free_list;
    next.active_list = active_list;
    next.integer_queue = integer_queue;
}