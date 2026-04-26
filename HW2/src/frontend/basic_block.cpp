#include "basic_block.hpp"

#include <stdexcept>
#include <string>

static bool is_loop_instruction(const instruction_t& instruction)
{
    return instruction.opcode == instruction_opcode_t::Loop ||
           instruction.opcode == instruction_opcode_t::LoopPip;
}

static int get_loop_target_address(const instruction_t& instruction)
{
    if (instruction.operands.empty()) {
        throw std::runtime_error("Loop instruction has no target operand.");
    }

    const operand_t& target_operand = instruction.operands[0];

    if (target_operand.kind != operand_kind_t::Immediate) {
        throw std::runtime_error("Loop target must be an immediate.");
    }

    return static_cast<int>(target_operand.immediate);
}

basic_block_info_t split_basic_blocks(std::vector<instruction_t>& program)
{
    basic_block_info_t info;

    for (int i = 0; i < static_cast<int>(program.size()); i++) {
        if (is_loop_instruction(program[i])) {
            if (info.has_loop) {
                throw std::runtime_error("Multiple loops are not supported.");
            }

            info.has_loop = true;
            info.loop_instruction_address = i;
            info.loop_start_address = get_loop_target_address(program[i]);
        }
    }

    
    // Straight-line program case:
    // There is no loop body and no finalization block.
    // Treat the whole program as BB0 because all instructions execute once.
    // Later stages should handle this as normal local scheduling only:
    // no interloop dependencies, no post-loop dependencies, no loop.pip preparation.
    if (!info.has_loop) {
        for (instruction_t& instruction : program) {
            instruction.block = basic_block_t::BB0;
        }

        return info;
    }



    if (info.loop_start_address < 0 ||
        info.loop_start_address > info.loop_instruction_address ||
        info.loop_start_address >= static_cast<int>(program.size())) {
        throw std::runtime_error(
            "Invalid loop target address: " +
            std::to_string(info.loop_start_address)
        );
    }

    for (int i = 0; i < static_cast<int>(program.size()); i++) {
        if (i < info.loop_start_address) {
            program[i].block = basic_block_t::BB0;
        } else if (i <= info.loop_instruction_address) {
            program[i].block = basic_block_t::BB1;
        } else {
            program[i].block = basic_block_t::BB2;
        }
    }

    return info;
}