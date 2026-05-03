#include "register_allocation.hpp"

#include <optional>
#include <stdexcept>



// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------

static int read_last_p_register(loop_schedule_result_t& schedule) {
	int p_reg = 0;
	for (auto& bundle : schedule) {
		for (instruction_t& instruction : bundle) {

			if (instruction.dest) {
				register_ref_t reg = *instruction.dest;

				if (reg.kind == register_kind_t::P && reg.index > p_reg) {
					p_reg = reg.index;
				}
			}
		}
	}

	return p_reg;
}

static int read_last_x_register(loop_schedule_result_t& schedule) {
	int x_reg = 0;
	for (auto& bundle : schedule) {
		for (instruction_t& instruction : bundle) {

			if (instruction.dest) {
				register_ref_t reg = *instruction.dest;

				if (reg.kind == register_kind_t::X && reg.index > x_reg) {
					x_reg = reg.index;
				}
			}
		}
	}

	return x_reg;
}

static std::vector<dependency_t> all_dependencies(dependency_table_t& dependency_table, int cycle) {
	std::vector<dependency_t> dependencies;

	if (cycle < 0) {
		return dependencies; // The instruction was added during scheduling (Nop or Mov to avoid dependencies)
	}

	instruction_dependency_info_t& entry = dependency_table.entries[cycle];

	for (dependency_t& d : entry.local_dependencies) {
		dependencies.push_back(d);
	}

	for (dependency_t& d : entry.interloop_dependencies) {
		dependencies.push_back(d);
	}

	for (dependency_t& d : entry.post_loop_dependencies) {
		dependencies.push_back(d);
	}

	for (dependency_t& d : entry.loop_invariant_dependencies) {
		dependencies.push_back(d);
	}

	return dependencies;
}

static register_ref_t register_produced_by_original_address(loop_schedule_result_t& schedule, int address) {
	for (auto& bundle : schedule) {
		for (instruction_t& instruction : bundle) {
			
			if (instruction.original_pc == address && instruction.dest) {
				return *instruction.dest;
			}
		}
	}

	throw std::runtime_error(
		"Can't find the cause of a dependency."
	);
}

static void update_register_operand(instruction_t& instruction, int index, register_ref_t& reg) {
	int start = instruction.dest ? 1 : 0;

	for (int i = start; i < static_cast<int>(instruction.operands.size()); i++) {
		if (instruction.operands[i].kind == operand_kind_t::Register) {
			if (index == 0) {
				instruction.operands[i].reg = reg;
				return;
			}
			else {
				index--;
			}
		}
		if (instruction.operands[i].kind == operand_kind_t::Memory) {
			if (index == 0) {
				instruction.operands[i].memory.base_register = reg;
				return;
			}
			else {
				index--;
			}
		}
	}

	throw std::runtime_error("Can't find required operand.");
}

static instruction_t find_loop_instruction(loop_schedule_result_t& schedule) {
	for (auto& bundle : schedule) {
		for (instruction_t& instruction : bundle) {
			if (instruction.opcode == instruction_opcode_t::Loop) {
				return instruction;
			}
		}
	}

	throw std::runtime_error(
		"Can't find the loop instruction"
	);
}

static void instert_mov_at_end_of_loop(
	loop_schedule_result_t& schedule,
	register_ref_t origin_reg,
	register_ref_t dest_reg
) 
{
	instruction_t loop_instruction = find_loop_instruction(schedule);

	int earliest_cycle = loop_instruction.scheduled_cycle;

	for (int i = loop_instruction.operands[0].immediate; i <= loop_instruction.scheduled_cycle; i++) {
		for (instruction_t& instruction : schedule[i]) {
			if (instruction.dest && instruction.dest == origin_reg) {
				earliest_cycle = max(earliest_cycle, instruction.scheduled_cycle + instruction.latency);
			}
		}
	}

	std::vector<bundle_slot_t> slots = possible_slots_for_unit(execution_unit_t::ALU);
	if (earliest_cycle == loop_instruction.scheduled_cycle && !is_any_slot_empty(schedule, earliest_cycle, slots)) {
		earliest_cycle++;
	}

	if (earliest_cycle != loop_instruction.scheduled_cycle) {
		insert_empty_cycles(
			schedule, 
			earliest_cycle - loop_instruction.scheduled_cycle,
			loop_instruction.scheduled_cycle + 1
		);

		remove_instruction_from_loop_schedule(schedule, loop_instruction.scheduled_cycle, bundle_slot_t::Branch);

		loop_instruction.scheduled_cycle = earliest_cycle;
		put_instruction_in_loop_schedule(schedule, earliest_cycle, bundle_slot_t::Branch, loop_instruction);
	}

	instruction_t mov_instruction;
	mov_instruction.opcode = instruction_opcode_t::Mov;
	mov_instruction.dest = dest_reg;

	operand_t dest_op;
	dest_op.kind = operand_kind_t::Register;
	dest_op.reg = dest_reg;

	operand_t origin_op;
	origin_op.kind = operand_kind_t::Register;
	origin_op.reg = origin_reg;

	mov_instruction.operands.push_back(dest_op);
	mov_instruction.operands.push_back(origin_op);
	mov_instruction.src_regs.push_back(origin_reg);

	for (bundle_slot_t& slot : slots) {
		if (is_slot_empty(schedule, earliest_cycle, slot)) {
			put_instruction_in_loop_schedule(schedule, earliest_cycle, slot, mov_instruction);
			mov_instruction.scheduled_cycle = earliest_cycle;
			break;
		}
	}
}

