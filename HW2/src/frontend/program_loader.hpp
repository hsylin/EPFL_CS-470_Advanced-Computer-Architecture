#pragma once

#include <string>
#include <vector>

using program_t = std::vector<std::string>;

struct Paths 
{
    std::string input_path;
    std::string loop_output_path;
    std::string looppip_output_path;
};

Paths parse_paths(int argc, char* argv[]);
program_t parse_instructions(const std::string& path);
