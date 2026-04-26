#pragma once

#include "instruction.hpp"

#include <optional>
#include <vector>

struct dependency_t
{

    register_ref_t operand_register;
    int consumer_instruction_address = -1;
    int producer_instruction_address = -1;

    int previous_iteration_producer_instruction_address = -1;
};

struct instruction_dependency_info_t
{
    int instruction_address = -1;

    std::optional<register_ref_t> destination_register;

    std::vector<dependency_t> local_dependencies;
    std::vector<dependency_t> interloop_dependencies;
    std::vector<dependency_t> loop_invariant_dependencies;
    std::vector<dependency_t> post_loop_dependencies;
};

struct dependency_table_t
{

    std::vector<instruction_dependency_info_t> entries;
};

dependency_table_t analyze_dependencies(const std::vector<instruction_t>& program);