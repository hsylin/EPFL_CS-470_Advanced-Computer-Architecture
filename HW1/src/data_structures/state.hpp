#pragma once

#include <nlohmann/json.hpp>

#include "program_counter.hpp"
#include "physical_register_file.hpp"
#include "decoded_instruction_register.hpp"
#include "exception_flag.hpp"
#include "exception_program_counter.hpp"
#include "register_map_table.hpp"
#include "free_list.hpp"
#include "busy_bit_table.hpp"
#include "active_list.hpp"
#include "integer_queue.hpp"

using json = nlohmann::json;

class State
{
public:
    State();

    void reset();
    json dump() const;

public:
    ProgramCounter program_counter;
    PhysicalRegisterFile physical_register_file;
    DecodedInstructionRegister decoded_instruction_register;
    ExceptionFlag exception_flag;
    ExceptionPC exception_pc;
    RegisterMapTable register_map_table;
    FreeList free_list;
    BusyBitTable busy_bit_table;
    ActiveList active_list;
    IntegerQueue integer_queue;
};