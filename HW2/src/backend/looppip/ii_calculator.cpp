#include "ii_calculator.hpp"

#include <algorithm>
#include <stdexcept>

static int ceil_divide(int numerator, int denominator)
{
    if (denominator <= 0)
    {
        throw std::runtime_error("ceil_divide denominator must be positive.");
    }

    if (numerator < 0)
    {
        throw std::runtime_error("ceil_divide numerator cannot be negative.");
    }

    return (numerator + denominator - 1) / denominator;
}

static bool is_loop_body_instruction(const instruction_t& instruction)
{
    return instruction.block == basic_block_t::BB1;
}

ii_resource_count_t count_loop_body_resources(
    const std::vector<instruction_t>& program
)
{
    ii_resource_count_t counts;

    for (const instruction_t& instruction : program)
    {
        if (!is_loop_body_instruction(instruction))
        {
            continue;
        }

        switch (instruction.unit)
        {
            case execution_unit_t::ALU:
                counts.alu_count++;
                break;

            case execution_unit_t::Mult:
                counts.mult_count++;
                break;

            case execution_unit_t::Mem:
                counts.mem_count++;
                break;

            case execution_unit_t::Branch:
                counts.branch_count++;
                break;

            case execution_unit_t::None:
                break;
        }
    }

    return counts;
}

int calculate_ii_res(
    const std::vector<instruction_t>& program,
    const basic_block_info_t& block_info
)
{
    if (!block_info.has_loop)
    {
        throw std::runtime_error(
            "Cannot calculate II_res: program does not contain a loop."
        );
    }

    const ii_resource_count_t counts =
        count_loop_body_resources(program);

    constexpr int k_num_alu_units = 2;
    constexpr int k_num_mult_units = 1;
    constexpr int k_num_mem_units = 1;
    constexpr int k_num_branch_units = 1;

    const int alu_bound =
        ceil_divide(counts.alu_count, k_num_alu_units);

    const int mult_bound =
        ceil_divide(counts.mult_count, k_num_mult_units);

    const int mem_bound =
        ceil_divide(counts.mem_count, k_num_mem_units);

    const int branch_bound =
        ceil_divide(counts.branch_count, k_num_branch_units);

    const int ii_res = std::max(
        {
            1,
            alu_bound,
            mult_bound,
            mem_bound,
            branch_bound
        }
    );

    return ii_res;
}