#include <iostream>
#include <string>
#include <filesystem>
#include <utility>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "instruction.hpp"
#include "basic_block.hpp"
#include <exception>
#include <iostream>
#include <vector>

#include "frontend/program_loader.hpp"
#include "frontend/instruction_parser.hpp"
#include "frontend/basic_block.hpp"

#include "middleend/dependency_analysis.hpp"

#include "common/instruction.hpp"
#include "common/schedule.hpp"

#include "backend/output/schedule_writer.hpp"

// #include "backend/loop_scheduling.hpp"

#include "backend/looppip/looppip_scheduler.hpp"
#include "backend/looppip/rotating_register_allocator.hpp"
#include "backend/looppip/looppip_preparer.hpp"

int main(int argc, char* argv[])
{
    Paths paths = parse_paths(argc, argv);

    // 1. parse input_program into Instruction objects
    program_t raw_program = parse_instructions(paths.input_path);
    std::vector<instruction_t> program = parse_program(raw_program);
    // 2. split BB0 / BB1 / BB2
    basic_block_info_t block_info = split_basic_blocks(program);

    // 3. analyze dependencies
    dependency_table_t dependency_table = analyze_dependencies(program);
    
   
    // 4. No-loop case

    //if (!block_info.has_loop)
    // {
    //    const schedule_t schedule = schedule_loop(program, dependency_table, block_info);

    //    write_schedule(paths.loop_output_path, schedule);
    //    write_schedule(paths.looppip_output_path, schedule);

    //    return 0;
    // }

    // 5. generate loop_schedule
    // schedule_t loop_schedule = schedule_loop(program, dependency_table, block_info);
    // 6. generate looppip_schedule

    looppip_schedule_result_t looppip_result =
    schedule_looppip(
        program,
        dependency_table,
        block_info
    );

    schedule_t allocated_looppip_schedule =
    allocate_rotating_registers(
        program,
        dependency_table,
        block_info,
        looppip_result
    );

   schedule_t final_looppip_schedule =
    prepare_looppip_schedule(
        allocated_looppip_schedule,
        looppip_result
    );


    //6. Write JSON outputs    


    // write_schedule(paths.loop_output_path, loop_schedule);
    write_schedule(paths.looppip_output_path, final_looppip_schedule);

    return 0;
}



















 

    





















