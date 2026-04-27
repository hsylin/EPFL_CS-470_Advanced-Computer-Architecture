#include "schedule_writer.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

static std::string format_output_instruction(const std::string& text)
{
    if (text.empty())
    {
        return text;
    }

    if (text == "nop")
    {
        return " nop";
    }

    if (text[0] == '(')
    {
        return text;
    }

    if (text[0] == ' ')
    {
        return text;
    }

    return " " + text;
}

void write_schedule(const std::string& path, const schedule_t& schedule)
{
    std::ofstream out(path);
    if (!out)
    {
        throw std::runtime_error("Failed to open output file: " + path);
    }

    out << "[\n";

    for (std::size_t i = 0; i < schedule.size(); i++)
    {
        out << "  [";

        for (std::size_t j = 0; j < schedule[i].size(); j++)
        {
            json instruction_json = format_output_instruction(schedule[i][j]);
            out << instruction_json.dump();

            if (j + 1 < schedule[i].size())
            {
                out << ", ";
            }
        }

        out << "]";

        if (i + 1 < schedule.size())
        {
            out << ",";
        }

        out << "\n";
    }

    out << "]\n";
}