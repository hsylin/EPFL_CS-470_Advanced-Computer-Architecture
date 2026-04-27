#pragma once

#include "../../common/instruction.hpp"
#include "../../frontend/basic_block.hpp"

#include <vector>

struct ii_resource_count_t
{
    int alu_count = 0;
    int mult_count = 0;
    int mem_count = 0;
    int branch_count = 0;
};

ii_resource_count_t count_loop_body_resources(const std::vector<instruction_t>& program);

int calculate_ii_res( const std::vector<instruction_t>& program, const basic_block_info_t& block_info );