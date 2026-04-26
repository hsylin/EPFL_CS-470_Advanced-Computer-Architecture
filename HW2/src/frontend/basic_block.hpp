#pragma once

#include "instruction.hpp"

#include <vector>

struct basic_block_info_t
{
    bool has_loop = false;

    int loop_start_address = -1;
    int loop_instruction_address = -1;
};

basic_block_info_t split_basic_blocks(std::vector<instruction_t>& program);