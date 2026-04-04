
#pragma once
#include <cstdint>
#include <string>
#include <vector>






enum class instruction_opcode_t
{
    add,
    addi,
    sub,
    mulu,
    divu,
    remu
};

using logical_reg_t = unsigned int;
using physical_reg_t = unsigned int;



struct instruction_decode_t
{
    instruction_opcode_t opcode;
    
    logical_reg_t op1_logical_reg = 0;

    bool op2_is_immediate = false;
    logical_reg_t op2_logical_reg = 0;
    int64_t op2_immediate = 0;

    logical_reg_t logical_destination = 0;
    uint64_t pc = 0;
};


struct instruction_issue_t
{
    instruction_opcode_t opcode;
    uint64_t op1_value = 0;
    uint64_t op2_value = 0;
    physical_reg_t physical_destination = 0;
    uint64_t pc = 0;
};



