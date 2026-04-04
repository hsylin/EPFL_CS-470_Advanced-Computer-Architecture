#pragma once

#include <string>
#include <utility>
#include <vector>

using program_t = std::vector<std::string>;

std::pair<std::string, std::string> parse_paths(int argc, char* argv[]);
program_t parse_instructions(const std::string& path);