#include "state.hpp"

State::State()
{
    reset();
}

void State::reset()
{
    program_counter.reset();
    physical_register_file.reset();
    decoded_instruction_register.reset();
    exception_flag.reset();
    exception_pc.reset();
    register_map_table.reset();
    free_list.reset();
    busy_bit_table.reset();
    active_list.reset();
    integer_queue.reset();
}

json State::dump() const
{
    json j;

    program_counter.dump(j);
    physical_register_file.dump(j);
    decoded_instruction_register.dump(j);
    exception_flag.dump(j);
    exception_pc.dump(j);
    register_map_table.dump(j);
    free_list.dump(j);
    busy_bit_table.dump(j);
    active_list.dump(j);
    integer_queue.dump(j);

    return j;
}