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
    
    // TODO
    // 4. generate loop_schedule
    schedule_t loop_schedule = schedule_loop(program, deps, block_info);
    // 5. generate looppip_schedule
    schedule_t looppip_schedule = schedule_looppip(program, deps, block_info);

    write_schedule(paths.loop_output_path, loop_schedule);
    write_schedule(paths.looppip_output_path, looppip_schedule);

    return 0;
}



















 

    





















