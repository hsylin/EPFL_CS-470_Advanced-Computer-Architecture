#pragma once

#include <vector>
#include <array>

#include "../../common/instruction.hpp"
#include "../../middleend/dependency_analysis.hpp"
#include "../../common/schedule.hpp"
#include "../../frontend/basic_block.hpp"
#include "../looppip/ii_calculator.hpp"


using loop_schedule_result_t = std::vector<std::array<>instruction_t, 5>;

schedule_t schedule_loop(
	std::vector<instruction_t>& program, 
	const dependency_table_t& dependency_table, 
	const basic_block_info_t& block_info);