static void rename_register(register_ref_t& reg, int& next_index_p, int& next_index_x) {
	switch (reg.kind) {
		case register_kind_t::P: {
			if (next_index_p > MAX_REG_INDEX) {
				// Shouldn't happen in hw2
				throw std::runtime_error(
					"Renamed too many registers"
				);
			}
			reg.index = next_index_p;
			next_index_p++;
			break;
		}
		case register_kind_t::X: {
			if (next_index_x > MAX_REG_INDEX) {
				// Shouldn't happen in hw2
				throw std::runtime_error(
					"Renamed too many registers"
				);
			}
			reg.index = next_index_x;
			next_index_x++;
			break;
		}

		default:
			break;
		}

	reg.renamed = true;
}



// -----------------------------------------------------------------------------
// Renaming produced registers
// -----------------------------------------------------------------------------

static void rename_produced_registers(loop_schedule_result_t& schedule) {
	int next_index_x = 1;
	int next_index_p = 1;

	for (auto& bundle : schedule) {
		for (instruction_t& instruction : bundle) {

			// Check if the instruction produces a new value
			if (instruction.dest) {
				register_ref_t reg = *instruction.dest;


				rename_register(reg, next_index_p, next_index_x);
				instruction.dest = reg;
				instruction.operands[0].reg = reg;

				if (instruction.operands[0].kind != operand_kind_t::Register) {
					throw std::runtime_error("Destination operand is not a register.");
				}
			}
		}
	}
}



// -----------------------------------------------------------------------------
// Renaming produced registers
// -----------------------------------------------------------------------------

static void rename_operand_registers(
	loop_schedule_result_t& schedule, 
	dependency_table_t& dependency_table
)
{
	for (auto& bundle : schedule) {
		for (instruction_t& instruction : bundle) {
			for (dependency_t& dependency : all_dependencies(dependency_table, instruction.original_pc)) {
				
				if (dependency.producer_instruction_address == -1) {
					continue;
				}

				int new_reg = register_produced_by_original_address(schedule, dependency.producer_instruction_address).index;

				register_ref_t new_operand_reg;
				for (int i = 0; i < static_cast<int>(instruction.src_regs.size()); i++) {
					register_ref_t& reg = instruction.src_regs[i];
					if (reg == dependency.operand_register) {
						reg.index = new_reg;
						reg.renamed = true;
						new_operand_reg = reg;
						update_register_operand(instruction, i, reg);
					}
				}
				
				dependency.operand_register = new_operand_reg;
			}
		}
	}
}



// -----------------------------------------------------------------------------
// Renaming produced registers
// -----------------------------------------------------------------------------

static void remove_interloop_dependencies(
	loop_schedule_result_t& schedule,
	dependency_table_t& dependency_table
)
{
	std::vector<register_ref_t> resolved;

	for (instruction_dependency_info_t& entry : dependency_table.entries) {
		for (dependency_t& dependency : entry.interloop_dependencies) {
			if (dependency.previous_iteration_producer_instruction_address == -1)
			{
				continue;
			}

			if (std::count(resolved.begin(), resolved.end(), dependency.operand_register) > 0) {
				continue;
			}

			register_ref_t origin = register_produced_by_original_address(
				schedule, dependency.previous_iteration_producer_instruction_address);
			register_ref_t dest;
			
			if (dependency.producer_instruction_address != -1) {
				dest = register_produced_by_original_address(schedule, dependency.producer_instruction_address);
			}
			else {
				// If it doesn't have a producer in BB0, the operand register was renamed
				dest = dependency.operand_register; 
			}
			

			instert_mov_at_end_of_loop(
				schedule,
				origin,
				dest
			);

			resolved.push_back(dest);
		}
	}
}



// -----------------------------------------------------------------------------
// Renaming produced registers
// -----------------------------------------------------------------------------


static void rename_starting_registers(
	loop_schedule_result_t& schedule, 
	dependency_table_t& dependency_table
)
{
	int next_index_x = read_last_x_register(schedule) + 1;
	int next_index_p = read_last_p_register(schedule) + 1;

	for (auto& bundle : schedule) {
		for (instruction_t& instruction : bundle) {
			for (int i = 0; i < static_cast<int>(instruction.src_regs.size()); i++) {
				register_ref_t reg = instruction.src_regs[i];
				if (!reg.renamed) {
					rename_register(instruction.src_regs[i], next_index_p, next_index_x);
					update_register_operand(instruction, i, instruction.src_regs[i]);

					for (int j = i; j < static_cast<int>(instruction.src_regs.size()); j++) {
						if (instruction.src_regs[j] == reg && !instruction.src_regs[j].renamed) {
							instruction.src_regs[j] = instruction.src_regs[i];
							update_register_operand(instruction, j, instruction.src_regs[i]);
						}
					}

					// Rename interloop dependencies
					for (dependency_t& dependency : dependency_table.entries[instruction.original_pc].interloop_dependencies) {
						if (dependency.operand_register == reg) {
							dependency.operand_register = instruction.src_regs[i];
						}
					}
				}
			}
		}
	}
}



// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void rename_registers(
	loop_schedule_result_t& schedule,
	const dependency_table_t& dependency_table
) {
	dependency_table_t copy_dependency_table = dependency_table;

	rename_produced_registers(schedule);
	
	rename_operand_registers(schedule, copy_dependency_table);

	rename_starting_registers(schedule, copy_dependency_table);

	remove_interloop_dependencies(schedule, copy_dependency_table);
}