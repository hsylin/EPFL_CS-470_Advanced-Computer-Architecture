#pragma once

#include "loop_scheduling.hpp"

void rename_registers(
	loop_schedule_result_t& schedule, 
	const dependency_table_t& dependency_table
);