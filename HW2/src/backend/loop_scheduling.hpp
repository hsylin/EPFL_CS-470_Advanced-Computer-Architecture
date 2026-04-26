#pragma once

#import "schedule.hpp"
#include "dependency_analysis.hpp"
#include "schedule.hpp"
#include "basic_block.hpp"


schedule_t schedule_loop(
	const std::vector<instruction_t>& program, 
	const dependency_table_t& dependency_table, 
	const basic_block_info_t& block_info);