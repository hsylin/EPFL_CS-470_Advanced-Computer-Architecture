
#pragma once

#include <array>
#include <string>
#include <vector>

using program_t = std::vector<std::string>;
using bundle_t = std::array<std::string, 5>;
using schedule_t = std::vector<bundle_t>;

struct Paths 
{
    std::string input_path;
    std::string loop_output_path;
    std::string looppip_output_path;
};

Paths parse_paths(int argc, char* argv[]);
program_t parse_instructions(const std::string& path);
void write_schedule(const std::string& path, const schedule_t& schedule);