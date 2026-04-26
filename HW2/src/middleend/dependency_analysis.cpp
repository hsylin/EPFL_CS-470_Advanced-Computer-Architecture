#include "dependency_analysis.hpp"

#include <stdexcept>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// Helper functions for dependency analysis
// -----------------------------------------------------------------------------



static bool is_trackable_register(const register_ref_t& reg)
{
    return is_x_register(reg);
}

static bool has_register(
    const std::vector<register_ref_t>& registers,
    const register_ref_t& target
)
{
    for (const register_ref_t& reg : registers) {
        if (reg == target) {
            return true;
        }
    }

    return false;
}

static std::vector<register_ref_t> collect_unique_data_source_registers( const instruction_t& instruction )
{
    std::vector<register_ref_t> result;

    for (const register_ref_t& reg : instruction.src_regs) 
    {
        if (!is_trackable_register(reg)) {
            continue;
        }

        if (!has_register(result, reg)) {
            result.push_back(reg);
        }
    }

    return result;
}

static bool instruction_writes_register(
    const instruction_t& instruction,
    const register_ref_t& reg
)
{
    if (!instruction.dest.has_value()) 
    {
        return false;
    }

    if (!is_trackable_register(*instruction.dest))
    {
        return false;
    }

    return *instruction.dest == reg;
}

static int find_latest_writer_before_in_same_block(
    const std::vector<instruction_t>& program,
    const register_ref_t& reg,
    basic_block_t block,
    int before_address
)
{
    for (int i = before_address - 1; i >= 0; i--) 
    {
        if (program[i].block != block) 
        {
            continue;
        }

        if (instruction_writes_register(program[i], reg)) 
        {
            return program[i].instruction_address;
        }
    }

    return -1;
}

static int find_latest_writer_in_block(
    const std::vector<instruction_t>& program,
    const register_ref_t& reg,
    basic_block_t block
)
{
    int latest_writer = -1;

    for (const instruction_t& instruction : program) 
    {
        if (instruction.block != block) {
            continue;
        }

        if (instruction_writes_register(instruction, reg)) 
        {
            latest_writer = instruction.instruction_address;
        }
    }

    return latest_writer;
}

static dependency_t make_dependency(
    const register_ref_t& operand_register,
    int consumer_address,
    int producer_address,
    int previous_iteration_producer_address = -1
)
{
    dependency_t dependency;

    dependency.operand_register = operand_register;
    dependency.consumer_instruction_address = consumer_address;
    dependency.producer_instruction_address = producer_address;
    dependency.previous_iteration_producer_instruction_address =
        previous_iteration_producer_address;

    return dependency;
}

// -----------------------------------------------------------------------------
// Main functions for dependency analysis
// -----------------------------------------------------------------------------

dependency_table_t analyze_dependencies(const std::vector<instruction_t>& program)
{
    dependency_table_t table;
    table.entries.resize(program.size());


    for (const instruction_t& instruction : program) 
    {
        const int address = instruction.instruction_address;



        instruction_dependency_info_t info;
        info.instruction_address = address;
        info.destination_register = instruction.dest;

        table.entries[address] = info;
    }


    for (const instruction_t& consumer : program) 
    {
        const int consumer_address = consumer.instruction_address;


        instruction_dependency_info_t& consumer_info =
            table.entries[consumer_address];

        const std::vector<register_ref_t> source_registers =
            collect_unique_data_source_registers(consumer);

        for (const register_ref_t& source_reg : source_registers) 
        {
            const basic_block_t consumer_block = consumer.block;


            // Case 1: Local dependency

            const int local_producer =
                find_latest_writer_before_in_same_block(
                    program,
                    source_reg,
                    consumer_block,
                    consumer_address
                );

            if (local_producer != -1) 
            {
                consumer_info.local_dependencies.push_back(
                    make_dependency(
                        source_reg,
                        consumer_address,
                        local_producer
                    )
                );

                continue;
            }

           
            // Case 2: Consumer is in BB1.
            // If BB1 writes this register somewhere, and no local producer was found (Case 1), then this is an interloop dependency.
            // Otherwise, if only BB0 writes it, this is a loop-invariant dependency.
            
            if (consumer_block == basic_block_t::BB1) 
            {
                const int bb0_initial_producer =
                    find_latest_writer_in_block(
                        program,
                        source_reg,
                        basic_block_t::BB0
                    );

                const int bb1_previous_iteration_producer =
                    find_latest_writer_in_block(
                        program,
                        source_reg,
                        basic_block_t::BB1
                    );

                if (bb1_previous_iteration_producer != -1) 
                {
                    consumer_info.interloop_dependencies.push_back(
                        make_dependency(
                            source_reg,
                            consumer_address,
                            bb0_initial_producer,
                            bb1_previous_iteration_producer
                        )
                    );

                    continue;
                }

                if (bb0_initial_producer != -1) 
                {
                    consumer_info.loop_invariant_dependencies.push_back(
                        make_dependency(
                            source_reg,
                            consumer_address,
                            bb0_initial_producer
                        )
                    );

                    continue;
                }


                continue;
            }


            
            // Case 3: Consumer is in BB2.
            // If BB1 writes this register, BB2 consumes the final value produced by the last loop iteration. This is a post-loop dependency.
            // If BB1 does not write this register, but BB0 writes it, then the value comes from loop initialization and is treated as loop-invariant.
            // If BB2 has an earlier writer in the same BB2 block, it was already handled by Case 1 as a local dependency.
            if (consumer_block == basic_block_t::BB2) 
            {
                const int bb1_producer =
                    find_latest_writer_in_block(
                        program,
                        source_reg,
                        basic_block_t::BB1
                    );

                if (bb1_producer != -1) 
                {
                    consumer_info.post_loop_dependencies.push_back(
                        make_dependency(
                            source_reg,
                            consumer_address,
                            bb1_producer
                        )
                    );

                    continue;
                }

                const int bb0_producer =
                    find_latest_writer_in_block(
                        program,
                        source_reg,
                        basic_block_t::BB0
                    );

                if (bb0_producer != -1) 
                {
                    consumer_info.loop_invariant_dependencies.push_back(
                        make_dependency(
                            source_reg,
                            consumer_address,
                            bb0_producer
                        )
                    );

                    continue;
                }


                continue;
            }

        }
    }

    return table;
}