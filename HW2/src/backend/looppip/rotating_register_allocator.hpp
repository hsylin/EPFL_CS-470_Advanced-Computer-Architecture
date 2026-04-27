#pragma once

#include "looppip_scheduler.hpp"

#include "../../common/instruction.hpp"
#include "../../common/schedule.hpp"
#include "../../frontend/basic_block.hpp"
#include "../../middleend/dependency_analysis.hpp"

#include <vector>

schedule_t allocate_rotating_registers(
    const std::vector<instruction_t>& program,
    const dependency_table_t& dependency_table,
    const basic_block_info_t& block_info,
    const looppip_schedule_result_t& schedule_result
);