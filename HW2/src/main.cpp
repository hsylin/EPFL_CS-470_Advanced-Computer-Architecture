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
    auto program = parse_instructions(paths.input_path);
    // 2. split BB0 / BB1 / BB2
    std::vector<instruction_t> program = parse_program(program);


    // TODO
    // 3. dependency analysis
    // 4. generate loop_schedule
    schedule_t loop_schedule;
    // 5. generate looppip_schedule
    schedule_t looppip_schedule;


    write_schedule(paths.loop_output_path, loop_schedule);
    write_schedule(paths.looppip_output_path, looppip_schedule);
    basic_block_info_t info = split_basic_blocks(program);

    return 0;
}



















 

    





















