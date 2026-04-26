#include "program_loader.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

Paths parse_paths(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: scheduler <input.json> <loop.json> <looppip.json>\n";
        std::exit(EXIT_FAILURE);
    }

    std::string input_path = argv[1];
    std::string loop_output_path = argv[2];
    std::string looppip_output_path = argv[3];

    if (!fs::exists(input_path))
    {
        std::cerr << "Error: Input path does not exist: " << input_path << "\n";
        std::exit(EXIT_FAILURE);
    }

    if (!fs::is_regular_file(input_path))
    {
        std::cerr << "Error: Input path is not a regular file: " << input_path << "\n";
        std::exit(EXIT_FAILURE);
    }

    return {
        fs::absolute(input_path).string(),
        fs::absolute(loop_output_path).string(),
        fs::absolute(looppip_output_path).string()
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

    if (!j.is_array())
    {
        throw std::runtime_error("Input JSON must be an array of instruction strings.");
    }

    program_t program;
    program.reserve(j.size());

    for (const auto& x : j)
    {
        if (!x.is_string())
        {
            throw std::runtime_error("Each input instruction must be a string.");
        }
        program.push_back(x.get<std::string>());
    }

    return program;
}

void write_schedule(const std::string& path, const schedule_t& schedule)
{
    json j = json::array();

    for (const auto& bundle : schedule)
    {
        json row = json::array();
        for (const auto& inst : bundle)
        {
            row.push_back(inst);
        }
        j.push_back(row);
    }

    std::ofstream out(path);
    if (!out)
    {
        throw std::runtime_error("Failed to open output file: " + path);
    }

    out << j.dump(4) << "\n";
}