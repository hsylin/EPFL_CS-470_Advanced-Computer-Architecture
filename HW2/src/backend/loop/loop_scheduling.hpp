#pragma once

#include <vector>
#include <array>

#include "../../common/instruction.hpp"
#include "../../middleend/dependency_analysis.hpp"
#include "../../common/schedule.hpp"
#include "../../frontend/basic_block.hpp"
#include "../looppip/ii_calculator.hpp"


using loop_schedule_result_t = std::vector<std::array<instruction_t, 5>>;


inline int max(int x, int y) {
	return x >= y ? x : y;
}

inline std::array<instruction_t, 5> make_empty_instruction_bundle() 
{
	std::array<instruction_t, 5> bundle;
	for (instruction_t i : bundle) {
		i.opcode = instruction_opcode_t::Nop;
	}

	return bundle;
}

inline void ensure_loop_schedule_has_cycle(loop_schedule_result_t& schedule, int cycle)
{
	if (cycle < 0)
	{
		throw std::runtime_error("Schedule cycle cannot be negative.");
	}

	while (static_cast<int>(schedule.size()) <= cycle)
	{
		schedule.push_back(make_empty_instruction_bundle());
	}
}

inline bool is_slot_empty(
	const loop_schedule_result_t& schedule,
	int cycle,
	bundle_slot_t slot
)
{
	if (cycle < 0)
	{
		throw std::runtime_error("Schedule cycle cannot be negative.");
	}


	if (cycle >= static_cast<int>(schedule.size()))
	{
		return true;
	}

	return schedule[cycle][bundle_slot_to_index(slot)].opcode == instruction_opcode_t::Nop;
}

inline bool is_any_slot_empty(
	const loop_schedule_result_t& schedule,
	int cycle,
	std::vector<bundle_slot_t> slots
)
{
	for (bundle_slot_t slot : slots) {
		if (is_slot_empty(schedule, cycle, slot)) return true;
	}

	return false;
}

inline void combine_loop_schedules(
	loop_schedule_result_t& schedule1,
	loop_schedule_result_t& schedule2,
	int padding = 0
)
{
	int last_cycle = schedule1.size() - 1 + padding;
	ensure_loop_schedule_has_cycle(schedule1, last_cycle);

	for (auto bundle : schedule2) {
		schedule1.push_back(bundle);
	}
}

inline void put_instruction_in_loop_schedule(
	loop_schedule_result_t& schedule,
	int cycle,
	bundle_slot_t slot,
	const instruction_t& instruction
)
{
	ensure_loop_schedule_has_cycle(schedule, cycle);

	const int index = bundle_slot_to_index(slot);

	if (schedule[cycle][index].opcode != instruction_opcode_t::Nop)
	{
		throw std::runtime_error(
			"Trying to put instruction into a non-empty bundle slot."
		);
	}

	schedule[cycle][index] = instruction;
}

inline void insert_empty_cycles(loop_schedule_result_t& schedule, int n, int pos) {
	while (n > 0) {
		schedule.insert(schedule.begin() + pos, make_empty_instruction_bundle());
		n--;
	}
}



schedule_t encode_schedule(schedule_loop);

loop_schedule_result_t schedule_loop(
	std::vector<instruction_t>& program, 
	const dependency_table_t& dependency_table, 
	const basic_block_info_t& block_info);