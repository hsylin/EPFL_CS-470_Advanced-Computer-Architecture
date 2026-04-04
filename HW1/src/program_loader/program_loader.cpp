#include "program_loader.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

std::pair<std::string, std::string> parse_paths(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: hw1 <input_path> <output_path>\n";
        std::exit(-1);
    }

    std::string input_path = argv[1];
    std::string output_path = argv[2];

    if (!fs::exists(input_path))
    {
        std::cerr << "Error: Input path does not exist: " << input_path << "\n";
        std::exit(-1);
    }

    if (!fs::is_regular_file(input_path))
    {
        std::cerr << "Error: Input path is not a regular file: " << input_path << "\n";
        std::exit(-1);
    }

    return {
        fs::absolute(input_path).string(),
        fs::absolute(output_path).string()
    };
}

program_t parse_instructions(const std::string& path)
{
    std::ifstream in(path);
    if (!in)
    {
        throw std::runtime_error("Failed to open input file: " + path);
    }

    json j = json::parse(in);

    program_t program;
    program.reserve(j.size());

    for (const auto& x : j)
    {
        program.push_back(x.get<std::string>());
    }

    return program;
}