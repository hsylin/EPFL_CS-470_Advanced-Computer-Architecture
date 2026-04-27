#pragma once

#include "../../common/instruction.hpp"
#include "../../common/schedule.hpp"
#include "../../frontend/basic_block.hpp"
#include "../../middleend/dependency_analysis.hpp"

#include <vector>

struct looppip_schedule_result_t
{
    int ii = -1;
    int num_stages = -1;
    int loop_body_start_cycle = -1;

    scheduled_program_t scheduled_program;
};

looppip_schedule_result_t schedule_looppip(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const basic_block_info_t& block_info,
    int max_ii_attempts = 64
);