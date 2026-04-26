#pragma once

#include "instruction.hpp"

#include <cstdint>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// Basic string utilities
// -----------------------------------------------------------------------------

std::string trim(const std::string& text);
std::string to_lower_copy(std::string text);
std::vector<std::string> split_operands(const std::string& text);

// -----------------------------------------------------------------------------
// Operand-level parsers
// -----------------------------------------------------------------------------

operand_t parse_operand(const std::string& text);
register_ref_t parse_register(const std::string& text);
int64_t parse_immediate_value(const std::string& text);
memory_operand_t parse_memory_operand(const std::string& text);



// -----------------------------------------------------------------------------
// Instruction-level helpers
// -----------------------------------------------------------------------------

instruction_opcode_t parse_opcode(const std::string& text);
execution_unit_t execution_unit_for_opcode(instruction_opcode_t opcode);
int latency_for_opcode(instruction_opcode_t opcode);

// -----------------------------------------------------------------------------
// Full instruction / program parser
// -----------------------------------------------------------------------------

instruction_t parse_instruction(const std::string& raw, int instruction_address);
std::vector<instruction_t> parse_program(const std::vector<std::string>& raw_program);