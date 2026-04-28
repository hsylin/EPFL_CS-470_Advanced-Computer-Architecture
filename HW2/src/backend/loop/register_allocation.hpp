#pragma once

#include "loop_scheduling.hpp";

loop_schedule_result_t rename_registers(loop_schedule_result_t& schedule, const dependency_table_t& dependency_table);