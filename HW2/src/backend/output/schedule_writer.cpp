#include "schedule_writer.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

void write_schedule(const std::string& path, const schedule_t& schedule)
{
    json j = json::array();

    for (const auto& bundle : schedule)
    {
        json row = json::array();

        for (const auto& instruction_text : bundle)
        {
            row.push_back(instruction_text);
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